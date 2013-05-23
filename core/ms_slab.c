#include "ms_core.h"

#define MS_SLAB_PAGE_MASK   3
#define MS_SLAB_PAGE        0
#define MS_SLAB_BIG         1
#define MS_SLAB_EXACT       2
#define MS_SLAB_SMALL       3

#if (MS_PTR_SIZE == 4)

#define MS_SLAB_PAGE_FREE   0
#define MS_SLAB_PAGE_BUSY   0xffffffff
#define MS_SLAB_PAGE_START  0x80000000

#define MS_SLAB_SHIFT_MASK  0x0000000f
#define MS_SLAB_MAP_MASK    0xffff0000
#define MS_SLAB_MAP_SHIFT   16

#define MS_SLAB_BUSY        0xffffffff

#else /* (MS_PTR_SIZE == 8) */

#define MS_SLAB_PAGE_FREE   0
#define MS_SLAB_PAGE_BUSY   0xffffffffffffffff
#define MS_SLAB_PAGE_START  0x8000000000000000

#define MS_SLAB_SHIFT_MASK  0x000000000000000f
#define MS_SLAB_MAP_MASK    0xffffffff00000000
#define MS_SLAB_MAP_SHIFT   32

#define MS_SLAB_BUSY        0xffffffffffffffff

#endif


#if (MS_DEBUG_MALLOC)

#define ms_slab_junk(p, size)     ms_memset(p, 0xA5, size)

#else

#define ms_slab_junk(p, size)

#endif

static ms_slab_page_t *ms_slab_alloc_pages(ms_slab_pool_t *pool,
    ms_uint_t pages);
static void ms_slab_free_pages(ms_slab_pool_t *pool, ms_slab_page_t *page,
    ms_uint_t pages);

static ms_uint_t  ms_slab_max_size;
static ms_uint_t  ms_slab_exact_size;
static ms_uint_t  ms_slab_exact_shift;
static ms_uint_t  ms_pagesize;
static ms_uint_t  ms_pagesize_shift;

void
ms_slab_init(ms_slab_pool_t *pool)
{
    u_char           *p;
    size_t            size;
    ms_int_t         m;
    ms_uint_t        i, n, pages;
    ms_slab_page_t  *slots;

    assert(pool != NULL);

	/*pagesize*/
	ms_pagesize = getpagesize();
	for (n = ms_pagesize, ms_pagesize_shift = 0; 
			n >>= 1; ms_pagesize_shift++) { /* void */ }

    /* STUB */
    if (ms_slab_max_size == 0) {
        ms_slab_max_size = ms_pagesize / 2;
        ms_slab_exact_size = ms_pagesize / (8 * sizeof(uintptr_t));
        for (n = ms_slab_exact_size; n >>= 1; ms_slab_exact_shift++) {
            /* void */
        }
    }

    pool->min_size = 1 << pool->min_shift;

    p = (u_char *) pool + sizeof(ms_slab_pool_t);
    size = pool->end - p;

    ms_slab_junk(p, size);

    slots = (ms_slab_page_t *) p;
    n = ms_pagesize_shift - pool->min_shift;

    for (i = 0; i < n; i++) {
        slots[i].slab = 0;
        slots[i].next = &slots[i];
        slots[i].prev = 0;
    }

    p += n * sizeof(ms_slab_page_t);

    pages = (ms_uint_t) (size / (ms_pagesize + sizeof(ms_slab_page_t)));

    ms_memzero(p, pages * sizeof(ms_slab_page_t));

    pool->pages = (ms_slab_page_t *) p;

    pool->free.prev = 0;
    pool->free.next = (ms_slab_page_t *) p;

    pool->pages->slab = pages;
    pool->pages->next = &pool->free;
    pool->pages->prev = (uintptr_t) &pool->free;

    pool->start = (u_char *)
                  ms_align_ptr((uintptr_t) p + pages * sizeof(ms_slab_page_t),
                                 ms_pagesize);

    m = pages - (pool->end - pool->start) / ms_pagesize;
    if (m > 0) {
        pages -= m;
        pool->pages->slab = pages;
    }
}


