#include <my_global.h>
#include <m_ctype.h>
#include <mysql.h>
#include <sql_common.h>
#include "ms_binlog.h"
#include "ms_event.h"
#include "ms_net.h"
#include "ms_qs.h"


uint64_t build_cols_mask(uint8_t **p, uint32_t *s, uint32_t n)
{
	uint64_t cols_mask;
	uint8_t l = (uint8_t)((n+7)/8);

	switch( l ) 
	{   
		case 1:
			cols_mask = 0xFFFFFFFFFFFFFFFFLL & (uint64_t)*(*p);
			break;
		case 2:
			cols_mask = 0xFFFFFFFFFFFFFFFFLL & (uint64_t)uint2korr(*p);
			break;
		case 3:
			cols_mask = 0xFFFFFFFFFFFFFFFFLL & (uint64_t)uint3korr(*p);
			break;
		case 4:
			cols_mask = 0xFFFFFFFFFFFFFFFFLL & (uint64_t)uint4korr(*p);
			break;
		case 5:
			cols_mask = 0xFFFFFFFFFFFFFFFFLL & (uint64_t)uint5korr(*p);
			break;
		case 6:
			cols_mask = 0xFFFFFFFFFFFFFFFFLL & (uint64_t)uint6korr(*p);
			break;
		default:
			return (uint64_t) - 1;
	}

	if (p && *p) (*p) += l;

	if (s) *s -= l;

	return cols_mask;
}
	
void
cols_pre_tune(ms_col_info_t *ci, uint8_t *data, uint32_t flen, uint32_t meta)
{
	ci->org_data.data = data;
	ci->org_data.len  = flen;
	ci->data.data	  = NULL;
	ci->data.len	  = 0;
	ci->is_null   = 0;

	switch (ci->type)
	{
		case MY_TYPE_STRING:
		{
			ms_coltype_e subtype = (ms_coltype_e)(meta & 0xFF);
			int len = meta >> 8;
			switch (subtype)
			{
				case MY_TYPE_STRING:
				{
					if (len > 255) {
						ci->org_data.data += 2;
						ci->org_data.len -= 2;
					}
					else {
						ci->org_data.data += 1;
						ci->org_data.len -= 1;
					}
					break;
				}
				case MY_TYPE_ENUM:
				case MY_TYPE_SET:
				{
					ci->type = subtype;
					ci->org_data.len = len;
					break;
				}
				default:
				{
					ci->is_null = 1; // invalid case
				}
			}

			ci->type = subtype;
			break;
		}                       

		case MY_TYPE_VARCHAR:
		{
			if (meta < 256)
			{
				ci->org_data.len = *ci->org_data.data;
				ci->org_data.data += 1;
			}
			else
			{
				ci->org_data.len = uint2korr(ci->org_data.data);
				ci->org_data.data += 2;
			}
			break;
		}
		case MY_TYPE_BLOB:
		{
			switch (meta)
			{
				case 1: 
					ci->org_data.len = *ci->org_data.data; break;
				case 2: 
					ci->org_data.len = uint2korr(ci->org_data.data); break;
				case 3: 
					ci->org_data.len = uint3korr(ci->org_data.data); break;
				case 4: 
					ci->org_data.len = uint4korr(ci->org_data.data); break;
				default: 
				{
					ci->org_data.len = 0; 
					ci->is_null = 1;
				}
			}

			if (ci->is_null == 0)
			{
				ci->org_data.data += meta;
			}
			break;
		}
		default:
		{
			/* void */
		}
	}           
}

void cols_tune(mysync_info_t *mi, ms_col_info_t *ci) 
{
	if (ci->is_null) {
		return;
	}

	switch (ci->type) {
		case MY_TYPE_ENUM:
		{
			ci->data = tune_as_enum(mi->pool, &ci->org_data, ci->meta);
			break;
		}

		case MY_TYPE_SET:
		{
			ci->data = tune_as_set(mi->pool, &ci->org_data, ci->meta);
			break;
		}

		case MY_TYPE_STRING:
		case MY_TYPE_VAR_STRING:
		case MY_TYPE_VARCHAR:
		case MY_TYPE_BLOB:
		{
			/* nothing to do, it is ok */
			break;
		}
		
		case MY_TYPE_YEAR:
		{
			ci->data = tune_as_year(mi->pool, &ci->org_data, NULL);
			break;
		}
		
		case MY_TYPE_DATETIME:
		{
			ci->data = tune_as_datetime(mi->pool, &ci->org_data, NULL);
			break;
		}

		case MY_TYPE_TIMESTAMP:
		{
			ci->data = tune_as_timestamp(mi->pool, &ci->org_data, NULL);
			break;
		}

		case MY_TYPE_TIME:
		{
			ci->data = tune_as_time(mi->pool, &ci->org_data, NULL);
			break;
		}

		case MY_TYPE_DATE:
		{
			ci->data = tune_as_date(mi->pool, &ci->org_data, NULL);
			break;
		}

		case MY_TYPE_BIT:
		{
			ci->data = tune_as_uint(mi->pool, &ci->org_data, NULL);
			break;
		}

		case MY_TYPE_DECIMAL:
		case MY_TYPE_TINY:
		case MY_TYPE_SHORT:
		case MY_TYPE_LONG:
		case MY_TYPE_LONGLONG:
		{
			ci->data = tune_as_int(mi->pool, &ci->org_data, NULL);
			break;
		}

		//case MY_TYPE_FLOAT:
		case MY_TYPE_DOUBLE:
		{
			ci->data = tune_as_double(mi->pool, &ci->org_data, NULL);
			break;
		}
		
		case MYSQL_TYPE_NEWDECIMAL:
		{
			/* TODO */
			break;
		}

		default:
		{
			/* un-supported type, seen as MY_TYPE_STRING */
			break;
		}
	}

	/* JUST FOR DEBUG */
	if (!ms_str_null(&ci->data)) {
		ms_debug("type:%u, len:%u. data:%s", 
				ci->type, ci->data.len, ci->data.data);
	}
	else {
		ms_debug("type:%u, len:%u. data:%s", 
				ci->type, ci->org_data.len, ci->org_data.data);
	}
}

