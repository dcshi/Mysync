#ifndef MS_STRING_INC
#define MS_STRING_INC

#include "ms_core.h"

struct ms_str_s{
    uint32_t len;   /* string length */
    uint8_t	 *data; /* string data */
};

#define ms_str(_str)   { sizeof(_str) - 1, (uint8_t *)(_str) }
#define ms_null_str    { 0, NULL }

#define ms_str_text(_str, _text) do {       \
	    (_str)->len = (uint32_t)(sizeof(_text) - 1);\
	    (_str)->data = (uint8_t *)(_text);          \
} while (0);

#define ms_str_raw(_str, _raw) do {         \
    (_str)->len = (uint32_t)(ms_strlen(_raw));  \
    (_str)->data = (uint8_t *)(_raw);           \
} while (0);

void ms_str_init(ms_str_t *str);
void ms_str_deinit(ms_slab_pool_t *pool, ms_str_t *str);
bool ms_str_null(const ms_str_t *str);

char *ms_strdup(ms_slab_pool_t *pool, const char *src);
char *ms_strndup(ms_slab_pool_t *pool, const char *src, size_t len);
int  ms_strcmp(const ms_str_t *s1, const ms_str_t *s2);

uint32_t ms_atou(const char *line, size_t n);

#define ms_tolower(c)  (u_char) ((c >= 'A' && c <= 'Z') ? (c | 0x20) : c)
#define ms_toupper(c)  (u_char) ((c >= 'a' && c <= 'z') ? (c & ~0x20) : c)
void ms_strlow(u_char *dst, u_char *src, size_t n);
/*
 * Wrapper around common routines for manipulating C character
 * strings
 */
#define ms_memcmp(_d, _c)				\
	memcmp(_d, _c, ms_strlen(_c))	

#define ms_memcpy(_d, _c, _n)			\
	memcpy(_d, _c, _n)

#define ms_memmove(_d, _c, _n)          \
    memmove(_d, _c, (size_t)(_n))

#define ms_memzero(buf, n)              \
    memset(buf, 0, n) 

#define ms_memset(buf, c, n)            \
    memset(buf, c, n)

#define ms_memchr(_d, _c, _n)           \
    memchr(_d, _c, (size_t)(_n))

#define ms_strlen(_s)                   \
    strlen((char *)(_s))

#define ms_strmsmp(_s1, _s2, _n)        \
    memcmp((char *)(_s1), (char *)(_s2), (size_t)(_n))

#define ms_strchr(_p, _l, _c)           \
    _ms_strchr((uint8_t *)(_p), (uint8_t *)(_l), (uint8_t)(_c))

#define ms_strrchr(_p, _s, _c)          \
    _ms_strrchr((uint8_t *)(_p),(uint8_t *)(_s), (uint8_t)(_c))

#define ms_snprintf(_s, _n, ...)        \
    snprintf((char *)(_s), (size_t)(_n), __VA_ARGS__)

#define ms_scnprintf(_s, _n, ...)       \
    _scnprintf((char *)(_s), (size_t)(_n), __VA_ARGS__)

#define ms_vscnprintf(_s, _n, _f, _a)   \
    _vscnprintf((char *)(_s), (size_t)(_n), _f, _a)

static inline uint8_t *
_ms_strchr(uint8_t *p, uint8_t *last, uint8_t c)
{
    while (p < last) {
        if (*p == c) {
            return p;
        }
        p++;
    }

    return NULL;
}

static inline uint8_t *
_ms_strrchr(uint8_t *p, uint8_t *start, uint8_t c)
{
    while (p >= start) {
        if (*p == c) {
            return p;
        }
        p--;
    }

    return NULL;
}

#endif
