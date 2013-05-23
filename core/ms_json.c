#include "ms_json.h"

int ms_json_enstring_len(ms_str_t *src)
{
	int		 len;
	uint32_t n;
	uint8_t *p;

	len = 2;
	n = src->len;
	p = src->data;

	while (n-- > 0) {

		if (strchr("\"\\\b\f\n\r\t", *p)) {
			len++;
		}
		else if (*p < 32) {
			len += 5;
		}
		
		p++;
		len++;
	}

	return len;
}

int ms_json_enstring(uint8_t **dst, ms_str_t *src)
{
	uint32_t  n;
	uint8_t  *ps, *pd;

	if (dst == NULL || src == NULL) {
		return -1;
	}

	n  = src->len;
	ps = src->data;
	pd = *dst;

	*(pd++) = '"';
	while (n-- > 0) {

		if (*ps > 31 && *ps != '\"' && *ps !='\\' ) {
			*(pd++) = *ps++;
			continue;
		}
		
		*(pd++) = '\\';
		switch (*ps) {
			case '\\':
				*(pd++) = '\\';
				break;
			case '\"':
				*(pd++) = '"';
				break;
			case '\b':
				*(pd++) = 'b';
				break;
			case '\f':
				*(pd++) = 'f';
				break;
			case '\n':
				*(pd++) = 'n';
				break;
			case '\r':
				*(pd++) = 'r';
				break;
			case '\t':
				*(pd++) = 't';
				break;
			default :
				ms_snprintf(pd, 5, "u%04x", *ps); 	/* escape and print */
				pd += 5;
				break;
		}

		ps++;
	}

	*(pd++) = '"';
	*dst = pd;

	return 0;
}