void *
ms_slab_alloc(ms_slab_pool_t *pool, size_t size)
{
    void  *p;

    assert(pool != NULL);

    ms_shmtx_lock(&pool->mutex);

    p = ms_slab_alloc_locked(pool, size);

    ms_shmtx_unlock(&pool->mutex);

    return p;
}


void *
ms_slab_alloc_locked(ms_slab_pool_t *pool, size_t size)
{
    size_t            s;
    uintptr_t         p, n, m, mask, *bitmap;
    ms_uint_t        i, slot, shift, map;
    ms_slab_page_t  *page, *prev, *slots;

    assert(pool != NULL);

    if (size >= ms_slab_max_size) {

		ms_debug("slab alloc: %zu", size);

        page = ms_slab_alloc_pages(pool, (size >> ms_pagesize_shift)
                                          + ((size % ms_pagesize) ? 1 : 0));
        if (page) {
            p = (page - pool->pages) << ms_pagesize_shift;
            p += (uintptr_t) pool->start;

        } else {
            p = 0;
        }

        goto done;
    }

    if (size > pool->min_size) {
        shift = 1;
        for (s = size - 1; s >>= 1; shift++) { /* void */ }
        slot = shift - pool->min_shift;

    } else {
        size = pool->min_size;
        shift = pool->min_shift;
        slot = 0;
    }

    slots = (ms_slab_page_t *) ((u_char *) pool + sizeof(ms_slab_pool_t));
    page = slots[slot].next;

    if (page->next != page) {

        if (shift < ms_slab_exact_shift) {

            do {
                p = (page - pool->pages) << ms_pagesize_shift;
                bitmap = (uintptr_t *) (pool->start + p);

                map = (1 << (ms_pagesize_shift - shift))
                          / (sizeof(uintptr_t) * 8);

                for (n = 0; n < map; n++) {

                    if (bitmap[n] != MS_SLAB_BUSY) {

                        for (m = 1, i = 0; m; m <<= 1, i++) {
                            if ((bitmap[n] & m)) {
                                continue;
                            }

                            bitmap[n] |= m;

                            i = ((n * sizeof(uintptr_t) * 8) << shift)
                                + (i << shift);

                            if (bitmap[n] == MS_SLAB_BUSY) {
                                for (n = n + 1; n < map; n++) {
                                     if (bitmap[n] != MS_SLAB_BUSY) {
                                         p = (uintptr_t) bitmap + i;

                                         goto done;
                                     }
                                }

                                prev = (ms_slab_page_t *)
                                            (page->prev & ~MS_SLAB_PAGE_MASK);
                                prev->next = page->next;
                                page->next->prev = page->prev;

                                page->next = NULL;
                                page->prev = MS_SLAB_SMALL;
                            }

                            p = (uintptr_t) bitmap + i;

                            goto done;
                        }
                    }
                }

                page = page->next;

            } while (page);

        } else if (shift == ms_slab_exact_shift) {

            do {
                if (page->slab != MS_SLAB_BUSY) {

                    for (m = 1, i = 0; m; m <<= 1, i++) {
                        if ((page->slab & m)) {
                            continue;
                        }

                        page->slab |= m;
  
                        if (page->slab == MS_SLAB_BUSY) {
                            prev = (ms_slab_page_t *)
                                            (page->prev & ~MS_SLAB_PAGE_MASK);
                            prev->next = page->next;
                            page->next->prev = page->prev;

                            page->next = NULL;
                            page->prev = MS_SLAB_EXACT;
                        }

                        p = (page - pool->pages) << ms_pagesize_shift;
                        p += i << shift;
                        p += (uintptr_t) pool->start;

                        goto done;
                    }
                }

                page = page->next;

            } while (page);

        } else { /* shift > ms_slab_exact_shift */

            n = ms_pagesize_shift - (page->slab & MS_SLAB_SHIFT_MASK);
            n = 1 << n;
            n = ((uintptr_t) 1 << n) - 1;
            mask = n << MS_SLAB_MAP_SHIFT;
 
            do {
                if ((page->slab & MS_SLAB_MAP_MASK) != mask) {

                    for (m = (uintptr_t) 1 << MS_SLAB_MAP_SHIFT, i = 0;
                         m & mask;
                         m <<= 1, i++)
                    {
                        if ((page->slab & m)) {
                            continue;
                        }

                        page->slab |= m;

                        if ((page->slab & MS_SLAB_MAP_MASK) == mask) {
                            prev = (ms_slab_page_t *)
                                            (page->prev & ~MS_SLAB_PAGE_MASK);
                            prev->next = page->next;
                            page->next->prev = page->prev;

                            page->next = NULL;
                            page->prev = MS_SLAB_BIG;
                        }

                        p = (page - pool->pages) << ms_pagesize_shift;
                        p += i << shift;
                        p += (uintptr_t) pool->start;

                        goto done;
                    }
                }

                page = page->next;

            } while (page);
        }
    }

    page = ms_slab_alloc_pages(pool, 1);

    if (page) {
        if (shift < ms_slab_exact_shift) {
            p = (page - pool->pages) << ms_pagesize_shift;
            bitmap = (uintptr_t *) (pool->start + p);

            s = 1 << shift;
            n = (1 << (ms_pagesize_shift - shift)) / 8 / s;

            if (n == 0) {
                n = 1;
            }

            bitmap[0] = (2 << n) - 1;

            map = (1 << (ms_pagesize_shift - shift)) / (sizeof(uintptr_t) * 8);

            for (i = 1; i < map; i++) {
                bitmap[i] = 0;
            }

            page->slab = shift;
            page->next = &slots[slot];
            page->prev = (uintptr_t) &slots[slot] | MS_SLAB_SMALL;

            slots[slot].next = page;

            p = ((page - pool->pages) << ms_pagesize_shift) + s * n;
            p += (uintptr_t) pool->start;

            goto done;

        } else if (shift == ms_slab_exact_shift) {

            page->slab = 1;
            page->next = &slots[slot];
            page->prev = (uintptr_t) &slots[slot] | MS_SLAB_EXACT;

            slots[slot].next = page;

            p = (page - pool->pages) << ms_pagesize_shift;
            p += (uintptr_t) pool->start;

            goto done;

        } else { /* shift > ms_slab_exact_shift */

            page->slab = ((uintptr_t) 1 << MS_SLAB_MAP_SHIFT) | shift;
            page->next = &slots[slot];
            page->prev = (uintptr_t) &slots[slot] | MS_SLAB_BIG;

            slots[slot].next = page;

            p = (page - pool->pages) << ms_pagesize_shift;
            p += (uintptr_t) pool->start;

            goto done;
        }
    }

    p = 0;

done:

    ms_debug("slab alloc: %p", (void *)p);

    return (void *) p;
}


