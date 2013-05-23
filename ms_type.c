#include <my_global.h>
#include <m_ctype.h>
#include <mysql.h>
#include <sql_common.h>
#include "ms_type.h"

int calc_metadata_size(ms_coltype_e ftype)
{
	int rc;
	switch( ftype ) 
	{
	case MY_TYPE_FLOAT:
	case MY_TYPE_DOUBLE:
	case MY_TYPE_BLOB:
	case MY_TYPE_GEOMETRY:
		rc = 1;
		break;
	case MY_TYPE_VARCHAR:
	case MY_TYPE_BIT:
	case MY_TYPE_NEWDECIMAL:
	case MY_TYPE_VAR_STRING:
	case MY_TYPE_STRING:
		rc = 2;
		break;
	default:
		rc = 0;
	}
	return rc;
}
	
int calc_field_size(ms_coltype_e ftype, 
		const uint8_t *pfield, uint32_t metadata)
{
	uint32_t length;

	switch (ftype) {
	case MY_TYPE_VAR_STRING:
	/* This type is hijacked for result set types. */
		length= metadata;
		break;
	case MY_TYPE_NEWDECIMAL:
	{
		static const int dig2bytes[10] = {0, 1, 1, 2, 2, 3, 3, 4, 4, 4};
		int precision = (int)(metadata & 0xff);
		int scale = (int)(metadata >> 8);
		int intg = precision - scale;
		int intg0 = intg / 9;
		int frac0 = scale / 9;
		int intg0x = intg - intg0*9;
		int frac0x = scale - frac0*9;
		length = intg0 * sizeof(int32_t) + dig2bytes[intg0x] + frac0 * sizeof(int32_t) + dig2bytes[frac0x];
		break;
	}
	case MY_TYPE_DECIMAL:
	case MY_TYPE_FLOAT:
	case MY_TYPE_DOUBLE:
		length = metadata;
		break;
	/*
	The cases for SET and ENUM are include for completeness, however
	both are mapped to type MY_TYPE_STRING and their real types
	are encoded in the field metadata.
	*/
	case MY_TYPE_SET:
	case MY_TYPE_ENUM:
	case MY_TYPE_STRING:
	{
		ms_coltype_e type = (ms_coltype_e)(metadata & 0xFF);
		int len = metadata >> 8;
		if ((type == MY_TYPE_SET) || (type == MY_TYPE_ENUM))
		{
			length = len;
		}
		else
		{
			length = len < 256 ? (unsigned int) *pfield + 1 : uint2korr(pfield);
		}
		break;
	}
	case MY_TYPE_YEAR:
	case MY_TYPE_TINY:
		length = 1;
		break;
	case MY_TYPE_SHORT:
		length = 2;
		break;
	case MY_TYPE_INT24:
		length = 3;
		break;
	case MY_TYPE_LONG:
		length = 4;
		break;
	case MY_TYPE_LONGLONG:
	    length= 8;
	    break;
	case MY_TYPE_NULL:
		length = 0;
		break;
	case MY_TYPE_NEWDATE:
	case MY_TYPE_DATE:
	case MY_TYPE_TIME:
		length = 3;
		break;
	case MY_TYPE_TIMESTAMP:
		length = 4;
		break;
	case MY_TYPE_DATETIME:
	length = 8;
		break;
	case MY_TYPE_BIT:
	{
		/*
		Decode the size of the bit field from the master.
		from_len is the length in bytes from the master
		from_bit_len is the number of extra bits stored in the master record
		If from_bit_len is not 0, add 1 to the length to account for accurate
		number of bytes needed.
		*/
		uint32_t from_len = (metadata >> 8U) & 0x00ff;
		uint32_t from_bit_len = metadata & 0x00ff;
		length = from_len + ((from_bit_len > 0) ? 1 : 0);
		break;
	}
	case MY_TYPE_VARCHAR:
	{
		length = metadata > 255 ? 2 : 1;
		length += length == 1 ? (uint32_t)*pfield : *((uint16_t *)pfield);
		break;
	}
	case MY_TYPE_TINY_BLOB:
	case MY_TYPE_MEDIUM_BLOB:
	case MY_TYPE_LONG_BLOB:
	case MY_TYPE_BLOB:
	case MY_TYPE_GEOMETRY:
	{
		switch (metadata)
		{
			case 1:
				length = 1 + (uint32_t) pfield[0];
				break;
			case 2:
				length = 2 + (uint32_t) (*(uint16_t*)(pfield) & 0xFFFF);
				break;
			case 3:
				length = 3 + (uint32_t) (long) (*((uint32_t *) (pfield)) & 0xFFFFFF);
				break;
			case 4:
				length = 4 + (uint32_t) (long) *((uint32_t *) (pfield));
				break;
			default:
				length= 0;
				break;
		}
		break;
	}
	default:
		length = ~(uint32_t) 0;
	}
	
	return length;
}

