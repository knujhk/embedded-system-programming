#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "safe_malloc.h"

//주의 : alloc_list와 freed_queue간에 vptr이동시킬 때 left,right 포인터 수정 필수

//in "list_safe_malloc.h
//static struct v_list_head{ 
//    V_LIST* list_head;
//    V_LIST* list_tail;
//} v_list_head;

V_PTR* safe_malloc(unsigned nbytes){
   //safe_malloc 안에서 링크 추가(+큐 생성)
   //노드들은 virt_free되면 큐로 옮겨짐
   V_LIST* list = NULL;
   V_PTR* vptr;
   for(V_LIST* cur = v_list_head.list_head; cur != NULL; cur = cur->right)
   {
       if(cur->byte_size == nbytes){
            list = cur;
        }
   }
   if(!list)
   {   //요청된 블록 사이즈에 맞는 리스트 자체가 없는 경우
        list = vlist_create(nbytes);
   }

   vptr = vq_deqeueue(list->freed_queue);
   if(!vptr){   //freed 블록이 없어서 새로 만드는 경우
        vptr = vptr_add(list);
   }

    return vptr;
}

void safe_free(V_PTR* p) {
    //ref가 0인 경우 free 필드를 1로 셋 / ref가 1이상인 경우 로그 프린트
    if(p->ref_cnt > 1)
    {
        fprintf(stderr,"couldn't free the block which is referenced\n");
    }
    else
    {
        if(p->ref_cnt == 1)
            virt_free(p);
        else //ref_cnt == 0
            real_free(p);
    }  
    return;
}
void virt_free(V_PTR* p){
    assert(p->addr != NULL);

    p->free_flag = true;
    p->ref_cnt = 0;
}
void real_free(V_PTR* p){
    //virt_freed 블록이 freed 큐로 옮겨졌다면 임의로 해제 불가능
    //메모리에 여유가 없는 상황이면 가비지 컬렉터 작동 조건을 더 엄격하게 설정하면 됨
    
}

//garbage collector
//기능1. alloc_list에 있는 virt_freed블록을 virt_freed큐로 옮기기
//기능2.  