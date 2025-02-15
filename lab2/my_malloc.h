#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#define MIN_SIZE_CHUNK 32 //32Byte
#define ALIGNMENT_CHUNK 16
#define PAGE_SIZE 4096

typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef struct bag {
    void* addr;
    uint32 size;
    int16 free;
    int16 ref_cnt;
}bag;

struct chunk{
    uint32 prev_size;   
    uint32 chunk_size;  //하위 2bit의미: uint8 status; //0: free 1: allocated 2: top chunk
    
    struct bag* bag_ptr;

    struct chunk* fwd_ptr;  //allocated인 경우 여기서부터 user data
    struct chunk* bwd_ptr;
};

typedef struct chunk* chunkptr;

bag* my_malloc(uint32 byte_size);
void my_free(bag* bag);
void defragmentation();



//status 2B : lower B for flag upper B for ref_cnt
//  0bit-> 1: free 0: alloc 
//chunk_ptr은 chunk_size의 맨 앞 가리킴

/*allocated chunk
    prev_size 4B
    chunk_size 4B
    status 2B
    payload
*/

/*freed chunk -> 재사용한다면 payload로 쓸 수 있는 크기는 chunk_size - 10B
    prev_size 4B
    chunk_size 4B
    status 2B
    fwd_ptr 8B
    bwd_ptr 8B
*/

/*top chunk
    prev_size 4B
    chunk_size 4B
    resource 
*/