ms_str_t
tune_as_enum(ms_slab_pool_t *pool, ms_str_t *data, ms_array_t *meta)
{
	uint32_t n;
	ms_str_t s, *m;

	ms_str_init(&s);

	switch (data->len)
	{   
		case 1:
			n = *data->data;	
			break;
		case 2: 
			n = uint2korr(data->data); 
			break;
		case 3: 
			n = uint3korr(data->data); 
			break;
		case 4: 
			n = uint4korr(data->data); 
			break;
	}

	m = ms_array_get(meta, n - 1);
	if (m == NULL) {
		return s;
	}

	s.len = m->len;
	s.data = (uint8_t*)ms_strndup(pool, (char*)m->data, m->len);

	return s;
}

ms_str_t 
tune_as_set(ms_slab_pool_t *pool, ms_str_t *data, ms_array_t *meta)
{
	uint32_t i, n, len;
	uint32_t bit;
	uint8_t	 *p;
	ms_str_t s, *m;

	ms_str_init(&s);

	switch (data->len)
	{   
		case 1:
			n = *data->data;	
			break;
		case 2: 
			n = uint2korr(data->data); 
			break;
		case 3: 
			n = uint3korr(data->data); 
			break;
		case 4: 
			n = uint4korr(data->data); 
			break;
	}

	if (n < 0) {
		return s;
	}

	i = 0;
	len = 0;
	bit = 0x01;

	/* clac the set-data len */
	while ((bit <= n)) 
	{
		if ((n & bit) && i < ms_array_n(meta)) {
			
			m = ms_array_get(meta, i);
			if (m == NULL) {
				return s;
			}
	
			len += m->len;
			len += 1;	  	/* for , */
		}

		bit <<= 1;
		i++;
	}

	s.len = len - 1;
	s.data = ms_palloc(pool, len);

	i = 0;
	bit = 0x01;
	p = s.data;

	while ((bit <= n)) 
	{
		if ((n & bit)) {
			
			m = ms_array_get(meta, i);

			ms_memcpy(p, m->data, m->len);	
			p += m->len;

			*p++ = ',';
		}

		bit <<= 1;
		i++;
	}
	
	/* trim the last , */
	*(--p) = '\0';

	return s;
}


ms_str_t
tune_as_year(ms_slab_pool_t *pool, ms_str_t *data, ms_array_t *meta)
{
	uint32_t year;
	ms_str_t s;

	year = *data->data + 1900;

	s.len = 4;
	s.data = ms_palloc(pool, s.len+1);

	ms_snprintf(s.data, s.len+1, "%u", year);

	return s;
}

ms_str_t 
tune_as_datetime(ms_slab_pool_t *pool, ms_str_t *data, ms_array_t *meta)
{
	uint64_t dt;
	int d1, d2;
	int y, m, d;
	int H, M, S;
	ms_str_t s;

	dt = uint8korr(data->data);
	d1 = dt / 1000000;
	d2 = dt % 1000000;

	y = d1 / 10000;
	m = (d1 % 10000) / 100;
	d = d1 %100;

	H = d2 / 10000;
	M = (d2 % 10000) / 100;
	S = d2 % 100;

	s.len = 19;
	s.data = ms_palloc(pool, s.len+1);

	ms_snprintf(s.data, s.len+1, "%04d-%02d-%02d %02d:%02d:%02d",
			y, m, d, H, M, S);

	return s;
}

