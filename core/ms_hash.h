#ifndef MS_HASH_INC
#define MS_HASH_INC

#include "ms_core.h"

typedef uint32_t (*ms_hash_key_pt) (u_char *data, size_t len);

typedef struct {
    void             *value;
    u_short           len;
    u_char            name[1];
} ms_hash_elt_t;

typedef struct {
    ms_hash_elt_t  **buckets;
    uint32_t         size;
} ms_hash_t;

typedef struct {
    ms_str_t         key;
    uint32_t         key_hash;
    void             *value;
} ms_hash_key_t;


typedef struct {
    ms_hash_t       *hash;
    ms_hash_key_pt  key;

    uint32_t        max_size;
    uint32_t        bucket_size;

    char            *name;
    ms_slab_pool_t  *pool;
} ms_hash_init_t;


int
ms_hash_init(ms_hash_init_t *hinit, 
        ms_hash_key_t *names, uint32_t nelts);

void 
ms_hash_deinit(ms_hash_init_t *hinit);

void *
ms_hash_find(ms_hash_t *hash, 
        uint32_t key, u_char *name, size_t len);

/*
 * hash function 
 */
uint32_t 
ms_hash_key(u_char *data, size_t len);

uint32_t 
ms_hash_key_lc(u_char *data, size_t len);

uint32_t 
ms_hash_strlow(u_char *dst, u_char *src, size_t n);

#endif
