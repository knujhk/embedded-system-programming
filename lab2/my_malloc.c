#include "my_malloc.h"
#include <unistd.h>
#include <string.h>
#include <pthread.h>


static chunkptr bin = NULL;

#define MAX_BAG 100
static bag bag_arr[MAX_BAG];

#define ALLOC_OVERHEAD 16 //chunk안의 prev_size, chunk_size, bag_ptr
#define MAX_ALLOC_SIZE 1000000000 //2^30B payload + 16B overhead ~ 약 10^9으로 제한

//payload addr -> chunk_ptr
#define get_ptr_chunk(addr) ((chunkptr)(((char*)addr)-16))
//chunk_ptr -> payload addr
#define get_ptr_payload(ptr_chunk) &(ptr_chunk->fwd_ptr)

#define get_prev_size(ptr_chunk) ptr_chunk->prev_size
#define set_prev_size(ptr_chunk,size) ptr_chunk->prev_size = size
#define get_chunk_size(ptr_chunk) ((ptr_chunk->chunk_size)>>2)
#define set_chunk_size(ptr_chunk,size) ptr_chunk->chunk_size = ((ptr_chunk->chunk_size & 0x03) | (size << 2))

#define set_status_top(ptr_chunk) ptr_chunk->chunk_size = ((ptr_chunk->chunk_size & 0xfffffffc) | 0x2)
#define set_status_alloc(ptr_chunk) ptr_chunk->chunk_size = ((ptr_chunk->chunk_size & 0xfffffffc) | 0x1)
#define set_status_free(ptr_chunk) ptr_chunk->chunk_size = ptr_chunk->chunk_size & 0xfffffffc
#define is_alloc(ptr_chunk) (ptr_chunk->chunk_size) & 0x00000001
#define is_top_chunk(ptr_chunk) (ptr_chunk->chunk_size) & 0x00000002

#define get_ref_cnt(ptr_chunk) ptr_chunk->bag_ptr->ref_cnt
#define set_ref_cnt(ptr_chunk,val) ptr_chunk->bag_ptr->ref_cnt = val
#define add_ref_cnt(ptr_chunk,val) ptr_chunk->bag_ptr->ref_cnt += val

//for freed chunk
#define get_fwd_ptr(ptr_chunk) ptr_chunk->fwd_ptr
#define get_bwd_ptr(ptr_chunk) ptr_chunk->bwd_ptr
#define set_fwd_ptr(ptr_chunk,ptr) ptr_chunk->fwd_ptr = ptr
#define set_bwd_ptr(ptr_chunk,ptr) ptr_chunk->bwd_ptr = ptr

static chunkptr top_chunk = NULL;
static void* heap_start = NULL;
static int n_allocated_bag = 0;

#define get_top_chunk_resource() (get_chunk_size(top_chunk))-8
#define MIN_TOP_RESOURCE 2048

static bool initialized = false;

pthread_mutex_t mutex_my_malloc;

static void _initialization();
static void _get_virt_mem(uint32 byte_size);
static void _map_bag(chunkptr chunk, uint32 size_payload);
static void _unmap_bag(chunkptr chunk);
static chunkptr _find_free_chunk(uint32 byte_size);
static chunkptr _alloc_from_top(uint32 byte_size);
static void _move_top_chunk(uint32 offset, int down);
static void _move_chunk(chunkptr chunk, uint32 offset, int down);
static uint32 _calc_block_size(uint32 byte_size, uint32 min, uint32 unit);

static void _initialization(){
    //한 페이지 크기만큼 힙 늘리고 top청크에 등록
    int res = pthread_mutex_init(&mutex_my_malloc, NULL);

    void* old_brk = sbrk(4096);
    heap_start = old_brk;
    assert(old_brk > 0);

    top_chunk = (chunkptr)old_brk;
    set_prev_size(top_chunk,0);
    set_chunk_size(top_chunk,4096);
    set_status_top(top_chunk);

    for(int i = 0; i < MAX_BAG; i++){
        bag_arr[i].free = 1;
    }

    initialized = true;
}

