#ifndef _MS_SLAB_H_INCLUDED_
#define _MS_SLAB_H_INCLUDED_

#include "ms_core.h"

typedef struct ms_slab_page_s  ms_slab_page_t;

struct ms_slab_page_s {
    uintptr_t		slab;
    ms_slab_page_t	*next;
    uintptr_t       prev;
};

struct ms_slab_pool_s{
    size_t            min_size;
    size_t            min_shift;

    ms_slab_page_t  *pages;
    ms_slab_page_t   free;

    u_char           *start;
    u_char           *end;

	ms_shmtx_t		 mutex;

    void             *addr;
};

typedef struct {
	size_t 			pool_size, used_size, used_pct; 
	size_t			pages, free_page;
	size_t			p_small, p_exact, p_big, p_page; /* 四种slab占用的page数 */
	size_t			b_small, b_exact, b_big, b_page; /* 四种slab占用的byte数 */
	size_t			max_free_pages;					 /* 最大的连续可用page数 */
} ms_slab_stat_t;

#define ms_alloc(s)     malloc(s)
#define ms_free(p)      free(p)   
#define ms_palloc(p, s)	ms_slab_alloc(p, s)
#define ms_pfree(p, s)	ms_slab_free(p, s)

void ms_slab_init(ms_slab_pool_t *pool);
void *ms_slab_alloc(ms_slab_pool_t *pool, size_t size);
void *ms_slab_alloc_locked(ms_slab_pool_t *pool, size_t size);
void ms_slab_free(ms_slab_pool_t *pool, void *p);
void ms_slab_free_locked(ms_slab_pool_t *pool, void *p);

void ms_slab_dummy_init(ms_slab_pool_t *pool);
void ms_slab_stat(ms_slab_pool_t *pool, ms_slab_stat_t *stat);

#endif /* _MS_SLAB_H_INCLUDED_ */
