#ifndef  MS_ARRAY_INC
#define  MS_ARRAY_INC

#include "ms_core.h"

struct ms_array_s{
    void 	       *elts;
    uint32_t		nelts;	
    size_t      	size;
    uint32_t  		nalloc;
    ms_slab_pool_t  *pool;
};

inline void 
ms_array_null(ms_array_t *a);

inline uint32_t 
ms_array_n(const ms_array_t *a);

ms_array_t *ms_array_create(ms_slab_pool_t *p, uint32_t n, size_t size);
void ms_array_destroy(ms_array_t *a);

int ms_array_init(ms_array_t *a, uint32_t n, size_t size);
void ms_array_deinit(ms_array_t *a);
void *ms_array_get(ms_array_t *a, uint32_t idx);
void **ms_array_get2(ms_array_t *a, uint32_t idx);
void *ms_array_push(ms_array_t *a);
void *ms_array_pop(ms_array_t *a);

#endif 