void
ms_slab_free(ms_slab_pool_t *pool, void *p)
{
    assert(pool != NULL && p != NULL);

    ms_shmtx_lock(&pool->mutex);

    ms_slab_free_locked(pool, p);

    ms_shmtx_unlock(&pool->mutex);
}


void
ms_slab_free_locked(ms_slab_pool_t *pool, void *p)
{
    size_t            size;
    uintptr_t         slab, m, *bitmap;
    ms_uint_t        n, type, slot, shift, map;
    ms_slab_page_t  *slots, *page;

    assert(pool != NULL && p != NULL);

    ms_debug("slab free: %p", p);

    if ((u_char *) p < pool->start || (u_char *) p > pool->end) {
        ms_error("ms_slab_free(): outside of pool");
        goto fail;
    }

    n = ((u_char *) p - pool->start) >> ms_pagesize_shift;
    page = &pool->pages[n];
    slab = page->slab;
    type = page->prev & MS_SLAB_PAGE_MASK;

    switch (type) {

    case MS_SLAB_SMALL:

        shift = slab & MS_SLAB_SHIFT_MASK;
        size = 1 << shift;

        if ((uintptr_t) p & (size - 1)) {
            goto wrong_chunk;
        }

        n = ((uintptr_t) p & (ms_pagesize - 1)) >> shift;
        m = (uintptr_t) 1 << (n & (sizeof(uintptr_t) * 8 - 1));
        n /= (sizeof(uintptr_t) * 8);
        bitmap = (uintptr_t *) ((uintptr_t) p & ~(ms_pagesize - 1));

        if (bitmap[n] & m) {

            if (page->next == NULL) {
                slots = (ms_slab_page_t *)
                                   ((u_char *) pool + sizeof(ms_slab_pool_t));
                slot = shift - pool->min_shift;

                page->next = slots[slot].next;
                slots[slot].next = page;

                page->prev = (uintptr_t) &slots[slot] | MS_SLAB_SMALL;
                page->next->prev = (uintptr_t) page | MS_SLAB_SMALL;
            }

            bitmap[n] &= ~m;

            n = (1 << (ms_pagesize_shift - shift)) / 8 / (1 << shift);

            if (n == 0) {
                n = 1;
            }

            if (bitmap[0] & ~(((uintptr_t) 1 << n) - 1)) {
                goto done;
            }

            map = (1 << (ms_pagesize_shift - shift)) / (sizeof(uintptr_t) * 8);

            for (n = 1; n < map; n++) {
                if (bitmap[n]) {
                    goto done;
                }
            }

            ms_slab_free_pages(pool, page, 1);

            goto done;
        }

        goto chunk_already_free;

    case MS_SLAB_EXACT:

        m = (uintptr_t) 1 <<
                (((uintptr_t) p & (ms_pagesize - 1)) >> ms_slab_exact_shift);
        size = ms_slab_exact_size;

        if ((uintptr_t) p & (size - 1)) {
            goto wrong_chunk;
        }

        if (slab & m) {
            if (slab == MS_SLAB_BUSY) {
                slots = (ms_slab_page_t *)
                                   ((u_char *) pool + sizeof(ms_slab_pool_t));
                slot = ms_slab_exact_shift - pool->min_shift;

                page->next = slots[slot].next;
                slots[slot].next = page;

                page->prev = (uintptr_t) &slots[slot] | MS_SLAB_EXACT;
                page->next->prev = (uintptr_t) page | MS_SLAB_EXACT;
            }

            page->slab &= ~m;

            if (page->slab) {
                goto done;
            }

            ms_slab_free_pages(pool, page, 1);

            goto done;
        }

        goto chunk_already_free;

    case MS_SLAB_BIG:

        shift = slab & MS_SLAB_SHIFT_MASK;
        size = 1 << shift;

        if ((uintptr_t) p & (size - 1)) {
            goto wrong_chunk;
        }

        m = (uintptr_t) 1 << ((((uintptr_t) p & (ms_pagesize - 1)) >> shift)
                              + MS_SLAB_MAP_SHIFT);

        if (slab & m) {

            if (page->next == NULL) {
                slots = (ms_slab_page_t *)
                                   ((u_char *) pool + sizeof(ms_slab_pool_t));
                slot = shift - pool->min_shift;

                page->next = slots[slot].next;
                slots[slot].next = page;

                page->prev = (uintptr_t) &slots[slot] | MS_SLAB_BIG;
                page->next->prev = (uintptr_t) page | MS_SLAB_BIG;
            }

            page->slab &= ~m;

            if (page->slab & MS_SLAB_MAP_MASK) {
                goto done;
            }

            ms_slab_free_pages(pool, page, 1);

            goto done;
        }

        goto chunk_already_free;

    case MS_SLAB_PAGE:

        if ((uintptr_t) p & (ms_pagesize - 1)) {
            goto wrong_chunk;
        }

		if (slab == MS_SLAB_PAGE_FREE) {
			ms_alert("ms_slab_free(): page is already free");
			goto fail;
        }

		if (slab == MS_SLAB_PAGE_BUSY) {
			ms_alert("ms_slab_free(): pointer to wrong page");
			goto fail;
        }

        n = ((u_char *) p - pool->start) >> ms_pagesize_shift;
        size = slab & ~MS_SLAB_PAGE_START;

        ms_slab_free_pages(pool, &pool->pages[n], size);

        ms_slab_junk(p, size << ms_pagesize_shift);

        return;
    }

    /* not reached */

    return;

done:

    ms_slab_junk(p, size);

    return;

wrong_chunk:

	ms_error("ms_slab_free(): pointer to wrong chunk");

    goto fail;

chunk_already_free:

	ms_error("ms_slab_free(): chunk is already free");

fail:

    return;
}


