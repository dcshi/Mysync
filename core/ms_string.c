#include "ms_core.h"
#include "ms_slab.h"

void ms_str_init(ms_str_t *str)
{
    str->len = 0;
    str->data = NULL;
}

void ms_str_deinit(ms_slab_pool_t *pool, ms_str_t *str)
{
    assert((str->len == 0 && str->data == NULL) ||
           (str->len != 0 && str->data != NULL));

    if (str->data != NULL) {
        ms_pfree(pool, str->data);
        ms_str_init(str);
    }
}

bool ms_str_null(const ms_str_t *str)
{
    assert((str->len == 0 && str->data == NULL) ||
           (str->len != 0 && str->data != NULL));

    return str->len == 0 ? true : false;
}

char *ms_strdup(ms_slab_pool_t *pool, const char *src)
{
	char *dst;
	size_t	l = ms_strlen(src) + 1;

    assert(src != NULL);

	dst = ms_slab_alloc(pool, l);
	if (dst == NULL) {
		return NULL;
	}

	ms_memcpy(dst, src, l);
	dst[l] = '\0';

    return dst;
}

char *ms_strndup(ms_slab_pool_t *pool, const char *src, size_t len)
{
	char *dst;

    assert(len != 0 && src != NULL);

	dst = ms_slab_alloc(pool, len + 1);
	if (dst == NULL) {
		return NULL;
	}

	ms_memcpy(dst, src, len);
	dst[len] = '\0';

    return dst;
}
int ms_strcmp(const ms_str_t *s1, const ms_str_t *s2)
{
    if (s1->len != s2->len) {
        return s1->len - s2->len > 0 ? 1 : -1;
    }

    return ms_strmsmp(s1->data, s2->data, s1->len);
}

uint32_t ms_atou(const char *line, size_t n)
{
	uint32_t value;

	if (n == 0) {
		return 0;
	}   

	for (value = 0; n--; line++) {
		if (*line < '0' || *line > '9') {
			return 0; 
		}   

		value = value * 10 + (*line - '0');
	}   

	if (value < 0) {
		return 0; 
	}   

	return value;
}

void
ms_strlow(u_char *dst, u_char *src, size_t n)
{
	while (n) {
		*dst = ms_tolower(*src);
		dst++;
		src++;
		n--; 
	}    
}
