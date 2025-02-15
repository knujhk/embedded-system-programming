#ifndef __STRUCT_HEADERS__
#define __STRUCT_HEADERS__

#include <stdint.h>
#include <stdio.h>
#include <assert.h>

typedef struct {
    void* addr;
    bool free_flag;
    uint16_t ref_cnt;
    V_PTR* left;
    V_PTR* right;
} V_PTR;

#endif