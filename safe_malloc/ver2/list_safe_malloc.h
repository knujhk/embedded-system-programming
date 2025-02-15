#ifndef __SAFE_MALLOC_LIST__
#define __SAFE_MALLOC_LIST__

#include <stdlib.h>
#include "struct_safe_malloc.h"

//V_LIST들을 연결
static struct v_list_head{ 
    V_LIST* list_head;
    V_LIST* list_tail;
} v_list_head;

//ALLOC_HEAD와 FREED_QUEUE 관리
typedef struct v_list{
    unsigned byte_size;
    V_PTR* alloc_head;
    V_PTR* alloc_tail;
    V_FREED_QUEUE* freed_queue;
    struct v_list* left;
    struct v_list* right;
} V_LIST;

V_LIST* vlist_add(unsigned byte_size){
    //byte_size크기의 블록들을 관리하는 링크 생성&연결
    V_LIST* list = (V_LIST*)malloc(sizeof(V_LIST));
    if (list == NULL) {
        perror("malloc() failed: insufficient memory!\n");
        exit(-1);
    }

    list->byte_size = byte_size;
    list->alloc_head = NULL;
    list->alloc_tail = NULL;
    list->freed_queue = vq_create();
    list->right = NULL;

    if(!v_list_head.list_head){
        list->left = NULL;
        v_list_head.list_head = list;
        v_list_head.list_tail = list;
    }
    else{
        list->left = v_list_head.list_tail;
        v_list_head.list_tail->right = list;
        v_list_head.list_tail = list;
    }
	
    return list;
}

int vlist_delete(V_LIST* list) 
{
    assert(list != NULL);
    //alloc된 vptr들
    //freed queue
    //모두 삭제
    for(V_PTR* cur = list->alloc_head; cur != NULL;){
        V_PTR* temp = cur;
        cur = cur->right;

        if(temp->addr)
            free(temp->addr);
        free(temp);
    }

    if(list->freed_queue){
        vq_destroy(list->freed_queue);
    }

	if (list == v_list_head.list_head) 
		v_list_head.list_head = list->right;
	else{
        list->left->right = list->right;
        if(list->right){
            list->right->left = list->left;
        }
    }
    free(list);

	return 0;
}

void vlist_destroy(void) 
{
	for(V_LIST* cur = v_list_head.list_head; cur != NULL;){
        V_LIST* temp = cur;
        cur = cur->right;
        vlist_delete(temp);
    }
}

V_PTR* vptr_add(V_LIST* list)
{
    assert(list != NULL);

    V_PTR* vptr = (V_PTR*)malloc(sizeof(V_PTR));
    if (vptr == NULL) {
        perror("malloc() failed: insufficient memory!\n");
        exit(-1);
    }
    vptr->addr = malloc(list->byte_size);
    if (vptr->addr == NULL) {
        perror("malloc() failed: insufficient memory!\n");
        exit(-1);
    }

    vptr->free_flag = false;
    vptr->ref_cnt = 1;
    vptr->right = NULL;

    if(!list->alloc_head){
        vptr->left = NULL;
        list->alloc_head = vptr;
        list->alloc_tail = vptr;
    }
    else{
        vptr->left = list->alloc_tail;
        list->alloc_tail = vptr;
    }

    return vptr;
}

void vptr_delete(V_LIST* list, V_PTR* vptr)
{
    assert(list != NULL);
    assert(vptr != NULL);

    if(vptr == list->alloc_head){
        if(!list->alloc_head->right){ //하나 남은 블록을 삭제하는 경우
            list->alloc_head = NULL;
            list->alloc_tail = NULL;
        }
        else{
            list->alloc_head = vptr->right;
        }
    }
    else{
        vptr->left->right = vptr->right;
        if(vptr->right){
            vptr->right->left = vptr->left;
        }
    }
    
    //wrapper function에서 vptr->left, right 반드시 처리
}

#endif 