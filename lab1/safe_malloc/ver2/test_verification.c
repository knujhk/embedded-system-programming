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

    V_PTR* vptr0 = safe_malloc(sizeof(uint32_t)*10);

    ptr0 = (uint32_t*)vptr0->addr;    // type casting

    *(ptr0 + 0) = 10;
    *(ptr0 + 1) = 11;
    *(ptr0 + 2) = 12;
    *(ptr0 + 3) = 13;
    *(ptr0 + 4) = 14;
    ptr0[5] = 15;
    ptr0[6] = 16;
    ptr0[7] = 17;
    ptr0[8] = 18;
    ptr0[9] = 19;


    for(uint8_t i = 0; i < 10; i++) {
        printf("%d ", ptr0[i]);
    }
    puts("");

    //Dangling Pointers : 할당 해제된 힙을 가리키는 포인터


    // 댜양하게 랜덤하게 safe_malloc, safe_free.호출하면서
    // 기존에 할당된것 free만 셋팅하고 다시 malloc요청오면 기존에 할당된것 바로 리턴 (재활용_
    // ref_cnt이용해서 혹시 다른 곳에 복사되어 여러군데에서 참조중일때 dangling이슈 안생기도록
    //
    // 해당 특성을 확인하는 테스트 코드를 작성해야 함.

    //test 1
    safe_free(vptr0);
    V_PTR* vptr1 = safe_malloc(sizeof(uint32_t)*10);
    uint32_t* ptr1 = vptr1->addr;
    memset(ptr1,0,vptr1->byte_size);

    //test 2 - virt_freed된 블록에 해제하고 재할당할 것
    V_PTR* vptr2 = safe_malloc(sizeof(uint32_t)*8);
    uint32_t* ptr2 = vptr2->addr;
    memset(ptr2,0,vptr2->byte_size);
    safe_free(vptr2);

    V_PTR* vptr3 = safe_malloc(sizeof(uint32_t)*12);
    uint32_t* ptr3 = vptr3->addr;
    memset(ptr3,0,vptr3->byte_size);

    //test 3
    V_PTR* vptr4 = safe_malloc(sizeof(uint32_t)*12);
    uint32_t* ptr4 = vptr4->addr;
    memset(ptr4,0,vptr4->byte_size);

    V_PTR* vptr5 = safe_malloc(sizeof(uint32_t)*12);
    uint32_t* ptr5 = vptr5->addr;
    memset(ptr5,0,vptr5->byte_size);

    V_PTR* vptr6 = safe_malloc(sizeof(uint32_t)*12);
    uint32_t* ptr6 = vptr6->addr;
    memset(ptr6,0,vptr6->byte_size);

    V_PTR* vptr7 = safe_malloc(sizeof(uint32_t)*12);
    uint32_t* ptr7 = vptr7->addr;
    memset(ptr7,0,vptr7->byte_size);

    V_PTR* vptr8 = safe_malloc(sizeof(uint32_t)*12);
    uint32_t* ptr8 = vptr8->addr;
    memset(ptr8,0,vptr8->byte_size);

    V_PTR* vptr9 = safe_malloc(sizeof(uint32_t)*12);
    uint32_t* ptr9 = vptr9->addr;
    memset(ptr9,0,vptr9->byte_size);

    V_PTR* vptr10 = safe_malloc(sizeof(uint32_t)*12);
    uint32_t* ptr10 = vptr10->addr;
    memset(ptr10,0,vptr10->byte_size);

    V_PTR* vptr11 = safe_malloc(sizeof(uint32_t)*12);
    uint32_t* ptr11 = vptr11->addr;
    memset(ptr11,0,vptr11->byte_size);

    //활성화하면 segfault
    // V_PTR* vptr12 = safe_malloc(sizeof(uint32_t)*12);
    // uint32_t* ptr12 = vptr12->addr;
    // memset(ptr12,0,vptr12->byte_size);

    //test 4
    pthread_t thread1;
    vptr1->ref_cnt++;
    int res = pthread_create(&thread1,NULL,ref_vptr1,(void*)vptr1);
    if(res != 0) {
        perror("Thread creation failed");
        exit(EXIT_FAILURE);        
    }
    safe_free(vptr1); //쓰레드 안에서 ref_cnt 증가시키도록 만들면 쓰레드 생성 직후엔 딜레이 때문에 safe_free안 되는 경우 있음
    vptr1->ref_cnt--;
    
    //test 5
    void* thread_res;
    res = pthread_join(thread1,&thread_res);
    safe_free(vptr1); //virt_free
    safe_free(vptr1); //real_free

    return 0;
}
void* ref_vptr1(void* arg){
    printf("thread in\n");
    V_PTR* p = (V_PTR*)arg;
    sleep(3);
    printf("thread out\n");
}
