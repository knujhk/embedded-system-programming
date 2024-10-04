#include "safe_malloc.h"
#include <assert.h>

#define N_ENTRY 10

//향후 구현할 것들
//1. 힙 관리 테이블 정리 작업을 메모리할당 요청 들어왔을 때 처리하면
//딜레이 커지니까 안 쓰는 블록 정리하는 함수 따로 만들어서 백그라운드에서 돌리는 것도 좋을 듯
//2. safe_malloc 호출 시 할당 요청할 수 있는 최대 크기 정해놔도 좋을 듯(safe_malloc 안에서 assert로 체크)
//3. 블록들을 합치거나 분할하는 기능(각각 해제하고 재할당해야됨)
//4. 테이블에 새로운 엔트리를 올려야 할 때 덮어씌울 블록을 결정하는 알고리즘-clock policy
//5. 관리 테이블 사이즈 키우고 적절한 자료구조도 도입

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

//디버깅용
int reuse = 0;
int alloc = 0;
int realloc_cnt = 0;
int real_free_cnt = 0;
int virt_free_cnt = 0;

V_PTR* safe_malloc(unsigned nbytes) {
    // 우선동적메모리 chunk 10개만 관리
    if(ctrl_blk.cnt < N_ENTRY){
    //1. table entry not full
        //테이블에서 서치
        // 요청된 크기와 동일한 사이즈의 freed 블록이 존재하는가
        for(int i = 0 ; i < N_ENTRY; i++){
            if(vptr_arr[i].byte_size == nbytes && vptr_arr[i].free_flag && vptr_arr[i].addr){
                vptr_arr[i].free_flag = false;
                vptr_arr[i].ref_cnt++;
                vptr_arr[i].idx = i;
                ctrl_blk.cnt++;
                reuse++;
                //printf("freed block[%d] reuse, ctrl_blk.cnt++ -> cnt = %d\n",i,ctrl_blk.cnt);
                return &vptr_arr[i];
            }
        }
        //매칭되는 블록이 없는 경우
        //freed인데 연결된 힙이 존재하는거랑 그렇지 않은거랑 구분 어떻게할까? - 주소가 널
        for(int i = 0 ; i < N_ENTRY; i++){
            if(vptr_arr[i].free_flag == true){ //
                ctrl_blk.cnt++;
                if(vptr_arr[i].addr){
                    free(vptr_arr[i].addr);
                    realloc_cnt++;
                    //printf("reallocate at freed block[%d], ctrl_blk.cnt++ -> cnt = %d\n",i,ctrl_blk.cnt);
                }
                else{
                    alloc++;
                    //printf("allocate at freed block[%d], ctrl_blk.cnt++ -> cnt = %d\n",i,ctrl_blk.cnt);
                }
                
                vptr_arr[i].addr = malloc(nbytes);
                vptr_arr[i].byte_size = nbytes;
                vptr_arr[i].free_flag = false;
                vptr_arr[i].ref_cnt = 1;
                vptr_arr[i].idx = i;
                
                return &vptr_arr[i];
            }
        }
    }
    else{
    //2. table entry full 
        printf("There are no freed block. return NULL\n");
        exit -1;
        //return NULL;
    }
}


void safe_free(V_PTR* p) {
    //V_PTR구조체 멤버들 값 잘 관리해줘야됨
    //real free -> addr = null 
    //ref가 0인 경우 free 필드를 1로 셋 / ref가 1이상인 경우 로그 프린트
    if(p->ref_cnt > 1){
        //printf("couldn't free the block which is referenced\n");
    }
    else{
        if(p->ref_cnt == 1){
            virt_free(p);
            virt_free_cnt++;
        }
        else{ //ref_cnt == 0
            real_free(p);
            real_free_cnt++;
        }
    }
        
    return;
}

void virt_free(V_PTR* p){
    //assert(p->ref_cnt == 1);
    p->free_flag = true;
    p->ref_cnt = 0;
    ctrl_blk.cnt--;
    //printf("virt free [%d], ctrl_blk.cnt-- -> cnt = %d\n",p->idx,ctrl_blk.cnt);
    
}
void real_free(V_PTR* p){
    //assert(p->ref_cnt == 0);
    //printf("real free [%d], ctrl_blk.cnt no change -> cnt = %d\n",p->idx,ctrl_blk.cnt);
    printf("call free\n");
    free(p->addr);
    p->addr = NULL;
}
void initialize_vptr_arr(){
    for(int i = 0; i < N_ENTRY; i++){
        vptr_arr[i].free_flag = true;
    }
}