static ms_slab_page_t *
ms_slab_alloc_pages(ms_slab_pool_t *pool, ms_uint_t pages)
{
    ms_slab_page_t  *page, *p;

    assert(pool != NULL);

    for (page = pool->free.next; page != &pool->free; page = page->next) {

        if (page->slab >= pages) {

            if (page->slab > pages) {
                page[pages].slab = page->slab - pages;
                page[pages].next = page->next;
                page[pages].prev = page->prev;

                p = (ms_slab_page_t *) page->prev;
                p->next = &page[pages];
                page->next->prev = (uintptr_t) &page[pages];

            } else {
                p = (ms_slab_page_t *) page->prev;
                p->next = page->next;
                page->next->prev = page->prev;
            }

            page->slab = pages | MS_SLAB_PAGE_START;
            page->next = NULL;
            page->prev = MS_SLAB_PAGE;

            if (--pages == 0) {
                return page;
            }

            for (p = page + 1; pages; pages--) {
                p->slab = MS_SLAB_PAGE_BUSY;
                p->next = NULL;
                p->prev = MS_SLAB_PAGE;
                p++;
            }

            return page;
        }
    }

    ms_error("ms_slab_alloc() failed: no memory");

    return NULL;
}


static void
ms_slab_free_pages(ms_slab_pool_t *pool, ms_slab_page_t *page,
    ms_uint_t pages)
{
    ms_slab_page_t  *prev;

    assert(pool != NULL && page != NULL);

    page->slab = pages--;

    if (pages) {
        ms_memzero(&page[1], pages * sizeof(ms_slab_page_t));
    }

    if (page->next) {
        prev = (ms_slab_page_t *) (page->prev & ~MS_SLAB_PAGE_MASK);
        prev->next = page->next;
        page->next->prev = page->prev;
    }

    page->prev = (uintptr_t) &pool->free;
    page->next = pool->free.next;

    page->next->prev = (uintptr_t) page;

    pool->free.next = page;
}

