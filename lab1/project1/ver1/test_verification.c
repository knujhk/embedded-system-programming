#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "safe_malloc.h"

void* ref_vptr1(void* arg);

int main() {
    initialize_vptr_arr();

    uint32_t* ptr0 = NULL;
    // uint8_t* ptr1 = NULL;
    // uint8_t* ptr2 = NULL;

    printf("\ntest 1\n");
    printf("vptr%d = safe_malloc(%d);\n",0,40);
    V_PTR* vptr0 = safe_malloc(sizeof(uint32_t)*10);
    ptr0 = (uint32_t*)vptr0->addr;    // type casting
    printf("vptr%d->addr : %p\n",0,vptr0->addr);

    //Dangling Pointers : 할당 해제된 힙을 가리키는 포인터

    printf("safe_free(vptr%d);\n",0);
    safe_free(vptr0);
    printf("vptr%d = safe_malloc(%d);\n",1,40);
    V_PTR* vptr1 = safe_malloc(sizeof(uint32_t)*10);
    uint32_t* ptr1 = vptr1->addr;
    printf("vptr%d->addr : %p\n",1,vptr1->addr);

    printf("\n******************************\n\n");

    
    //test 2 - virt_freed된 블록에 해제하고 재할당할 것
    printf("test 2\n");
    printf("vptr%d = safe_malloc(%d);\n",2,32);
    V_PTR* vptr2 = safe_malloc(sizeof(uint32_t)*8);
    uint32_t* ptr2 = vptr2->addr;
    printf("vptr%d->addr : %p\n",2,vptr2->addr);
    printf("safe_free(vptr%d);\n",2);
    safe_free(vptr2);

    printf("vptr%d = safe_malloc(%d);\n",3,48);
    V_PTR* vptr3 = safe_malloc(sizeof(uint32_t)*12);
    uint32_t* ptr3 = vptr3->addr;
    printf("vptr%d->addr : %p\n",3,vptr3->addr);
    printf("\n******************************\n\n");

    //test 3
    printf("test 3\n");
    V_PTR* vptr4 = safe_malloc(sizeof(uint32_t)*12);
    uint32_t* ptr4 = vptr4->addr;

    V_PTR* vptr5 = safe_malloc(sizeof(uint32_t)*12);
    uint32_t* ptr5 = vptr5->addr;

    V_PTR* vptr6 = safe_malloc(sizeof(uint32_t)*12);
    uint32_t* ptr6 = vptr6->addr;

    V_PTR* vptr7 = safe_malloc(sizeof(uint32_t)*12);
    uint32_t* ptr7 = vptr7->addr;

    V_PTR* vptr8 = safe_malloc(sizeof(uint32_t)*12);
    uint32_t* ptr8 = vptr8->addr;

    V_PTR* vptr9 = safe_malloc(sizeof(uint32_t)*12);
    uint32_t* ptr9 = vptr9->addr;

    V_PTR* vptr10 = safe_malloc(sizeof(uint32_t)*12);
    uint32_t* ptr10 = vptr10->addr;

    V_PTR* vptr11 = safe_malloc(sizeof(uint32_t)*12);
    uint32_t* ptr11 = vptr11->addr;

    //이미 관리 테이블이 꽉 찬 상태
    printf("vptr%d = safe_malloc(%d);\n",12,48);
    V_PTR* vptr12 = safe_malloc(sizeof(uint32_t)*12);

    printf("\n******************************\n\n");


    //test 4
    printf("test 4\n");
    pthread_t thread1;
    vptr1->ref_cnt++;
    int res = pthread_create(&thread1,NULL,ref_vptr1,(void*)vptr1);
    if(res != 0) {
        perror("Thread creation failed");
        exit(EXIT_FAILURE);        
    }
    sleep(1);
    printf("safe_free(vptr%d);\n",1);
    safe_free(vptr1); //쓰레드 안에서 ref_cnt 증가시키도록 만들면 쓰레드 생성 직후엔 딜레이 때문에 safe_free안 되는 경우 있음
    vptr1->ref_cnt--;
    void* thread_res;
    res = pthread_join(thread1,&thread_res);
    
    printf("\n******************************\n\n");

    //test 5
    printf("test 5\n");
    printf("safe_free(vptr%d);\n",1);
    safe_free(vptr1); //virt_free
    printf("safe_free(vptr%d);\n",1);
    safe_free(vptr1); //real_free

    return 0;
}
void* ref_vptr1(void* arg){
    printf("thread in\n");
    V_PTR* p = (V_PTR*)arg;
    sleep(3);
    printf("thread out\n");
}
