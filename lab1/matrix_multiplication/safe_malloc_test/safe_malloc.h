#ifndef __H_SAFE_MALLOC__
#define __H_SAFE_MALLOC__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    void* addr;
    uint8_t idx;
    unsigned byte_size;
    bool free_flag;
    uint8_t ref_cnt;
} V_PTR;

typedef struct {
    V_PTR* table;
    unsigned cnt;
    //unsigned last_alloc_idx;
    //unsigned last_free_idx;
} V_CTRL_BLK;


V_PTR* safe_malloc(unsigned nbytes);
void safe_free(V_PTR* p);
void virt_free(V_PTR* p);
void real_free(V_PTR* p);
void initialize_vptr_arr();

#endif