static void _map_bag(chunkptr chunk, uint32 size_payload){//두번째 인자는 실제 payload크기x 사용자가 요청한 크기
    //bag_arr에서 free인 bag하나 골라 포인터를 청크에 등록
    assert(chunk->bag_ptr == NULL);
    assert(n_allocated_bag < MAX_BAG);

    for(int i = 0; i < MAX_BAG; i++){
        if(bag_arr[i].free == 1){
            chunk->bag_ptr = &(bag_arr[i]);
            bag_arr[i].addr = get_ptr_payload(chunk);
            bag_arr[i].size = size_payload;
            bag_arr[i].free = 0;
            bag_arr[i].ref_cnt = 1;

            n_allocated_bag++;
            return;
        }
    }

    fprintf(stderr,"failed to map_bag\n");
    exit(-1);
}

static void _unmap_bag(chunkptr chunk){
    chunk->bag_ptr->free = 1;
    chunk->bag_ptr = NULL;
    n_allocated_bag--;
}

bag* my_malloc(uint32 byte_size){
    if(!initialized)
        _initialization();
    //16바이트 alignment
    if(byte_size > MAX_ALLOC_SIZE){
        printf("can't allocate a block bigger than 10^9B\n");
        return NULL;
    }

    pthread_mutex_lock(&mutex_my_malloc);

    //남은 bag이 없다면
    if(n_allocated_bag >= MAX_BAG)
        return NULL;

    //bin에 연결된 free청크 순회하면서 사이즈 맞는 것 찾아보기
    chunkptr chunk = _find_free_chunk(byte_size);

    if(chunk != NULL){
        //해당 청크 내용을 allocated에 맞게 바꾸고 bag* 리턴
        set_status_alloc(chunk);
        _map_bag(chunk,byte_size);
        pthread_mutex_unlock(&mutex_my_malloc);
        return chunk->bag_ptr;
    }
    else{//없으면 top 청크 쪼갤 수 있는지 확인, 부족하면 가상메모리 추가 할당
        int32_t left = ((int32_t)(get_top_chunk_resource())-((int32_t)byte_size+sizeof(struct chunk)));
        if(left < MIN_TOP_RESOURCE){
            _get_virt_mem(_calc_block_size(byte_size,PAGE_SIZE,PAGE_SIZE));
        }
              
    }
    
    //top청크를 쪼개서 할당(bin에 추가)
    chunk = _alloc_from_top(byte_size);
    _map_bag(chunk,byte_size);

    pthread_mutex_unlock(&mutex_my_malloc);

    return chunk->bag_ptr;
}

#define MIN_VIRTMEM_ALLOC_UNIT 4096

static void _get_virt_mem(uint32 byte_size){

    assert(byte_size % PAGE_SIZE == 0);

    //MIN_VIRTMEM_ALLOC_UNIT 이상, 

    void* old_brk = sbrk(byte_size);
    if(old_brk < 0){
        fprintf(stderr,"system doesn't have enough memory");
        exit(-1);
    }

    uint32 size_before = get_chunk_size(top_chunk);
    set_chunk_size(top_chunk,(size_before+byte_size));
}

#define is_match_size(chunk,byte_size) (get_chunk_size(chunk)-(ALLOC_OVERHEAD+ALIGNMENT_CHUNK-1) <= byte_size && \
                                    get_chunk_size(chunk)-ALLOC_OVERHEAD >= byte_size) ? 1 : 0

static chunkptr _find_free_chunk(uint32 byte_size){
    chunkptr chunk = bin;
    
    while(chunk){
        if(is_match_size(chunk,byte_size)){
            break;
        }
        else{
            chunk = get_fwd_ptr(chunk);
        }
    }

    return chunk;
}

static chunkptr _alloc_from_top(uint32 byte_size){
    
    chunkptr new_chunk = top_chunk;

    //byte_size만큼 할당하기 위한 청크 사이즈 계산
    //payload + 16B  이  MIN_SIZE_CHUNK(32) 이상, 16바이트 alignment만족
    uint32 new_chunk_size =  _calc_block_size(byte_size+16,MIN_SIZE_CHUNK,ALIGNMENT_CHUNK);
    assert((new_chunk_size % 16) == 0);

    //new chunk 내용 set - chunk size,status 2B
    set_chunk_size(new_chunk,new_chunk_size);
    set_prev_size(new_chunk,get_prev_size(top_chunk));
    new_chunk->bag_ptr = NULL;

    //new chunk 사이즈만큼 top chunk내용을 위로 복사
    _move_top_chunk(new_chunk_size,0);
    set_prev_size(top_chunk,new_chunk_size);

    set_status_alloc(new_chunk);

    return new_chunk;
}

