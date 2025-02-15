#include <stdio.h>
#include <stdint.h>

#define get_size(chunk) (chunk>>2)
#define set_size(chunk,size) chunk = ((chunk & 0x03) | (size << 2))

int main(){
    uint32_t a;
    set_size(a,0x3fffffff);
    printf("%d\n",get_size(a));
    //shift right할 때 msb 복사하기 때문에 저장하고자 하는 데이터 30비트 중 최상위 비트가 1이면 읽을 때 음수로 바뀜
    //그럼 변수 타입이 unsigned면 >> 연산 시 msb를 0로 채우나? yes.
    printf("%d\n",0x3fffffff);

    return 0;
}
