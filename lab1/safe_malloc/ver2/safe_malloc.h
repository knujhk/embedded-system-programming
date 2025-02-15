#ifndef __H_SAFE_MALLOC__
#define __H_SAFE_MALLOC__

#include "list_safe_malloc.h"
#include "queue_safe_malloc.h"

V_PTR* safe_malloc(unsigned nbytes);
void safe_free(V_PTR* p);
void virt_free(V_PTR* p);
void real_free(V_PTR* p);
void initialize_vptr();

#endif