void
ms_slab_dummy_init(ms_slab_pool_t *pool)
{
    ms_uint_t n;

    assert(pool != NULL);

	ms_pagesize = getpagesize();
	for (n = ms_pagesize, ms_pagesize_shift = 0; 
			n >>= 1; ms_pagesize_shift++) { /* void */ }

    if (ms_slab_max_size == 0) {
        ms_slab_max_size = ms_pagesize / 2;
        ms_slab_exact_size = ms_pagesize / (8 * sizeof(uintptr_t));
        for (n = ms_slab_exact_size; n >>= 1; ms_slab_exact_shift++) {
            /* void */
        }
    }
}

void
ms_slab_stat(ms_slab_pool_t *pool, ms_slab_stat_t *stat)
{
	uintptr_t 			m, n, mask, slab;
	uintptr_t 			*bitmap;
	ms_uint_t 			i, j, map, type, obj_size;
	ms_slab_page_t 	*page;

    assert(pool != NULL && stat != NULL);

	ms_memzero(stat, sizeof(ms_slab_stat_t));

	page = pool->pages;
 	stat->pages = (pool->end - pool->start) / ms_pagesize;;

	for (i = 0; i < stat->pages; i++)
	{
		slab = page->slab;
		type = page->prev & MS_SLAB_PAGE_MASK;

		switch (type) {

			case MS_SLAB_SMALL:
	
				n = (page - pool->pages) << ms_pagesize_shift;
                bitmap = (uintptr_t *) (pool->start + n);

				obj_size = 1 << slab;
                map = (1 << (ms_pagesize_shift - slab))
                          / (sizeof(uintptr_t) * 8);

				for (j = 0; j < map; j++) {
					for (m = 1 ; m; m <<= 1) {
						if ((bitmap[j] & m)) {
							stat->used_size += obj_size;
							stat->b_small   += obj_size;
						}

					}		
				}
	
				stat->p_small++;

				break;

			case MS_SLAB_EXACT:

				if (slab == MS_SLAB_BUSY) {
					stat->used_size += sizeof(uintptr_t) * 8 * ms_slab_exact_size;
					stat->b_exact   += sizeof(uintptr_t) * 8 * ms_slab_exact_size;
				}
				else {
					for (m = 1; m; m <<= 1) {
						if (slab & m) {
							stat->used_size += ms_slab_exact_size;
							stat->b_exact    += ms_slab_exact_size;
						}
					}
				}

				stat->p_exact++;

				break;

			case MS_SLAB_BIG:

				j = ms_pagesize_shift - (slab & MS_SLAB_SHIFT_MASK);
				j = 1 << j;
				j = ((uintptr_t) 1 << j) - 1;
				mask = j << MS_SLAB_MAP_SHIFT;
				obj_size = 1 << (slab & MS_SLAB_SHIFT_MASK);

				for (m = (uintptr_t) 1 << MS_SLAB_MAP_SHIFT; m & mask; m <<= 1)
				{
					if ((page->slab & m)) {
						stat->used_size += obj_size;
						stat->b_big     += obj_size;
					}
				}

				stat->p_big++;

				break;

			case MS_SLAB_PAGE:

				if (page->prev == MS_SLAB_PAGE) {		
					slab 			=  slab & ~MS_SLAB_PAGE_START;
					stat->used_size += slab * ms_pagesize;
					stat->b_page    += slab * ms_pagesize;
					stat->p_page    += slab;

					i += (slab - 1);

					break;
				}

			default:

				if (slab  > stat->max_free_pages) {
					stat->max_free_pages = page->slab;
				}

				stat->free_page += slab;

				i += (slab - 1);

				break;
		}

		page = pool->pages + i + 1;
	}

	stat->pool_size = pool->end - pool->start;
	stat->used_pct = stat->used_size * 100 / stat->pool_size;

	ms_info("pool_size : %zu bytes",	stat->pool_size);
	ms_info("used_size : %zu bytes",	stat->used_size);
	ms_info("used_pct  : %zu%%\n",		stat->used_pct);

	ms_info("total page count : %zu",	stat->pages);
	ms_info("free page count  : %zu\n",	stat->free_page);
		
	ms_info("small slab use page : %zu,\tbytes : %zu",	stat->p_small, stat->b_small);	
	ms_info("exact slab use page : %zu,\tbytes : %zu",	stat->p_exact, stat->b_exact);
	ms_info("big   slab use page : %zu,\tbytes : %zu",	stat->p_big,   stat->b_big);	
	ms_info("page slab use page  : %zu,\tbytes : %zu\n",	stat->p_page,  stat->b_page);				

	ms_info("max free pages : %zu\n",		stat->max_free_pages);
}
