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

static void tx_tasklet_handler(unsigned long data);
static enum hrtimer_restart tx_timer_handler(struct hrtimer *timer);
static irqreturn_t rx_irq_handler(int irq, void *dev_id);
static enum hrtimer_restart half_bit_time_timer_handler(struct hrtimer *timer);
static enum hrtimer_restart rx_timer_handler(struct hrtimer *timer);

#define GPIO_MAJOR   255
#define GPIO_MINOR   0
#define GPIO_DEVICE   "my_uart"
#define GPIO_UART_TX    12  
#define GPIO_UART_RX    13

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

static int gpio_open(struct inode* inode, struct file* file);
static int gpio_release(struct inode* inode, struct file* file);
static ssize_t gpio_read(struct file* file, char* buf, size_t len, loff_t* off);
static ssize_t gpio_write(struct file* file, const char* buf, size_t len, loff_t* off);

volatile unsigned* gpio;
char msg[BLOCK_SIZE] = {0};
struct cdev gpio_cdev;
static unsigned int rx_irq;
static int is_rx_irq_registered = 0;

//송신을 위한 태스크릿 등록, 첫번째 인자 이름으로 tasklet_struct 만들어줌
DECLARE_TASKLET(tx_tasklet,tx_tasklet_handler,NULL);

static enum bit_time{
   BIT_TIME_115200 = 8680, //nanosec.. 0.55nanosec 버림
};
struct hrtimer tx_timer; //1bit time length
struct hrtimer rx_timer; //1bit time length
struct hrtimer half_bit_time_timer;
ktime_t nsec_bit_time;
ktime_t nsec_half_bit_time;

//커널에서 제공하는 원형 버퍼 사용
#define BUF_SIZE BLOCK_SIZE * 2
static struct kfifo tx_buf;
static struct kfifo rx_buf;
#define KFIFO_UNIT 1; //n bytes

static unsigned char rx_reg; 
static unsigned char tx_reg;
static unsigned char tx_frame_idx;
static unsigned char rx_frame_idx;

static volatile unsigned int uart_status_reg = 0x00; 
      //0bit : tx_ing?, 1bit : rx_ing?, 
      //2bit : tx_buf할당여부 3bit: rx_buf할당여부
      //4bit : module 초기화됐는지

#define is_tx_ing()  (uart_status_reg & 0x1) //프레임 보내는 동안 활성화
#define is_rx_ing()  (uart_status_reg & 0x2) //
#define is_tx_buf_allocated()  (uart_status_reg & 0x4)
#define is_rx_buf_allocated()  (uart_status_reg & 0x8)
#define is_initialized()  (uart_status_reg & 0x10)

#define set_tx_ing() (uart_status_reg | 0x1)
#define set_rx_ing() (uart_status_reg | 0x2)
#define set_tx_buf_allocated() (uart_status_reg | 0x4)
#define set_rx_buf_allocated() (uart_status_reg | 0x8)
#define set_initialized() (uart_status_reg | 0x10)

#define clear_tx_ing() (uart_status_reg & 0xfffffffe)
#define clear_rx_ing() (uart_status_reg & 0xfffffffd)


static struct file_operations gpio_fop = {
   .owner = THIS_MODULE,
   .open = gpio_open,
   .release = gpio_release,
   .read = gpio_read,
   .write = gpio_write,
};

