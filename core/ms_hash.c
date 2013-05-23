#include "ms_hash.h"

/* todo: OS based */
#define ms_cacheline_size  1

#define ms_hash(key, c)   ((uint32_t) key * 31 + c)

#define MS_HASH_ELT_SIZE(name)  \
    (sizeof(void *) + ms_align((name)->key.len + 2, sizeof(void *)))

int
ms_hash_init(ms_hash_init_t *hinit, ms_hash_key_t *names, uint32_t nelts)
{
    u_char			*elts;
    size_t          len;
    u_short         *test;
    uint32_t       	i, n, key, size, start, bucket_size;
    ms_hash_elt_t  	*elt, **buckets;

    for (n = 0; n < nelts; n++) {
        if (hinit->bucket_size < 
				MS_HASH_ELT_SIZE(&names[n]) + sizeof(void *)) 
		{
			ms_error("could not build the %s, you should "
					"increase %s_bucket_size: %i",
					hinit->name, hinit->name, hinit->bucket_size);
			return -1;
		}
    }

    test = ms_alloc(hinit->max_size * sizeof(u_short));
    if (test == NULL) {
        return -1;
    }

	/* the las void* indicates the end of a bucket */
    bucket_size = hinit->bucket_size - sizeof(void *);

    start = nelts / (bucket_size / (2 * sizeof(void *)));
    start = start ? start : 1;

    if (hinit->max_size > 10000 && nelts && hinit->max_size / nelts < 100) {
        start = hinit->max_size - 1000;
    }

    for (size = start; size < hinit->max_size; size++) {

        ms_memzero(test, size * sizeof(u_short));

        for (n = 0; n < nelts; n++) {
            if (names[n].key.data == NULL) {
                continue;
            }

            key = names[n].key_hash % size;
            test[key] = (u_short) (test[key] + MS_HASH_ELT_SIZE(&names[n]));

            if (test[key] > (u_short) bucket_size) {
                goto next;
            }
        }

        goto found;

    next:

        continue;
    }

	ms_error("could not build the %s, you should increase "
			"either %s_max_size: %i or %s_bucket_size: %i",
			hinit->name, hinit->name, hinit->max_size,
			hinit->name, hinit->bucket_size);

    ms_free(test);

    return -1;

found:

    for (i = 0; i < size; i++) {
        test[i] = sizeof(void *);
    }

    for (n = 0; n < nelts; n++) {
        if (names[n].key.data == NULL) {
            continue;
        }

        key = names[n].key_hash % size;
        test[key] = (u_short) (test[key] + MS_HASH_ELT_SIZE(&names[n]));
    }

    len = 0;

    for (i = 0; i < size; i++) {
        if (test[i] == sizeof(void *)) {
            continue;
        }
      
		/* bucket boundary alignment */
        test[i] = (u_short) (ms_align(test[i], ms_cacheline_size));

        len += test[i];
    }

	buckets = ms_palloc(hinit->pool, size * sizeof(ms_hash_elt_t *));
	if (buckets == NULL) {
		ms_free(test);

		ms_error("ms_palloc fail, make sure the memory pool size is enough");
		return -1;
	}


    elts = ms_palloc(hinit->pool, len + ms_cacheline_size);
    if (elts == NULL) {
        ms_free(test);
		
		ms_error("ms_palloc fail, make sure the memory pool size is enough");
        return -1;
    }

    elts = ms_align_ptr(elts, ms_cacheline_size);

    for (i = 0; i < size; i++) {
        if (test[i] == sizeof(void *)) {
            continue;
        }

        buckets[i] = (ms_hash_elt_t *) elts;
        elts += test[i];

    }

    for (i = 0; i < size; i++) {
        test[i] = 0;
    }

    for (n = 0; n < nelts; n++) {
        if (names[n].key.data == NULL) {
            continue;
        }

        key = names[n].key_hash % size;
        elt = (ms_hash_elt_t *) ((u_char *) buckets[key] + test[key]);

        elt->value = names[n].value;
        elt->len = (u_short) names[n].key.len;

        ms_strlow(elt->name, names[n].key.data, names[n].key.len);

        test[key] = (u_short) (test[key] + MS_HASH_ELT_SIZE(&names[n]));
    }

    for (i = 0; i < size; i++) {
        if (buckets[i] == NULL) {
            continue;
        }

        elt = (ms_hash_elt_t *) ((u_char *) buckets[i] + test[i]);

        elt->value = NULL;
    }

    ms_free(test);

	if (hinit->hash == NULL) {
		hinit->hash = ms_palloc(hinit->pool, sizeof(ms_hash_t));

		if (hinit->hash == NULL) {
			ms_error("ms_palloc fail, make sure the memory pool size is enough");
			return -1;
		}
	}

	hinit->hash->buckets = buckets;
	hinit->hash->size = size;

	return 0;
}

void 
ms_hash_deinit(ms_hash_init_t *hinit)
{
    if (hinit == NULL || hinit->hash == NULL || hinit->pool == NULL) {
        return;
    }

    ms_pfree(hinit->pool, hinit->hash);

    hinit->hash = NULL;
    hinit->key  = NULL;
    hinit->name = NULL;
    hinit->pool = NULL;
}

void *
ms_hash_find(ms_hash_t *hash, uint32_t key, u_char *name, size_t len)
{
    uint32_t       i;
    ms_hash_elt_t  *elt;

    elt = hash->buckets[key % hash->size];

    if (elt == NULL) {
        return NULL;
    }

    while (elt->value) {
        if (len != (size_t) elt->len) {
            goto next;
        }

        for (i = 0; i < len; i++) {
            if (name[i] != elt->name[i]) {
                goto next;
            }
        }

        return elt->value;

    next:

        elt = (ms_hash_elt_t *) ms_align_ptr(&elt->name[0] + elt->len,
                                               sizeof(void *));
        continue;
    }

    return NULL;
}

uint32_t
ms_hash_key(u_char *data, size_t len)
{
    uint32_t  i, key;

    key = 0;

    for (i = 0; i < len; i++) {
        key = ms_hash(key, data[i]);
    }

    return key;
}


uint32_t
ms_hash_key_lc(u_char *data, size_t len)
{
    uint32_t  i, key;

    key = 0;

    for (i = 0; i < len; i++) {
        key = ms_hash(key, ms_tolower(data[i]));
    }

    return key;
}


uint32_t
ms_hash_strlow(u_char *dst, u_char *src, size_t n)
{
    uint32_t  key;

    key = 0;

    while (n--) {
        *dst = ms_tolower(*src);
        key = ms_hash(key, *dst);
        dst++;
        src++;
    }

    return key;
}