void comm_header_parse(comm_header_t *h, ms_str_t *buf)
{
	uint8_t *data = buf->data;

	h->when = uint4korr(data); 
	h->type = uint4korr(data + EVENT_TYPE_OFFSET);
	h->server_id = uint4korr(data + SERVER_ID_OFFSET);
	h->data_written = uint4korr(data + EVENT_LEN_OFFSET);
	h->log_pos = uint4korr(data + LOG_POS_OFFSET);
	h->flags = uint2korr(data + FLAGS_OFFSET);

	buf->data = data + LOG_EVENT_HEADER_LEN;
	buf->len  = buf->len - LOG_EVENT_HEADER_LEN;
}

void rotate_event_parse(rotate_event_t *e, ms_str_t *buf)
{
	uint8_t *data = buf->data;

	e->pos = uint8korr(data);
	e->new_log = data + ROTATE_HEADER_LEN;

	buf->data = data + ROTATE_HEADER_LEN;
	buf->len  = buf->len - ROTATE_HEADER_LEN;
}

void table_map_event_parse(table_map_event_t *e, ms_str_t *buf)
{
	uint8_t *data = buf->data;

	e->table_id = (uint64_t)uint6korr(data + TM_MAPID_OFFSET);
	e->flags = (uint16_t)uint2korr(data + TM_MAPID_OFFSET + 2);
	data = data + TABLE_MAP_HEADER_LEN;

	e->db_name.len  = *data;
	e->db_name.data = data + 1;
	data = data + e->db_name.len + 2;

	e->table_name.len   = *data;
	e->table_name.data  = data + 1;
	data = data + e->table_name.len + 2;

	e->cols.len  = (uint32_t)net_field_length(&data);
	e->cols.data = data;
	data = data + e->cols.len;

	e->metadata.len = (uint32_t)net_field_length(&data); 
	e->metadata.data = e->metadata.len ? data : NULL;
}

bool table_map_event_verify(table_map_event_t *e)
{
	return (!ms_str_null(&e->db_name) &&
			!ms_str_null(&e->table_name) &&
			!ms_str_null(&e->cols));
}

void table_map_event_handler(mysync_info_t *mi, table_map_event_t *e)
{
	uint32_t i;
	uint8_t* type;
	uint8_t  dt[256];
	ms_table_info_t **oti, *ti;
	ms_col_info_t *ci;

	/* find table_info from hash table */
	i = ms_snprintf(dt, 256, "%s.%s", e->db_name.data, e->table_name.data);
	ti = ms_hash_find(mi->ti_hash, ms_hash_key_lc(dt, i), dt, i);
	if (ti == NULL) return;

	/*
	 * the struct of this table has been modified
	 */
	if (ms_array_n(ti->cols) != e->cols.len) 
	{
		for (i = 0; i < ms_array_n(ti->cols); i++) {
			ci = ms_array_get(ti->cols, i);
			ms_col_info_deinit(mi, ci);
		}

		ms_array_destroy(ti->cols);

		ti->cols = ms_array_create(mi->pool, 
				e->cols.len, sizeof(ms_col_info_t));

		if (sync_one_table_info(mi, ti)) return;
	}

	/* 
	 * metadata length is also the same per table-map-event.
	 * only if the struct of this table is modified.
	 */
	if (ti->meta.len != e->metadata.len) {
		ms_str_deinit(mi->pool, &ti->meta);	

		ti->meta.data = ms_palloc(mi->pool, e->metadata.len);
		if (ti->meta.data == NULL) {
			ms_error("ms_palloc failed, memory pool may be full");
			return;
		}
	}

	ti->meta.len = e->metadata.len;
	ms_memcpy(ti->meta.data, e->metadata.data, e->metadata.len);

	/* we update the col-type everytime */
	for (i = 0, type = e->cols.data; i < e->cols.len; ++i) 
	{   
		ci = (ms_col_info_t*)ms_array_get(ti->cols, i);
		ci->type = (ms_coltype_e) *type++;
	}   

	/* 
	 * because as the table id is not fixed,
	 * to avoid memory flow and simple to clean the old maps,
	 * array struct is better than  hash struct
	 */
	i = e->table_id % mi->ti_map_prime;
	oti = (ms_table_info_t**)ms_array_get2(mi->ti_map, i);

	if (*oti != ti) {
		*oti = ti;
	}
}

