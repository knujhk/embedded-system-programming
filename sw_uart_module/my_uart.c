#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/kfifo.h>
#include <linux/ktime.h>
#include <linux/hrtimer.h>
#include <linux/semaphore.h>
#include <linux/delay.h>

static enum hrtimer_restart tx_timer_handler(struct hrtimer *timer);
static irqreturn_t rx_irq_handler(int irq, void *dev_id);
static enum hrtimer_restart half_bit_time_timer_handler(struct hrtimer *timer);
static enum hrtimer_restart rx_timer_handler(struct hrtimer *timer);

#define GPIO_MAJOR   255
#define GPIO_MINOR   0
#define GPIO_DEVICE   "my_uart"
#define GPIO_UART_TX    12  
#define GPIO_UART_RX    13 //gpio 45
#define UART_RX 45

#define GPIO_BASE   0x4804C000   //GPIO 1
#define GPIO_END   0x4804CFFF    //GPIO 1
#define GPIO_SIZE   GPIO_END-GPIO_BASE

#define GPIO_IRQSTATUS_0 0x2C    //인터럽트 트리거됐는지 확인 or clear가능
#define GPIO_IRQSTATUS_SET_0 0x34   //인터럽트 활성화
#define GPIO_IRQSTATUS_CLR_0 0x3C   //인터럽트 비활성화
#define GPIO_DATAIN     0x138
#define GPIO_DATAOUT    0x13C
#define GPIO_OE_OFF      0x134   //0: output mode 1: input mode
#define GPIO_FALLINGDETECT 0x14C
#define GPIO_SET_OFF   0x194
#define GPIO_CLEAR_OFF   0x190

#define GPIO_IN(g)      ((*(gpio+GPIO_OE_OFF/4)) |= (1<<g))
#define GPIO_OUT(g)      ((*(gpio+GPIO_OE_OFF/4)) &= ~(1<<g))
#define GPIO_SET(g)      ((*(gpio+GPIO_SET_OFF/4)) |= (1<<g))
#define GPIO_CLEAR(g)   ((*(gpio+GPIO_CLEAR_OFF/4)) |= (1<<g))
#define GPIO_READ(g)   ((*(gpio+GPIO_DATAIN/4)) & (1<<g)) ? 1:0

static int gpio_open(struct inode* inode, struct file* file);
static int gpio_release(struct inode* inode, struct file* file);
static ssize_t gpio_read(struct file* file, char* buf, size_t len, loff_t* off);
static ssize_t gpio_write(struct file* file, const char* buf, size_t len, loff_t* off);

volatile unsigned int* gpio;
char msg[BLOCK_SIZE] = {0};
struct cdev gpio_cdev;
static unsigned int rx_irq;
static int is_rx_irq_registered = 0;

struct semaphore sem_tx;

static enum bit_time{   //baud rate 9600
   B115200 = 8680, //nanosec.. 0.55nanosec 버림
   B9600 = 104000,
   B19200 = 52000,
};
static unsigned int bit_time;

struct hrtimer tx_timer; 
struct hrtimer rx_timer; 
ktime_t nsec_bit_time;
ktime_t nsec_half_bit_time;


//커널에서 제공하는 원형 버퍼 사용
#define BUF_SIZE BLOCK_SIZE * 2
static struct kfifo tx_buf;
static struct kfifo rx_buf;
#define KFIFO_UNIT 1 //n bytes

static volatile unsigned char next_gpio_val;
static volatile unsigned char rx_reg; 
static volatile unsigned char tx_reg;
static volatile unsigned char tx_out[8];
static volatile unsigned char tx_frame_idx;
static volatile unsigned char rx_frame_idx;
static char tx_buffer[BUF_SIZE];

static volatile unsigned int uart_status_reg = 0x00; 
   //0bit : 프레임 송신중인지
   //1bit : 프레임 수신중인지
   //2bit : tx_buf할당여부 3bit: rx_buf할당여부
   //4bit : module 초기화됐는지

