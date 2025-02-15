#ifndef __H_SAFE_MALLOC__
#define __H_SAFE_MALLOC__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define ALLOC 0
#define VIRT_FREED 1
#define REAL_FREED 2

typedef struct {
    void* addr;
    uint16_t byte_size;
    uint8_t free_flag; 
    //0b00: not freed, 0b01: virt freed, 
    //0b10: real freed
    uint8_t ref_cnt;
} V_PTR;

typedef struct {
    V_PTR* table;
    unsigned cnt;
    unsigned last_alloc_idx;
} V_CTRL_BLK;


V_PTR* safe_malloc(unsigned nbytes);
void safe_free(V_PTR* p);
void virt_free(V_PTR* p);
void real_free(V_PTR* p);
void initialize_vptr_arr();

#endif
