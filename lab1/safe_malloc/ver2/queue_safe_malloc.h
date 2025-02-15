#ifndef __SAFE_MALLOC_QUEUE__
#define __SAFE_MALLOC_QUEUE__

#include "struct_safe_malloc.h"
#include <stdlib.h>
#include <stdbool.h>


//freed_q
typedef struct freed_queue{
    V_PTR *front;
    V_PTR *rear;
    uint16_t count;
}V_FREED_QUEUE;

V_FREED_QUEUE* vq_create(void)
{
    V_FREED_QUEUE* q = (V_FREED_QUEUE*)malloc(sizeof(V_FREED_QUEUE));
    if (q == NULL) {
        perror("malloc() failed: insufficient memory!\n");
        exit(-1);
    }
    q->count = 0;
    q->front = NULL;
    q->rear = NULL;

    return  q;
}

int vq_enqueue(V_FREED_QUEUE *this, V_PTR *vptr)
{
    assert(this != NULL);
    assert(vptr != NULL);

    if(!this->front){
        this->front = vptr;
        this->rear = vptr;
    }
    else{
        this->rear->right = vptr;
        this->rear = vptr;
    }
    this->count++;

    return 0;
}

V_PTR* vq_dequeue(V_FREED_QUEUE *this)  //nullable
{
    assert(this != NULL);

    if (!this->front) {
        //printf("the queue is empty.\n");
        return NULL;
    }

    V_PTR* temp = this->front;
    
    if (this->count == 1)
        this->front = this->rear = NULL;
    else 
        this->front = temp->right;

    this->count--;

    return temp;
}

int vq_destroy(V_FREED_QUEUE *this)
{
    assert(this != NULL);

    V_PTR* cur, *temp = NULL;

    if (!this->front)
        free(this);
    else {
        for (cur = this->front ;cur != NULL;)
        {
            temp = cur;
            cur = cur->right;
            free(temp);
        }
        free(this);
    }

    return 0;
}

#endif