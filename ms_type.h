#ifndef MS_TYPE_INC
#define MS_TYPE_INC

#include "ms_core.h"

typedef enum {   
	MY_TYPE_DECIMAL, 
	MY_TYPE_TINY,
	MY_TYPE_SHORT,  
	MY_TYPE_LONG = 3,
	MY_TYPE_FLOAT,  
	MY_TYPE_DOUBLE,
	MY_TYPE_NULL,
	MY_TYPE_TIMESTAMP = 7,
	MY_TYPE_LONGLONG,
	MY_TYPE_INT24,
	MY_TYPE_DATE = 10, 
	MY_TYPE_TIME = 11, 
	MY_TYPE_DATETIME = 12, 
	MY_TYPE_YEAR = 13, 
	MY_TYPE_NEWDATE = 14, 
	MY_TYPE_VARCHAR = 15, 
	MY_TYPE_BIT,
	MY_TYPE_NEWDECIMAL = 246,
	MY_TYPE_ENUM = 247,
	MY_TYPE_SET = 248,
	MY_TYPE_TINY_BLOB = 249,
	MY_TYPE_MEDIUM_BLOB = 250,
	MY_TYPE_LONG_BLOB = 251,
	MY_TYPE_BLOB = 252,
	MY_TYPE_VAR_STRING = 253,
	MY_TYPE_STRING = 254,
	MY_TYPE_GEOMETRY = 255 
} ms_coltype_e;
  
int calc_metadata_size(ms_coltype_e ftype);

int calc_field_size(ms_coltype_e ftype, 
		const uint8_t *pfield, 
		uint32_t metadata);

ms_str_t
tune_as_enum(ms_slab_pool_t *pool, ms_str_t *data, ms_array_t *meta);

ms_str_t
tune_as_set(ms_slab_pool_t *pool, ms_str_t *data, ms_array_t *meta);

ms_str_t
tune_as_year(ms_slab_pool_t *pool, ms_str_t *data, ms_array_t *meta);

ms_str_t 
tune_as_datetime(ms_slab_pool_t *pool, ms_str_t *data, ms_array_t *meta);

ms_str_t
tune_as_time(ms_slab_pool_t *pool, ms_str_t *data, ms_array_t *meta);

ms_str_t
tune_as_date(ms_slab_pool_t *pool, ms_str_t *data, ms_array_t *meta);

ms_str_t 
tune_as_timestamp(ms_slab_pool_t *pool, ms_str_t *data, ms_array_t *meta);

ms_str_t 
tune_as_uint(ms_slab_pool_t *pool, ms_str_t *data, ms_array_t *meta);

ms_str_t 
tune_as_int(ms_slab_pool_t *pool, ms_str_t *data, ms_array_t *meta);

ms_str_t 
tune_as_double(ms_slab_pool_t *pool, ms_str_t *data, ms_array_t *meta);
#endif
