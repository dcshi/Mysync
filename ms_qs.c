#include <my_global.h>
#include <m_ctype.h>
#include <mysql.h>
#include <sql_common.h>
#include <tcbdb.h> 
#include "ms_qs.h"
#include "ms_json.h"

static TCBDB *tdb;

/* 读取队列写入点的值 */
static int httpsqs_read_putpos(const char* httpsqs_input_name)
{
	int queue_value = 0;
	char *queue_value_tmp;
	char queue_name[300] = {0}; 
	
	sprintf(queue_name, "%s:%s", httpsqs_input_name, "putpos");
	
	queue_value_tmp = tcbdbget2(tdb, queue_name);
	if(queue_value_tmp){
		queue_value = atoi(queue_value_tmp);
		ms_free(queue_value_tmp);
	}
	
	return queue_value;
}

/* 读取队列读取点的值 */
static int httpsqs_read_getpos(const char* httpsqs_input_name)
{
	int queue_value = 0;
	char *queue_value_tmp;
	char queue_name[300] = {0}; 
	
	sprintf(queue_name, "%s:%s", httpsqs_input_name, "getpos");
	
	queue_value_tmp = tcbdbget2(tdb, queue_name);
	if(queue_value_tmp){
		queue_value = atoi(queue_value_tmp);
		ms_free(queue_value_tmp);
	}
	
	return queue_value;
}

/* 读取用于设置的最大队列数 */
static int httpsqs_read_maxqueue(const char* httpsqs_input_name)
{
#ifndef HTTPSQS_DEFAULT_MAXQUEUE
#define HTTPSQS_DEFAULT_MAXQUEUE 1000000
#endif
	int queue_value = 0;
	char *queue_value_tmp;
	char queue_name[300] = {0}; 
	
	sprintf(queue_name, "%s:%s", httpsqs_input_name, "maxqueue");
	
	queue_value_tmp = tcbdbget2(tdb, queue_name);
	if(queue_value_tmp){
		queue_value = atoi(queue_value_tmp);
		ms_free(queue_value_tmp);
	} else {
		queue_value = HTTPSQS_DEFAULT_MAXQUEUE; /* 默认队列长度 */
	}
	
	return queue_value;
}

/* 获取本次“入队列”操作的队列写入点 */
static int httpsqs_now_putpos(const char* httpsqs_input_name)
{
	int maxqueue_num = 0;
	int queue_put_value = 0;
	int queue_get_value = 0;
	char queue_name[300] = {0}; 
	char queue_input[32] = {0};
	
	/* 获取最大队列数量 */
	maxqueue_num = httpsqs_read_maxqueue(httpsqs_input_name);
	
	/* 读取当前队列写入位置点 */
	queue_put_value = httpsqs_read_putpos(httpsqs_input_name);
	
	/* 读取当前队列读取位置点 */
	queue_get_value = httpsqs_read_getpos(httpsqs_input_name);	
	
	sprintf(queue_name, "%s:%s", httpsqs_input_name, "putpos");	
	
	/* 队列写入位置点加1 */
	queue_put_value = queue_put_value + 1;

	/* 如果队列写入ID+1之后追上队列读取ID，则说明队列已满，返回0，拒绝继续写入 */
	if (queue_put_value == queue_get_value) {
		queue_put_value = 0;
	}
	/* 如果队列写入ID大于最大队列数量，并且从未进行过出队列操作（=0）或进行过1次出队列操作（=1），返回0，拒绝继续写入 */
	else if (queue_get_value <= 1 && queue_put_value > maxqueue_num) { 
		queue_put_value = 0;
	}	

	/* 如果队列写入ID大于最大队列数量，则重置队列写入位置点的值为1 */
	else if (queue_put_value > maxqueue_num) {

		if(tcbdbput2(tdb, queue_name, "1")) {
			queue_put_value = 1;
		}
	} else { /* 队列写入位置点加1后的值，回写入数据库 */
		sprintf(queue_input, "%d", queue_put_value);
		tcbdbput2(tdb, queue_name, (char *)queue_input);
	}
	
	return queue_put_value;
}

int qs_db_init(mysync_info_t *mi)
{
	int err;

	tdb = tcbdbnew(); 

	/*
	tcbdbsetmutex(tdb); 
	tcbdbtune(tdb, 1024, 2048, 50000000, 8, 10, BDBTLARGE);
	tcbdbsetcache(tdb, 2048, httpsqs_settings_cachenonleaf);
	tcbdbsetxmsiz(tdb, 1024*1024*5); // 内存缓存大小
	*/

	if(!tcbdbopen(tdb, mi->qs_db, BDBOWRITER|BDBOCREAT|BDBONOLCK)){
		err = tcbdbecode(tdb);
		ms_error("open tdb failed: %s", tcbdberrmsg(err));
		return -1;
	}

    return 0;
}

