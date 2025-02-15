#include <stdio.h>    
#include <unistd.h>  
#include <fcntl.h>    
#include <string.h>

#define DEVICE_FILE "/dev/my_uart"  // 디바이스 파일 경로 정의

int main(int argc, char *argv[]) {
    int fd;                   // 디바이스 파일의 파일 디스크립터를 저장할 변수
    char buffer[30];           // 전송할 문자열 저장할 버퍼
    ssize_t bytes_read;       // 읽은 바이트 수를 저장할 변수

    if(argc != 2)   //프로그램 인자로 문자열 하나만 들어온 경우가 아닐 때
        return -1;   

    // 디바이스 파일을 열기
    fd = open(DEVICE_FILE, O_RDWR);
    if (fd < 0) {            
        perror("디바이스 파일 열기 실패");  
        return -1;            
    }

    //프로그램 인자를 버퍼에 넣기
    strcpy(buffer,argv[1]);

    int len = strlen(buffer);
    int pos = 0;
    int try = 0;
    int max_try = 50;

    while(pos < len){
        if(try >= max_try){
            fprintf(stderr,"시도 횟수 초과.\n");
            return -1;
        }

        pos += write(fd,buffer+pos,len-pos);
        try++;
    }
    printf("success\n");

    // 파일 닫기
    close(fd);
    return 0;                 // 프로그램 정상 종료
}