static void _move_top_chunk(uint32 offset, int down){
    //호출자에서 set_prev_size 직접 해줘야함
    assert(offset % 16 == 0);

    //down == 1 -> offset만큼 top 청크의 시작주소 down
    //down == 0 -> offset만큼 top 청크의 시작주소 up
    uint32 size_before = get_chunk_size(top_chunk);
    if(down){
        memmove((char*)top_chunk-offset,top_chunk,10);  //why 10?? top청크는 prev_size, chunk_size, status만 옮기면 됨
        top_chunk = (char*)top_chunk - offset;
        set_chunk_size(top_chunk,(size_before+offset));
    }
    else{//up
        memmove((char*)top_chunk+offset,top_chunk,10);
        top_chunk = (char*)top_chunk + offset;
        set_chunk_size(top_chunk,(size_before-offset));
    }
}

static void _move_chunk(chunkptr chunk, uint32 offset, int down){
    //호출자에서 set_prev_size 직접 해줘야함
    assert(offset % 16 == 0);

    if(down){
        memmove((char*)chunk-offset,chunk,get_chunk_size(chunk));
        chunk = (char*)chunk - offset;
    }
    else{//up
        memmove((char*)chunk+offset,chunk,get_chunk_size(chunk));
        chunk = (char*)chunk + offset;
    }

    if(is_alloc(chunk)){
        //청크에 매핑된 bag의 addr도 새 주소로 변경
        chunk->bag_ptr->addr = get_ptr_payload(chunk);
    }
}

static uint32 _calc_block_size(uint32 byte_size, uint32 min, uint32 unit){
    uint32 temp_size = byte_size;
    
    if(temp_size < min){
        temp_size = min;
    }
    else{
        temp_size += (unit - temp_size % unit);
    }

    return temp_size;
}

void my_free(bag* bag){
    if(!bag)
        return;
    
    chunkptr chunk = get_ptr_chunk(bag->addr);
    uint8 ref_cnt = bag->ref_cnt;

    //chunk의 ref_cnt가 2이상이면 아무것도 하지 않기
    if(ref_cnt >= 2){
        return;
    }
    else if(ref_cnt == 1){
        //chunk를 free상태로 바꿔주기
        set_status_free(chunk);
        if(bin == NULL){
            set_fwd_ptr(chunk,NULL);
        }
        else{
            set_fwd_ptr(chunk,bin);
            set_bwd_ptr(bin,chunk);
        }
        bin = chunk;
        set_bwd_ptr(chunk,NULL);
        _unmap_bag(chunk);
    }
    else{
        fprintf(stderr,"invalid ref_cnt\n");
        exit(-1);
    }
}

void defragmentation(){
    //메모리 할당, 해제 락 걸어주기
    pthread_mutex_lock(&mutex_my_malloc);

    uint32 free_sum = 0;
    
    //heap시작위치부터 allocated chunk들이 이웃하도록 복사
    chunkptr cursor = (chunkptr)heap_start;
    uint32 temp_offset;
    chunkptr temp_top;
    chunkptr next;
    while(!(is_top_chunk(cursor))){
        next = (char*)cursor+get_chunk_size(cursor);
        if(is_alloc(cursor)){
            if(temp_offset > 0){
                //현재 cursor가 가리키는 allocated chunk 아래에 free chunk존재
                //temp_offset만큼 아래로 이동
                _move_chunk(cursor,temp_offset,1);
                set_prev_size(cursor,get_chunk_size(temp_top));
                temp_offset = 0;
            }
            else{   //temp_offset == 0
                //사이에 free 청크를 두지 않고 allcated 청크가 이어지는 경우
            }
            temp_top = cursor;
        }
        else{   // cursor -> free chunk
            temp_offset += get_chunk_size(cursor);
            //모든 free 청크의 크기 합산
            free_sum += get_chunk_size(cursor);
        }
        cursor = next;
    }

    //top청크의 크기를 free_sum 만큼 늘림
    _move_top_chunk(free_sum,1);
    
    //free청크들을 모두 없앴으니까
    bin = NULL;

    pthread_mutex_unlock(&mutex_my_malloc);

    return;
}