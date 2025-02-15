#include "safe_malloc.h"
#include <assert.h>

#define N_ENTRY 10

//목표
//적은 수의 메모리 블록들을 할당/해제 반복할 때 메모리 주소를 빠르게 제공
//한번에 사용한 수 있는 블록의 갯수 제한
//배열 구현은 언제 유리한가- 동시에 사용하는 블록 갯수 적고 크기가 무작위적일 때

// typedef struct {
//     void* addr;
//     uint8_t idx;
//     uint8_t byte_size;
//     bool free_flag;
//     uint8_t ref_cnt;
// } V_PTR;

static V_PTR vptr_arr[N_ENTRY];
static V_CTRL_BLK ctrl_blk = {vptr_arr,0};
//virt_free된 블록 : addr != null && free_flag == true
//real_free된 블록 : addr == null;
int reuse_cnt = 0;
int realloc_cnt = 0;
int alloc_cnt = 0;

V_PTR* safe_malloc(unsigned nbytes) {
    // 우선동적메모리 chunk 10개만 관리
    if(ctrl_blk.cnt < N_ENTRY){
    //1. table entry not full
        //테이블에서 서치
        // 요청된 크기와 동일한 사이즈의 freed 블록이 존재하는가
        for(int i = 0 ; i < N_ENTRY; i++){
            if(vptr_arr[i].byte_size == nbytes && vptr_arr[i].free_flag == VIRT_FREED){
                vptr_arr[i].free_flag = ALLOC;
                vptr_arr[i].ref_cnt++;
                ctrl_blk.cnt++;
                //printf("reuse virt_freed block[%d], ctrl_blk.cnt++ -> cnt = %d\n",i,ctrl_blk.cnt);
                //printf("reuse\n");
                reuse_cnt++;

                ctrl_blk.last_alloc_idx = i;

                return &vptr_arr[i];
            }
        }
        //매칭되는 블록이 없는 경우
        //for debug
        int search_from_here = ctrl_blk.last_alloc_idx + 1;
        for(int i = search_from_here; i < N_ENTRY ; i++){
            if(vptr_arr[i].free_flag >= VIRT_FREED)
            {
                if(vptr_arr[i].free_flag == VIRT_FREED)
                {   
                    free(vptr_arr[i].addr);
                    realloc_cnt++;
                }
                else{
                    alloc_cnt++;
                }

                ctrl_blk.cnt++;
                vptr_arr[i].addr = malloc(nbytes);
                vptr_arr[i].byte_size = nbytes;
                vptr_arr[i].free_flag = ALLOC;
                vptr_arr[i].ref_cnt = 1;
                ctrl_blk.last_alloc_idx = i;
                
                return &vptr_arr[i];
            }
        }
        for(int i = 0; i < search_from_here; i++){
            if(vptr_arr[i].free_flag >= VIRT_FREED)
            {
                if(vptr_arr[i].free_flag == VIRT_FREED)
                {   
                    free(vptr_arr[i].addr);
                    realloc_cnt++;
                }
                else{
                    alloc_cnt++;
                }
                ctrl_blk.cnt++;
                vptr_arr[i].addr = malloc(nbytes);
                vptr_arr[i].byte_size = nbytes;
                vptr_arr[i].free_flag = ALLOC;
                vptr_arr[i].ref_cnt = 1;
                ctrl_blk.last_alloc_idx = i;
                
                return &vptr_arr[i];
            }
        }

        //for release
        // for(int i = ctrl_blk.last_alloc_idx, t = 0; t < N_ENTRY ; i = (i+1) % N_ENTRY){
        //     switch(vptr_arr[i].free_flag){
        //         case VIRT_FREED:
        //             free(vptr_arr[i].addr);
        //         case REAL_FREED:
        //             ctrl_blk.cnt++;
        //             vptr_arr[i].addr = malloc(nbytes);
        //             vptr_arr[i].byte_size = nbytes;
        //             vptr_arr[i].free_flag = ALLOC;
        //             vptr_arr[i].ref_cnt = 1;
        //             vptr_arr[i].idx = i;
        //             ctrl_blk.last_alloc_idx = i;
                
        //             return &vptr_arr[i];
        //         break;
                
        //         default:
        //             break;
        //     }
        // }
    }
    else{
    //2. table entry full 
    //메모리 관리 테이블 entry수를 프로그램이 동시에 사용하는 동적 메모리 수보다 크게 잡아야 함
        printf("There are no freed block. return NULL\n");
        return NULL;
    }
}


void safe_free(V_PTR* p) {
    //V_PTR구조체 멤버들 값 잘 관리해줘야됨
    //real free -> addr = null 
    //ref가 0인 경우 free 필드를 1로 셋 / ref가 1이상인 경우 로그 프린트
    if(p->ref_cnt > 1){
        printf("couldn't free this block. it is being referenced by someone\n");
    }
    else{
        if(p->ref_cnt == 1)
            virt_free(p);
        else //ref_cnt == 0
            real_free(p);
    }
        
    return;
}

void virt_free(V_PTR* p){
    assert(p->ref_cnt == 1);
    p->free_flag = VIRT_FREED;
    p->ref_cnt = 0;
    ctrl_blk.cnt--;
    //printf("virt free [%d], ctrl_blk.cnt-- -> cnt = %d\n",p->idx,ctrl_blk.cnt);
    //printf("    ->virt_free\n");
}
void real_free(V_PTR* p){
    assert(p->ref_cnt == 0);
    //printf("real free [%d], ctrl_blk.cnt no change -> cnt = %d\n",p->idx,ctrl_blk.cnt);
    if(p->free_flag == VIRT_FREED){
        //printf("    ->real_free\n");
        free(p->addr);
        p->free_flag = REAL_FREED;
        p->addr = NULL;
    }
}
void initialize_vptr_arr(){
    for(int i = 0; i < N_ENTRY; i++){
        vptr_arr[i].free_flag = REAL_FREED;
    }
}