int qs_db_deinit(mysync_info_t *mi)
{
	int err;

	if(!tcbdbclose(tdb)){
		err = tcbdbecode(tdb);
		ms_error("close tdb failed: %s\n", tcbdberrmsg(err));
		return -1;
	}

	tcbdbdel(tdb); 

	return 0;
}

void flush_binlog_info(mysync_info_t *mi,
		const char *binlog_name, const uint32_t binlog_pos)
{
	char pos[16];

	ms_snprintf(pos, 16, "%u", binlog_pos);

	tcbdbput2(tdb, "my_binlog_name", binlog_name);
	tcbdbput2(tdb, "my_binlog_pos", pos);

	tcbdbsync(tdb);
}

int load_binlog_info(mysync_info_t *mi,
		char **binlog_name, uint32_t *binlog_pos)
{
	char *name;
	char *pos;

	name = NULL;
	pos  = NULL;

	name = tcbdbget2(tdb, "my_binlog_name");
	if (name == NULL) {
		return -1;
	}

	pos  = tcbdbget2(tdb, "my_binlog_pos");
	if (pos == NULL) {
		ms_free(name);
		return -1;
	}

	if (*binlog_name) {
		ms_pfree(mi->pool, *binlog_name);
	}

	*binlog_name = ms_strdup(mi->pool, name);
	*binlog_pos  = ms_atou(pos, ms_strlen(pos));

	ms_free(name);
	ms_free(pos);

	ms_debug("loading binlog info...,");
	ms_debug("binlog name %s, binlog pos %u", *binlog_name, *binlog_pos);

	return 0;
}

int flush_row_info(mysync_info_t *mi, ms_str_t *k, ms_str_t *v)
{
	/* 
	 * put the json-string into qs :
	 * key : ti->dt
	 * val : json-string
	 */
	int err;
	int pos;
	char queue_name[300];

	pos = httpsqs_now_putpos((char*)k->data);
	ms_snprintf(queue_name, 300, "%s:%d", k->data, pos);

	if (!tcbdbput2(tdb, queue_name, (char*)v->data)) {
		err = tcbdbecode(tdb);
		ms_error("close tdb failed: %s\n", tcbdberrmsg(err));
		return -1;
	}

	flush_binlog_info(mi, mi->binlog_name, mi->binlog_pos);

	return 0;
}

ms_str_t 
conv_2_json(mysync_info_t *mi, ms_table_info_t *ti, ms_str_t *meta)
{
	uint32_t i, len;
	uint8_t	 *p;
	ms_str_t s;
	ms_col_info_t *ci;

	ms_str_init(&s);

	/*
	 * such as:
	 * {"meta":"insert","data":{"user":"dcshi","email":"dcshi@qq.com"}}
	 */

	len = sizeof("{\"data\":{},\"meta\":}") - 1;
	len += ms_json_enstring_len(meta);

	/* calc the total len */
	for (i = 0; i < ti->cols->nelts; i++) {
		ci = ms_array_get(ti->cols, i);

		if (ci->is_null || ms_str_null(&ci->name)) continue;

		/* len for column name */
		len += ms_json_enstring_len(&ci->name);

		/* len for column value */
		if (!ms_str_null(&ci->data)) {
			len += ms_json_enstring_len(&ci->data);
		}
		else {
			len += ms_json_enstring_len(&ci->org_data);
		}
	
		len += 2; /* :, */
	}

	s.len = len - 1;
	s.data = ms_palloc(mi->pool, len);
	p = s.data;

	/* json encode */

	*p++ = '{'; *p++ = '"';
	*p++ = 'm'; *p++ = 'e'; *p++ = 't'; *p++ = 'a';
	*p++ = '"';
	*p++ = ':'; 
	ms_json_enstring(&p, meta);

	*p++ = ','; *p++ = '"';
	*p++ = 'd'; *p++ = 'a'; *p++ = 't'; *p++ = 'a';
	*p++ = '"';
	*p++ = ':'; *p++ = '{';

	for (i = 0; i < ti->cols->nelts; i++) {
		ci = ms_array_get(ti->cols, i);

		if (ci->is_null || ms_str_null(&ci->name)) continue;

		ms_json_enstring(&p, &ci->name);

		*p++ = ':';

		if (!ms_str_null(&ci->data)) {
			len += ms_json_enstring(&p, &ci->data);
		}
		else {
			len += ms_json_enstring(&p, &ci->org_data);
		}

		*p++ = ',';
	}
	*--p = '}'; *++p = '}'; *p = '\0';
	
	return s;
}
