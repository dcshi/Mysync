#include <my_global.h>
#include <m_ctype.h>
#include <mysql.h>
#include <sql_common.h>
#include "ms_net.h"
#include "ms_qs.h"

static MYSQL _mysql;
bool  is_connected;

int connect_to_master(mysync_info_t *mi)
{
	uint32_t timeout;
	is_connected = false;

	if (!mysql_init(&_mysql))
	{
		ms_error("mysql_init fail");
		return -1;
	}

	timeout = 600;
	mysql_options(&_mysql, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
	mysql_options(&_mysql, MYSQL_OPT_READ_TIMEOUT, &timeout);
	mysql_options(&_mysql, MYSQL_OPT_WRITE_TIMEOUT, &timeout);

	if (mysql_real_connect(&_mysql, 
				mi->master_host, mi->master_user,
				mi->master_password, 0,
			   	mi->master_port, 0, 0) == 0)
    {
        ms_error("connect to mysql fail,"
                "user:%s, password:%s, host:%s, port:%u, error:%s",  
                mi->master_user, mi->master_password,     
                mi->master_host, mi->master_port,
                mysql_error(&_mysql));

        return -1;
	}

	is_connected = true;

	return 0;
}

int reconnect_to_master(mysync_info_t *mi)
{
	ms_debug("begin reconnect to master...");

	disconnect_to_master();
	
	if (connect_to_master(mi)) {
		return -1;
	}

	if (sync_binlog_info(mi)) {
		return -1;
	}
	
	return 0;
}

void disconnect_to_master()
{
	is_connected = false;
	mysql_close(&_mysql);
}

int sync_master_version()
{
	MYSQL_RES* res = 0;
	MYSQL_ROW row;
	const char* version;

	if (mysql_query(&_mysql, "SELECT VERSION()") || 
			!( res = mysql_store_result(&_mysql) ) ||
			!( row = mysql_fetch_row(res) ) || 
			!( version = row[0] )
	   )
	{
		if (res)
		{
			mysql_free_result(res);
		}

		ms_error("get master version fail, %s", mysql_error(&_mysql));
		return -1;
	}

	if (*version != '5')
	{
		if (res)
		{
			mysql_free_result(res);
		}

		ms_error("invalid master version(%s), only 5.x is supported.", version);
		return -1;
	}

	mysql_free_result(res);
    ms_debug("master version is %s", version);

	return 0;
}

int sync_master_binfmt()
{
	MYSQL_RES* res = 0;
	MYSQL_ROW row;
	const char* fmt;

	if (mysql_query(&_mysql, "SHOW VARIABLES LIKE \"BINLOG_FORMAT\"") || 
			!( res = mysql_store_result(&_mysql) ) ||
			!( row = mysql_fetch_row(res) ) || 
			!( fmt = row[1] )
	   )
	{
		if (res)
		{
			mysql_free_result(res);
		}

		ms_error("get master binlog format fail, %s", mysql_error(&_mysql));
		return -1;
	}

	if (ms_memcmp(fmt, "ROW"))
	{
		if (res)
		{
			mysql_free_result(res);
		}

		ms_error("invalid master binlog format(%s), only ROW  is supported.", fmt);
		return -1;
	}

	mysql_free_result(res);

	return 0;
}

int sync_table_info(mysync_info_t *mi)
{
	uint32_t i;
	ms_array_t  *dt;
    ms_table_info_t *ti;

	dt = mi->watch_dt;
	for (i = 0; i < ms_array_n(dt); i++) 
	{
        ti = (ms_table_info_t*)ms_array_get(dt, i);

		/* ti->cols is null now */
		ti->cols = ms_array_create(mi->pool, 4, sizeof(ms_col_info_t));

		if (sync_one_table_info(mi, ti)) {
			return -1;
		}
	}

	return 0;
}

int sync_one_table_info(mysync_info_t *mi, ms_table_info_t *ti)
{
	uint32_t i;
	char sql[256];
	MYSQL mysql;
	ms_str_t 	*s;
	ms_col_info_t	*ci;

	MYSQL_RES* res_column = 0;
	MYSQL_ROW row;

	assert(mi != NULL && ti != NULL && ti->cols != NULL);

	ms_snprintf(sql, 256, "show columns from %s;", ti->dt.data);

	/*
	 * use a new connection here
	 */
	if (!mysql_init(&mysql))
	{
		ms_error("mysql_init fail");
		return -1;
	}

	if (mysql_real_connect(&mysql, 
				mi->master_host, mi->master_user,
				mi->master_password, 0,
			   	mi->master_port, 0, 0) == 0)
    {
        ms_error("connect to mysql fail,"
                "user:%s, password:%s, host:%s, port:%u, error:%s",  
                mi->master_user, mi->master_password,     
                mi->master_host, mi->master_port,
                mysql_error(&_mysql));

        return -1;
	}

	if (mysql_query(&mysql, sql) != 0 || 
			(res_column = mysql_store_result(&mysql)) == NULL)
	{   
		mysql_close(&mysql);
		ms_error("mysql_query '%s' failed: %s", sql, mysql_error(&_mysql));
		return -1;
	}   

	i = 0;
	while ((row = mysql_fetch_row(res_column)) != NULL) 
	{   
		/*
		 * row[0] for column name
		 * row[1] for column type
		 */
		
		ci = ms_array_push(ti->cols);
		ci->name.len = ms_strlen(row[0]);
		ci->name.data = (uint8_t*)ms_strndup(mi->pool, row[0], ci->name.len);

		if (row[1] && 
				(ms_memcmp(row[1], "enum") == 0 || 
				 ms_memcmp(row[1],"set") == 0))  
		{
			char *pos1, *pos2;
			pos1 = row[1];

			ci->meta = ms_array_create(mi->pool, 4, sizeof(ms_str_t));

			while (*pos1) {
				pos1 = strstr(pos1, "\'");
				if (!pos1) break;

				pos2 = strstr(++pos1, "\'");
				if (!pos2) break;

				s = ms_array_push(ci->meta);
				if (s == NULL) {
					return -1;
				}

				s->len  = pos2 - pos1;
				s->data = (uint8_t*)ms_strndup(mi->pool, pos1, s->len);

				pos1 = pos2 + 1;
			}
		}

		i++;
	} 

	mysql_free_result(res_column);
	mysql_close(&mysql);
	res_column = NULL;

	ms_debug("mysql_query '%s' success", sql);

	return 0;
}

int sync_binlog_info(mysync_info_t *mi)
{
	unsigned char buf[1024];
	MYSQL_RES* res;
	MYSQL_ROW row;

	load_binlog_info(mi, &mi->binlog_name, &mi->binlog_pos);

	if (mi->binlog_name == NULL)
	{
		if (mysql_query(&_mysql, "SHOW MASTER STATUS") || 
				(res = mysql_store_result(&_mysql)) == NULL)
		{
			if (res)
			{
				mysql_free_result(res);
			}

			is_connected = false;
			ms_error("mysql_query 'show master status' failed: %s", 
					mysql_error(&_mysql));
			return -1;
		}

		if ((row = mysql_fetch_row(res)) == NULL)
		{
			mysql_free_result(res);

			ms_error("mysql_query 'show master status' returns 0 rows,"
			        " please check your master conf.");
			return -1;
		}

		if (!(row[0] && row[0][0] && row[1]))
		{
			mysql_free_result(res);

			is_connected = false;
			ms_error("returns invalid row due to '%s'", mysql_error(&_mysql));
			return -1;
		}

		mi->binlog_pos = ms_atou(row[1], ms_strlen(row[1]));
		mi->binlog_name = ms_strdup(mi->pool, row[0]); 

		if (mi->binlog_name == NULL)
		{
			mysql_free_result(res);

			ms_error("memory alloc fail, larger pool size is needed.");
			return -1;
		}

        ms_debug("binlog name is %s , binlog pos is %u",
                mi->binlog_name, mi->binlog_pos);

		flush_binlog_info(mi, mi->binlog_name, mi->binlog_pos);

		mysql_free_result(res);
	}

	int4store(buf, mi->binlog_pos);
	int2store(buf + 4, 0); // flags
	int4store(buf + 6, mi->slave_id);
    ms_snprintf(buf + 10, 1014, "%s", mi->binlog_name);

	if (simple_command(&_mysql, COM_BINLOG_DUMP,
			   	(const unsigned char*)buf, ms_strlen(mi->binlog_name) + 10, 1))
	{
		is_connected = false;
		ms_error("binlog dump request from '%s:%d' failed: %s", 
				mi->binlog_name, mi->binlog_pos, mysql_error(&_mysql));
		return -1;
	}

	return 0;
}

int master_save_read(ms_str_t *buf)
{
	ms_str_init(buf);

	buf->len = cli_safe_read(&_mysql);

	if (buf->len == packet_error || buf->len < 1)
	{
		is_connected = false;
		ms_error("reading packet from master failed: %s",
				mysql_error(&_mysql));
		return -1;
	}

	if (buf->len < 8 && _mysql.net.read_pos[0] == 254)
	{
		is_connected = false;
		ms_error("recv end packet from server," 
			   	"apparent master shutdown: %s", mysql_error(&_mysql));
		return -1;
	}

	buf->data = _mysql.net.read_pos;

	buf->data++; 
	buf->len--;

	return 0;
}