void write_rows_event_parse(write_rows_event_t *e, ms_str_t *buf)
{
	uint8_t *data = buf->data;

	e->table_id = (uint64_t)uint6korr(data + RW_MAPID_OFFSET);
	data = data + RW_FLAGS_OFFSET;

	e->flags = (uint16_t)uint2korr(data);
	data = data + 2;

	e->ncols = net_field_length(&data);

	e->cols_mask = build_cols_mask(&data, NULL, e->ncols);

	e->data.len = buf->len - (data - buf->data);
	e->data.data = data;
}

void update_rows_event_parse(update_rows_event_t *e, ms_str_t *buf)
{
	uint8_t *data = buf->data;

	e->table_id = (uint64_t)uint6korr(data + RW_MAPID_OFFSET);
	data = data + RW_FLAGS_OFFSET;

	e->flags = (uint16_t)uint2korr(data);
	data = data + 2;

	e->ncols = net_field_length(&data);

	e->pre_cols_mask = build_cols_mask(&data, NULL, e->ncols);
	e->post_cols_mask = build_cols_mask(&data, NULL, e->ncols);

	e->data.len = buf->len - (data - buf->data);
	e->data.data = data;
}

void delete_rows_event_parse(delete_rows_event_t *e, ms_str_t *buf)
{
	/* the same to write_rows_event */
	write_rows_event_parse((write_rows_event_t *)e, buf);
}

void
rows_event_handler(mysync_info_t *mi, void *event, int flag, ms_str_t *jmeta)
{
	uint32_t i, j, len, mt;
	uint32_t ftype, fsize;
	uint8_t *data, *pmeta;
   	uint64_t used_cols_mask, bit;
	ms_str_t jstr;
	ms_col_info_t *ci;
	ms_table_info_t *ti;
	rows_event_t *e;

	e = (rows_event_t*)event;
	
	ti = ms_table_info_map_get(mi, e->table_id);
	if (ti == NULL) return;

	data = e->data.data;
	len  = e->data.len;

	for (j = 1; len > 0; j++) 
	{
		/* 0 for used, 1 for null */
		used_cols_mask = ~build_cols_mask(&data, &len, e->ncols);	
		pmeta = ti->meta.data;

		for (i = 0, bit = 0x01; i < e->ncols; i++, bit <<= 1) {

			ci = ms_array_get(ti->cols, i);
			ci->is_null = 1;

			if (!ms_str_null(&ci->data)) {
				ms_str_deinit(mi->pool, &ci->data);
			}

			ftype = ci->type;
			switch (calc_metadata_size((ms_coltype_e)ftype)) {
				case 0:
				{
					mt = 0;
					break;
				}
				case 1:
				{
					mt = *pmeta++;
					break;
				}
				case 2:
				{
					mt = *(uint16_t*)pmeta;
					pmeta += 2;
					break;
				}
				default:
					mt = 0;
			}

			/* field is not null */
			if (!(used_cols_mask & bit)) {
				continue;
			}

			fsize = calc_field_size((ms_coltype_e)ftype, data, mt); 

			if (j & flag) {
				cols_pre_tune(ci, data, fsize, mt);
				cols_tune(mi, ci);
				ci->type = ftype;
			}

			data += fsize;
			len  -= fsize;
		}

		if (!(j & flag)) continue;

		/* serialization the row-data to json-string */
		jstr = conv_2_json(mi, ti, jmeta);
		if (ms_str_null(&jstr)) break;

		/* flush row-data to qs */
		flush_row_info(mi, &ti->dt, &jstr);

		ms_debug("json-str len %d, %s", jstr.len, jstr.data);
		ms_pfree(mi->pool, jstr.data);
	}
}

void 
write_rows_event_handler(mysync_info_t *mi, write_rows_event_t *e)
{
	ms_str_t jmeta;

	jmeta.data = (uint8_t*)"insert";
	jmeta.len  = sizeof("insert") - 1;

	rows_event_handler(mi, e, 0x03, &jmeta);	
}

void
update_rows_event_handler(mysync_info_t *mi, update_rows_event_t *e)
{
	ms_str_t jmeta;

	jmeta.data = (uint8_t*)"update";
	jmeta.len  = sizeof("update") - 1;

	rows_event_handler(mi, e, 0x02, &jmeta);	
}

void
delete_rows_event_handler(mysync_info_t *mi, delete_rows_event_t *e)
{
	ms_str_t jmeta;

	jmeta.data = (uint8_t*)"delete";
	jmeta.len  = sizeof("delete") - 1;

	rows_event_handler(mi, e, 0x03, &jmeta);	
}