#define is_tx_ing()  (uart_status_reg & 0x1) //프레임 보내는 동안 활성화
#define is_rx_ing()  (uart_status_reg & 0x2) //
#define is_tx_buf_allocated()  (uart_status_reg & 0x4)
#define is_rx_buf_allocated()  (uart_status_reg & 0x8)
#define is_initialized()  (uart_status_reg & 0x10)

#define set_tx_ing() uart_status_reg = (uart_status_reg | 0x1)
#define set_rx_ing() uart_status_reg = (uart_status_reg | 0x2)
#define set_tx_buf_allocated() uart_status_reg = (uart_status_reg | 0x4)
#define set_rx_buf_allocated() uart_status_reg = (uart_status_reg | 0x8)
#define set_initialized() uart_status_reg = (uart_status_reg | 0x10)

#define clear_tx_ing() uart_status_reg = (uart_status_reg & 0xfffffffe)
#define clear_rx_ing() uart_status_reg = (uart_status_reg & 0xfffffffd)


static struct file_operations gpio_fop = {
   .owner = THIS_MODULE,
   .open = gpio_open,
   .release = gpio_release,
   .read = gpio_read,
   .write = gpio_write,
};

int start_module(void){
   unsigned int cnt = 1;             // 관리할 장치의 개수를 1로 설정
   static void* map;                 // 메모리 매핑된 주소를 저장할 포인터 map을 선언
   int add, ret;                          // cdev_add 함수의 반환 값을 저장할 변수 add
   dev_t devno;                      // 장치 번호를 저장할 변수 devno
   int i;

   printk(KERN_INFO "START MODULE\n"); // 커널 로그에 "START MODULE" 메시지를 출력

   devno = MKDEV(GPIO_MAJOR, GPIO_MINOR); // MAJOR와 MINOR 번호를 사용해 장치 번호 devno 생성
   register_chrdev_region(devno, 1, GPIO_DEVICE); // 생성된 장치 번호를 커널에 등록하여 사용 가능하도록 설정

   cdev_init(&gpio_cdev, &gpio_fop); // 캐릭터 디바이스 구조체 gpio_cdev 초기화하고 file_operations 구조체 연결
   gpio_cdev.owner = THIS_MODULE;     // 이 캐릭터 디바이스가 현재 모듈에 속함을 지정

   add = cdev_add(&gpio_cdev, devno, cnt); // 장치 번호 devno로 gpio_cdev를 커널에 등록

   //gpio등록
   map = ioremap(GPIO_BASE, GPIO_SIZE);    // GPIO 레지스터의 물리 주소를 가상 주소로 매핑하여 map에 저장
   gpio = (unsigned int*)map;     // map을 gpio로 캐스팅하여 GPIO 레지스터에 접근할 수 있게 설정

   GPIO_IN(GPIO_UART_RX); // GPIO_BUTTON 핀을 입력 모드로 설정
   GPIO_OUT(GPIO_UART_TX);   // GPIO_LED 핀을 출력 모드로 설정
   
   //커널 api사용 
      //커널에 gpio 할당 요청, 두번째 매개변수는 sysfs에 노출되는 이름
   ret = gpio_request(UART_RX,"gpio_uart_rx");
   if(ret < 0){
      printk(KERN_ERR "failed to get gpio\n");
      return -1;
   }
   
      //gpio 핀의 입출력 방향 설정
   ret = gpio_direction_input(UART_RX);
   if(ret < 0){
      printk(KERN_ERR "failed to configure gpio\n");
      goto GPIO_ERR;
   }
   //gpio_export(UART_RX,false);
   

   //수신을 위한 인터럽트 등록, 우선순위 변경하려면 레지스터 직접 바꿔야됨
   rx_irq = gpio_to_irq(UART_RX);
   if(rx_irq < 0){   //출력값 high
      printk(KERN_ERR "failed to get irq_num\n");
      goto IRQ_ERR;
   }
   ret = request_irq(rx_irq, rx_irq_handler, IRQF_TRIGGER_FALLING,
                     "gpio_uart_rx", NULL);
   if(ret < 0){
      printk(KERN_ERR "failed to register irq\n");
      goto IRQ_ERR;
   }
   is_rx_irq_registered = 1;

   //bit_time = B9600;
   bit_time = B19200;
   

   //송수신 버퍼 초기화
   ret = kfifo_alloc(&tx_buf, BUF_SIZE, GFP_KERNEL);
   if(ret){
      printk(KERN_ERR "버퍼 초기화 실패.\n");
      goto IRQ_ERR;
   }
   set_tx_buf_allocated();

   ret = kfifo_alloc(&rx_buf, BUF_SIZE, GFP_KERNEL);
   if(ret){
      printk(KERN_ERR "버퍼 초기화 실패.\n");
      goto IRQ_ERR;
   }
   set_rx_buf_allocated();

   //ktime set
   nsec_bit_time = ktime_set(0, bit_time);
   nsec_half_bit_time = ktime_set(0, (bit_time / 2)); 

   //타이머 초기화
   hrtimer_init(&tx_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
   tx_timer.function = tx_timer_handler;
   hrtimer_init(&rx_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
   rx_timer.function = rx_timer_handler;
    
   sema_init(&sem_tx,0);

   GPIO_SET(GPIO_UART_TX);
   printk("tx initialized : %d\n",GPIO_READ(GPIO_UART_TX));

   uart_status_reg = 0;
   //초기화 완료
   set_initialized();
   printk(KERN_INFO "초기화 완료.\n");
   
   return ret;

   //예외처리 
IRQ_ERR:
   

GPIO_ERR:
   gpio_free(GPIO_UART_RX);

   return ret;           
}

void end_module(void){
   dev_t devno = MKDEV(GPIO_MAJOR, GPIO_MINOR);
   unregister_chrdev_region(devno, 1);
   cdev_del(&gpio_cdev);

   if(gpio){
      iounmap(gpio);
   }

   //gpio할당 해제
   //gpio_unexport(GPIO_UART_RX);
   if (gpio_is_valid(GPIO_UART_RX)) {
      gpio_free(GPIO_UART_RX);
   }

   //인터럽트 해제
   if(is_rx_irq_registered)
      free_irq(rx_irq,NULL);
   

   //버퍼로 사용했던 kfifo해제
   if (kfifo_initialized(&tx_buf))
        kfifo_free(&tx_buf);
    if (kfifo_initialized(&rx_buf))
        kfifo_free(&rx_buf);


   printk(KERN_INFO "END MODULE\n");
}

static int gpio_open(struct inode* inode, struct file* file){
   try_module_get(THIS_MODULE);
   printk("OPEN - my_uart device\n");
   return 0;
}

static int gpio_release(struct inode* inode, struct file* file){
   module_put(THIS_MODULE);
   printk("CLOSE - my_uart device\n");
   return 0;
}

static ssize_t gpio_read(struct file* file, char* buf, size_t len, loff_t* off){
                
   unsigned int ret = 0;
   unsigned int cnt_fail = 0;
   int i;

   if(!is_initialized()){
      printk(KERN_ERR "uart모듈 초기화 안 됨.\n");
      return -1;
   }

   memset(msg, 0, BLOCK_SIZE);

   disable_irq(rx_irq);
   len = kfifo_out(&rx_buf,msg,len);
   enable_irq(rx_irq);
   
   cnt_fail = copy_to_user(buf, msg, len);

   return len-cnt_fail;                       
}

static ssize_t gpio_write(struct file* file, const char* buf, size_t len, loff_t* off){
   unsigned int cnt;
   unsigned int ret = 0;
   unsigned int cnt_available;
   unsigned int cnt_fail = 0;
   int i;

   //uart코드
   if(!is_initialized()){
      printk(KERN_ERR "uart모듈 초기화 안 됨.\n");
      return -1;
   }

   memset(msg, 0, BLOCK_SIZE);
   cnt_fail = copy_from_user(msg, buf, len);
   cnt = len - cnt_fail; //cnt = copy from user로 읽어들인 바이트수

   //송신 버퍼 status 확인
   cnt_available = kfifo_avail(&tx_buf);

   if(cnt_available < cnt){
      printk(KERN_ERR "tx_buffer 공간 부족.\n");
      return -1;
   }
   else{
      for(; ret < cnt; ){
         ret += kfifo_in(&tx_buf, msg+ret , cnt - ret);
      }
   }

   //딜레이 이용하는 방법
   // while(!kfifo_is_empty(&tx_buf)){
   //    kfifo_out(&tx_buf,&tx_reg,sizeof(tx_reg));
   //    for(tx_frame_idx=0;tx_frame_idx<10;tx_frame_idx++){
   //       switch(tx_frame_idx){
   //          case 0:  //start of a frame
   //             GPIO_CLEAR(GPIO_UART_TX);
   //             break;
   //          case 9:  //end of a frame
   //             GPIO_SET(GPIO_UART_TX);
   //             break;
   //          default:
   //             if(tx_reg & (1 << (tx_frame_idx - 1))){
   //                GPIO_SET(GPIO_UART_TX);
   //                //gpio_set_value(GPIO_UART_TX, 1);
   //             }
   //             else{
   //                GPIO_CLEAR(GPIO_UART_TX);
   //             }
   //       }
   //       ndelay(bit_time);
   //    }
   // }

   while(!kfifo_is_empty(&tx_buf)){
      if(!kfifo_out(&tx_buf,&tx_reg,KFIFO_UNIT)){
            printk(KERN_ERR "kfifo pop error\n");
            return -1;
      }
      tx_frame_idx = 0;
      hrtimer_start(&tx_timer, nsec_bit_time, HRTIMER_MODE_REL);
      down_interruptible(&sem_tx);
   }


   return ret;
}

 enum hrtimer_restart rx_timer_handler(struct hrtimer *timer)
{
   //printk(KERN_ERR "rx_timer_handler.\n");
   switch(rx_frame_idx){
      case 0:
         if(GPIO_READ(GPIO_UART_RX)){
         //start bit가 아니라 노이즈였던것.
            //printk(KERN_ERR "noise.\n");
            clear_rx_ing();
            return HRTIMER_NORESTART; 
         }
         rx_frame_idx++;
         break;

      case 9:  //end of a frame
         if(kfifo_in(&rx_buf, &rx_reg , KFIFO_UNIT) < 1){
            printk(KERN_ERR "RX_BUF overflow\n");
         }
         clear_rx_ing();
         return HRTIMER_NORESTART;

      default:
         if(GPIO_READ(GPIO_UART_RX)){
            rx_reg = rx_reg | (1 << (rx_frame_idx-1));
         }
         rx_frame_idx++;
   }
   hrtimer_forward_now(timer,nsec_bit_time);
   return HRTIMER_RESTART;
}

 irqreturn_t rx_irq_handler(int irq, void *dev_id){
   
   //printk(KERN_ERR "수신 감지.\n");
   if(is_rx_ing()){
      return IRQ_HANDLED;   
   }

   //
   hrtimer_start(&rx_timer, nsec_half_bit_time, HRTIMER_MODE_REL);
   set_rx_ing();
   //

   // uart모듈과 인터럽트 라인 공유하는 하드웨어 없어서 아래 주석 부분은 불필요
   // if(irq != rx_irq){ 
   //    hrtimer_cancel(&rx_timer);
   //    printk(KERN_ERR "false irq.\n");
   //    return IRQ_HANDLED;
   // }

   rx_reg = 0;
   rx_frame_idx = 0;

   return IRQ_HANDLED;
}

 enum hrtimer_restart tx_timer_handler(struct hrtimer *timer)
{
   switch(tx_frame_idx){
      case 0:  //start of a frame
         GPIO_CLEAR(GPIO_UART_TX);
         next_gpio_val = tx_reg & (1 << (tx_frame_idx));
         tx_frame_idx++;
         break;

      case 9:  //end of a frame
         GPIO_SET(GPIO_UART_TX);
         up(&sem_tx);
         return HRTIMER_NORESTART;
         
      default:
         if(next_gpio_val){
            GPIO_SET(GPIO_UART_TX);
         }
         else{
            GPIO_CLEAR(GPIO_UART_TX);
         }
         next_gpio_val = tx_reg & (1 << (tx_frame_idx));
         tx_frame_idx++;
   }
   hrtimer_forward_now(timer,nsec_bit_time);
   return HRTIMER_RESTART;
}

MODULE_LICENSE("GPL");
module_init(start_module);
module_exit(end_module);