int start_module(void){
   unsigned int cnt = 1;             // 관리할 장치의 개수를 1로 설정
   //static void* map;                 // 메모리 매핑된 주소를 저장할 포인터 map을 선언
   int add, ret;                          // cdev_add 함수의 반환 값을 저장할 변수 add
   dev_t devno;                      // 장치 번호를 저장할 변수 devno

   printk(KERN_INFO "START MODULE\n"); // 커널 로그에 "START MODULE" 메시지를 출력

   devno = MKDEV(GPIO_MAJOR, GPIO_MINOR); // MAJOR와 MINOR 번호를 사용해 장치 번호 devno 생성
   register_chrdev_region(devno, 1, GPIO_DEVICE); // 생성된 장치 번호를 커널에 등록하여 사용 가능하도록 설정

   cdev_init(&gpio_cdev, &gpio_fop); // 캐릭터 디바이스 구조체 gpio_cdev 초기화하고 file_operations 구조체 연결
   gpio_cdev.owner = THIS_MODULE;     // 이 캐릭터 디바이스가 현재 모듈에 속함을 지정

   add = cdev_add(&gpio_cdev, devno, cnt); // 장치 번호 devno로 gpio_cdev를 커널에 등록

//gpio등록
   // map = ioremap(GPIO_BASE, GPIO_SIZE);    // GPIO 레지스터의 물리 주소를 가상 주소로 매핑하여 map에 저장
   // gpio = (volatile unsigned int*)map;     // map을 gpio로 캐스팅하여 GPIO 레지스터에 접근할 수 있게 설정

   // GPIO_IN(GPIO_UART_RX); // GPIO_BUTTON 핀을 입력 모드로 설정
   // GPIO_OUT(GPIO_UART_TX);   // GPIO_LED 핀을 출력 모드로 설정
   //위 방법 대신 커널 api사용
      //커널에 gpio 할당 요청, 두번째 매개변수는 sysfs에 노출되는 이름
   ret = gpio_request(GPIO_UART_RX,"gpio_uart_rx");
   if(ret < 0){
      printk(KERN_ERR "failed to get gpio\n");
      return -1;
   }
   ret = gpio_request(GPIO_UART_TX,"gpio_uart_tx");
   if(ret < 0){
      printk(KERN_ERR "failed to get gpio\n");
      return -1;
   }
      //gpio 핀의 입출력 방향 설정
   ret = gpio_direction_input(GPIO_UART_RX);
   if(ret < 0){
      printk(KERN_ERR "failed to configure gpio\n");
      goto GPIO_ERR;
   }
   ret = gpio_direction_output(GPIO_UART_TX,1);
   if(ret){   //출력값 high
      printk(KERN_ERR "failed to configure gpio\n");
      goto GPIO_ERR;
   }
   //gpio_get_value() 값 읽기- gpio_set_value() 값 쓰기

//수신을 위한 인터럽트 등록, 우선순위 변경하려면 레지스터 직접 바꿔야됨
   rx_irq = gpio_to_irq(GPIO_UART_RX);
   if(rx_irq < 0){   //출력값 high
      printk(KERN_ERR "failed to get irq_num\n");
      goto IRQ_ERR;
   }
   ret = request_irq(rx_irq, rx_irq_handler, IRQF_TRIGGER_FALLING,
                     "gpio_uart_rx", &gpio_cdev);
   if(ret < 0){
      printk(KERN_ERR "failed to register irq\n");
      goto IRQ_ERR;
   }
   is_rx_irq_registered = 1;


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

   // 1bit time 길이 타이머 초기화
   nsec_bit_time = ktime_set(0, BIT_TIME_115200); 
   hrtimer_init(&tx_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
   tx_timer.function = tx_timer_handler;
   hrtimer_init(&rx_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
   rx_timer.function = rx_timer_handler;

   // 0.5bit time 길이 타이머 초기화
   nsec_half_bit_time = ktime_set(0, (BIT_TIME_115200 >> 2)); 
   hrtimer_init(&half_bit_time_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
   half_bit_time_timer.function = half_bit_time_timer_handler;
    


   //초기화 완료
   set_initialized();

   //예외처리 
IRQ_ERR:
   

GPIO_ERR:
   gpio_free(GPIO_UART_TX);
   gpio_free(GPIO_UART_RX);

   return ret;           
}

void end_module(void){
   dev_t devno = MKDEV(GPIO_MAJOR, GPIO_MINOR);
   unregister_chrdev_region(devno, 1);
   cdev_del(&gpio_cdev);

   //gpio할당 해제
   if (gpio_is_valid(GPIO_UART_TX)) {
      gpio_free(GPIO_UART_TX);
   }
   if (gpio_is_valid(GPIO_UART_RX)) {
      gpio_free(GPIO_UART_RX);
   }

   //인터럽트 해제
   if(is_rx_irq_registered)
      free_irq(rx_irq,&gpio_cdev);

   //버퍼로 사용했던 kfifo해제
   if (kfifo_initialized(&tx_buf))
        kfifo_free(&tx_buf);
    if (kfifo_initialized(&rx_buf))
        kfifo_free(&rx_buf);


   printk(KERN_INFO "END MODUILE\n");
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
   
   unsigned int cnt;              
   unsigned int len_buf;
   unsigned int ret = 0;
   int i;

   if(!is_initialized()){
      printk(KERN_ERR "uart모듈 초기화 안 됨.\n");
      return -1;
   }

   //fifo에서 len만큼 꺼내서 buf에 복사하기

   
   memset(msg, 0, BLOCK_SIZE);

   disable_irq(rx_irq);
   len_buf = kfifo_len(&rx_buf);

   if(len < len_buf){
      cnt = len / KFIFO_UNIT; //버퍼 일부만 읽을 때
   } 
   else{
      cnt = len_buf / KFIFO_UNIT;
   }
   
   for(i = 0; i < cnt; i++){
      ret += kfifo_get(&rx_buf,msg+i);
   }
   enable_irq(rx_irq);
   
   ret = copy_to_user(msg, buf, ret);
   
   return ret;                       
}

static ssize_t gpio_write(struct file* file, const char* buf, size_t len, loff_t* off){
   unsigned int cnt;
   unsigned int ret = 0;
   unsigned int cnt_available;
   int i; 

   //uart코드
   if(!is_initialized()){
      printk(KERN_ERR "uart모듈 초기화 안 됨.\n");
      return -1;
   }

   memset(msg, 0, BLOCK_SIZE);
   cnt = copy_from_user(msg, buf, len);

   //송신 버퍼 status 확인
   //읽는 도중에 스케줄링 되었던 태스크릿이 실행될 수 있으니까 태스크릿 끄고 읽기
   //cnt가 송신버퍼 여유공간보다 크면 넣을 수 있는 만큼만 넣어주기 

   tasklet_disable(&tx_tasklet);
   cnt_available = kfifo_avail(&tx_buf);
   
   if(cnt > cnt_available){
      for(i = 0; i < cnt_available; i++){
         ret += kfifo_in(&tx_buf, msg+i , 1);
      }
   }
   else{
      for(i = 0; i < cnt; i++){
         ret += kfifo_in(&tx_buf, msg+i , 1);
      }
   }

   //태스크릿 활성화, 송신 태스크릿 스케줄링
   tasklet_enable(&tx_tasklet);
   tasklet_schedule(&tx_tasklet);

   return ret;
}

 enum hrtimer_restart rx_timer_handler(struct hrtimer *timer)
{
   switch(rx_frame_idx){
      case 9:  //end of a frame
         clear_tx_ing();
         return HRTIMER_NORESTART;

      default:
         rx_reg = rx_reg | (gpio_get_value(GPIO_UART_RX) << (rx_frame_idx-1));
         rx_frame_idx++;
   }

    return HRTIMER_RESTART;
}

 enum hrtimer_restart half_bit_time_timer_handler(struct hrtimer *timer)
{
   if(gpio_get_value(GPIO_UART_RX)){
   //start bit로 잘못 감지한 상황
      return HRTIMER_NORESTART; 
   }
   //signal level 0 -> 프레임 수신 시작
   rx_frame_idx = 1;
   hrtimer_start(&rx_timer, nsec_bit_time, HRTIMER_MODE_REL);
   
   return HRTIMER_NORESTART;
}

 irqreturn_t rx_irq_handler(int irq, void *dev_id){
   //
   volatile int temp;

   if(irq != rx_irq)
      return IRQ_HANDLED;

   if (kfifo_is_full(&rx_buf)){
      printk(KERN_ERR "RX_BUF overflow\n");
      return IRQ_NONE; 
   }

   set_rx_ing();
   rx_frame_idx = 0;
   rx_reg = 0;
   hrtimer_start(&half_bit_time_timer, nsec_half_bit_time, HRTIMER_MODE_REL);
   while(is_tx_ing()){
         temp += temp;  //just do
   }
   kfifo_in(&rx_buf,&rx_reg,sizeof(rx_reg));
   
   return IRQ_HANDLED;
}

 enum hrtimer_restart tx_timer_handler(struct hrtimer *timer)
{
    //한 바이트 보낼때 lsb부터 보내야함
    // 타이머를 재시작하려면 HRTIMER_RESTART 반환
    // 타이머를 종료하려면 HRTIMER_NORESTART 반환
   //start bit = 0
   //end bit = 1
   //8N1

   switch(tx_frame_idx){
      case 0:  //start of a frame
         gpio_set_value(GPIO_UART_TX, 0);
         tx_frame_idx++;
         break;

      case 9:  //end of a frame
         gpio_set_value(GPIO_UART_TX, 1);
         clear_tx_ing();
         return HRTIMER_NORESTART;

      default:
         gpio_set_value(GPIO_UART_TX, tx_reg & (tx_frame_idx - 1));
         tx_frame_idx++;
   }

   return HRTIMER_RESTART;
}

 void tx_tasklet_handler(unsigned long data){

   volatile int temp;

   while(!kfifo_is_empty(&tx_buf)){
      kfifo_out(&tx_buf,&tx_reg,sizeof(tx_reg));
      tx_frame_idx = 0;
      set_tx_ing();
      // 타이머 시작
      hrtimer_start(&tx_timer, nsec_bit_time, HRTIMER_MODE_REL);
      while(is_tx_ing()){
         temp += temp;  //just do
      }
   }
   
}

MODULE_LICENSE("GPL");
module_init(start_module);
module_exit(end_module);