ms_str_t
tune_as_time(ms_slab_pool_t *pool, ms_str_t *data, ms_array_t *meta)
{
	uint32_t t;
	int H, M, S;
	ms_str_t s;

	t = uint3korr(data->data);

	H = t / 10000;
	M = (t % 10000) / 100;
	S = t % 100;

	s.len = 8;
	s.data = ms_palloc(pool, s.len+1);

	ms_snprintf(s.data, s.len+1, "%02d:%02d:%02d", H, M, S);

	return s;
}

ms_str_t
tune_as_date(ms_slab_pool_t *pool, ms_str_t *data, ms_array_t *meta)
{
	uint32_t t;
	int y, m, d;
	ms_str_t s;

	t = uint3korr(data->data);

	y = t / (16L * 32L);
	m = t / 32L % 16L;
	d = t % 32L;

	s.len = 10;
	s.data = ms_palloc(pool, s.len+1);

	ms_snprintf(s.data, s.len+1, "%04d-%02d-%02d", y, m, d);

	return s;
}

ms_str_t
tune_as_timestamp(ms_slab_pool_t *pool, ms_str_t *data, ms_array_t *meta)
{
	uint32_t t;
	ms_str_t s;

	t = uint4korr(data->data);

	s.len = 10;
	s.data = ms_palloc(pool, s.len+1);

	ms_snprintf(s.data, s.len+1, "%010d", t);

	return s;
}

ms_str_t
tune_as_uint(ms_slab_pool_t *pool, ms_str_t *data, ms_array_t *meta)
{
	uint64_t u;
	ms_str_t s;

	switch (data->len)
	{   
		case 1:
			u = *data->data;	
			break;
		case 2: 
			u = uint2korr(data->data); 
			break;
		case 3: 
			u = uint3korr(data->data); 
			break;
		case 4: 
			u = uint4korr(data->data); 
			break;
		case 6:
			u = uint8korr(data->data);
			break;
		default:
			u = 0;
			break;
	}

	s.len = 8;
	if (u > 0xFFFF) {
		s.len = 16;
	}
	else if (u > 0xFFFFFFFF) {
		s.len = 32;
	}

	s.data = ms_palloc(pool, s.len);
	s.len = ms_snprintf(s.data, s.len, "%"PRIu64, u);

	return  s;
}

ms_str_t
tune_as_int(ms_slab_pool_t *pool, ms_str_t *data, ms_array_t *meta)
{
	int64_t  i;
	ms_str_t s;

	switch (data->len)
	{   
		case 1:
			i = *data->data;	
			break;
		case 2: 
			i = uint2korr(data->data); 
			break;
		case 3: 
			i = uint3korr(data->data); 
			break;
		case 4: 
			i = uint4korr(data->data); 
			break;
		case 6:
			i = uint8korr(data->data);
			break;
		default:
			i = 0;
			break;
	}

	s.len = 8;
	if (llabs(i) > 0xFFFF) {
		s.len = 16;
	}
	else if (llabs(i) > 0xFFFFFFFF) {
		s.len = 32;
	}

	s.data = ms_palloc(pool, s.len);
	s.len = ms_snprintf(s.data, s.len, "%"PRIu64, i);

	return  s;
}

ms_str_t
tune_as_double(ms_slab_pool_t *pool, ms_str_t *data, ms_array_t *meta)
{
	double d;
	ms_str_t s;

	d = 0;

	if (data->len == 4) {
#ifdef WORDS_BIGENDIAN
		float4get(d, data->data);
#else
		ms_memcpy(&d, data->data, sizeof(float));
#endif
	}
	else if (data->len == 8) {
#ifdef WORDS_BIGENDIAN
		float8get(d, data->data);
#else
		ms_memcpy(&d, data->data, sizeof(double));
#endif
	}

	s.len = 32;
	s.data = ms_palloc(pool, s.len);
	s.len = ms_snprintf(s.data, s.len, "%f", d);

	return s;
}
