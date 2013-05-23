#include "ms_core.h"

inline void ms_array_null(ms_array_t *a)
{
    a->nelts = 0;
    a->elts = NULL;
    a->size = 0;
    a->nalloc = 0;
}

inline uint32_t ms_array_n(const ms_array_t *a)
{
    return a->nelts;
}

ms_array_t *ms_array_create(ms_slab_pool_t *p, uint32_t n, size_t size)
{
	ms_array_t *a; 

	assert(n != 0 && size != 0); 

	a = ms_palloc(p, sizeof(ms_array_t));
	if (a == NULL) {
		return NULL;
	}   

	a->elts = ms_palloc(p, n * size);

	if (a->elts == NULL) {
		return NULL;
	}   

	a->nelts = 0;
	a->size = size;
	a->nalloc = n;
	a->pool = p;

	return a;
}

void ms_array_destroy(ms_array_t *a) 
{
	if (a == NULL) 
		return;

	ms_array_deinit(a);
	ms_pfree(a->pool, a);
	a = NULL;
}

int ms_array_init(ms_array_t *a, uint32_t n, size_t size)
{
	assert(n != 0 && size != 0); 

	a->elts = ms_palloc(a->pool, n * size);
	if (a->elts == NULL) {
		return -1;
	}   

	a->nelts = 0;
	a->size = size;
	a->nalloc = n;

	return 0;
}

void ms_array_deinit(ms_array_t *a)
{
	if (a && a->elts != NULL) {
		ms_pfree(a->pool, a->elts);
		a->elts = NULL;
	}
}

void *ms_array_get(ms_array_t *a, uint32_t idx)
{
	void *elts;

	assert(a->nelts != 0); 
	assert(idx < a->nelts);

	elts = (uint8_t *)a->elts + (a->size * idx);

	return elts;
}

void **ms_array_get2(ms_array_t *a, uint32_t idx)
{
	assert(a->nelts != 0); 
	assert(idx < a->nelts);

	return (void**)((uint8_t*)a->elts + (a->size * idx));
}

void *ms_array_push(ms_array_t *a)
{
	void *elts, *new;
	size_t size;
	ms_slab_pool_t *p;

	if (a->nelts == a->nalloc) {

		/* the array is full; allocate new array */
		size = a->size * a->nalloc;
		p 	 = a->pool;
		new  = ms_palloc(p, 2 * size);
		if (new == NULL) {
			return NULL;
		}
		
		ms_memcpy(new, a->elts, size);
		ms_pfree(p, a->elts);

		a->elts = new;
		a->nalloc *= 2;
	}

	elts = (uint8_t *)a->elts + a->size * a->nelts;
	a->nelts++;

	return elts;
}

void *ms_array_pop(ms_array_t *a)
{
	void *elts;

	assert(a->nelts != 0);

	a->nelts--;
	elts = (uint8_t *)a->elts + a->size * a->nelts;

	return elts;
}
