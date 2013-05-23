#include "ms_info.h"
#include "ms_core.h"
#include "ms_type.h"
#include "ms_qs.h"

uint32_t cal_prime(uint32_t arg) 
{
	uint32_t i, n;

	if (arg % 2 == 0) {
		arg--;
	}

	for (; arg > 2;) {

		n = (uint32_t)sqrt(arg);
		for (i = 3; i <= n; i += 2) {
			if (arg % i == 0) {
				goto redo;
			}
		}
		return arg;
redo:
		arg -= 2;
	}

	return arg;
}

ms_hash_t * ti_hash_init(mysync_info_t *mi)
{
	uint32_t		i, n;
	ms_hash_init_t 	hinit;
	ms_array_t  	*tis;
	ms_table_info_t *ti;
	ms_hash_key_t	*hk;

	n = mi->watch_dt->nelts;

	tis = ms_array_create(mi->pool, n, sizeof(ms_hash_key_t));
	if (tis == NULL) {
		ms_error("ms_array_create fail, larger memory pool size is needed");
		return NULL;
	}

	for (i = 0; i < n; i++) {
		hk = ms_array_push(tis);
		if (hk == NULL) {
			ms_array_destroy(tis);
			return NULL;
		}

		ti = ms_array_get(mi->watch_dt, i);
		hk->key = ti->dt;
		hk->key_hash = ms_hash_key_lc(ti->dt.data, ti->dt.len);
		hk->value = ti;
	}

	hinit.hash = NULL;
	hinit.key  = ms_hash_key_lc;
	hinit.max_size = mi->ti_bucket_height;
	hinit.bucket_size = mi->ti_bucket_size;
	hinit.name = "ti_hash";
	hinit.pool = mi->pool;

	if (ms_hash_init(&hinit, tis->elts, tis->nelts)) {
		ms_array_destroy(tis);
		return NULL;
	}

	ms_array_destroy(tis);

	return hinit.hash;
}

int mysync_info_verify(mysync_info_t *info)
{
    mysync_info_t *mi = info;

    if (mi->slave_id == 0 ||
            mi->master_host == NULL     ||
            mi->master_port == 0        ||
            mi->master_user == NULL     ||
            mi->master_password == NULL ||
            mi->cf_path == NULL         ||
            mi->qs_db == NULL           ||
			mi->ti_map_prime == 0        ||
            mi->ti_map == NULL          ||
            mi->ti_hash == NULL         ||
            mi->watch_dt == NULL        ||
            mi->pool == NULL) {
        return -1;
    }

    return 0;
}

void mysync_info_deinit(mysync_info_t *mi)
{
	if (mi == NULL) {
		return;
	}

	if (mi->qs_db) {
		qs_db_deinit(mi);
	}

	if (mi->pool) {
		ms_free(mi->pool);
	}

	mysync_info_init(mi);
}

void ms_col_info_deinit(mysync_info_t *mi, ms_col_info_t *ci)
{
	uint32_t i;
	ms_str_t *s;

	if (!ms_str_null(&ci->name)) {
		ms_pfree(mi->pool, ci->name.data);
	}

	if (!ms_str_null(&ci->data)) {
		ms_pfree(mi->pool, ci->data.data);
	}

	if (ci->meta) {
		for (i = 0; i < ms_array_n(ci->meta); i++) {
			s = ms_array_get(ci->meta, i);
			ms_pfree(mi->pool, s->data);
		}

		ms_array_destroy(ci->meta);
	}

	ms_col_info_init(ci);
}

void ms_table_info_deinit(mysync_info_t *mi, ms_table_info_t *ti)
{
	uint32_t i;
	ms_col_info_t *ci;

	if (!ms_str_null(&ti->dt)) {
		ms_pfree(mi->pool, ti->dt.data);
	}

	if (!ms_str_null(&ti->meta)) {
		ms_pfree(mi->pool, ti->meta.data);
	}

	if (ti->cols) {
		for (i = 0; i < ms_array_n(ti->cols); i++) {
			ci = ms_array_get(ti->cols, i);
			ms_col_info_deinit(mi, ci);
		}

		ms_array_destroy(ti->cols);
	}

	ms_table_info_init(ti);
}

int mysync_conf_load(mysync_info_t *mi, const char *cf)
{
	FILE *fp;
	uint32_t n;
	char buf[512];
	char key[128];
	char val[256];

	mysync_info_t new_info;
	mysync_info_init(&new_info);

    if (cf == NULL)
    {
        ms_error("config path is need; just try ./mysync -c etc/ms.cf");
        return -1;
    }

	if ((fp = fopen(cf, "r")) == NULL) 
	{
		ms_error("open mysync config file %s failed: %s", 
				cf, strerror(errno));
		return -1;
	}

	for (;;)
	{
		fgets((char *)buf, sizeof(buf)-1, fp);
		/*
			ms_error("read mysync config file %s failed: %s",
					cf, strerror(errno));
			return -1;
		*/
		if (feof(fp)) break;

		if (buf[0] == '#' || buf[0] == ';') continue;

		if (sscanf(buf, "%s %s", key, val) != 2) {
		    continue;
		}

		if (ms_memcmp(key, "slave_id") == 0) {
			new_info.slave_id = ms_atou(val, ms_strlen(val));
		}
		else if (ms_memcmp(key, "pid") == 0) {
			/* todo */
		}
		else if (ms_memcmp(key, "daemonize") == 0) {
			new_info.daemonize = ms_atou(val, ms_strlen(val)) > 0 ? 1 : 0;
		}
		else if (ms_memcmp(key, "pool_size") == 0) {
			size_t l;
			u_char *p;

			l = (size_t)ms_atou(val, ms_strlen(val));
			if (l <= 0) {
				ms_error("invaild config %s", buf);
				return -1;
			}

			p = (u_char*)ms_alloc(l * 1024);
			new_info.pool = (ms_slab_pool_t*)p;
			new_info.pool->addr = p;
			new_info.pool->min_shift = 3;
			new_info.pool->end = p + l*1024;
			ms_slab_init(new_info.pool);
		}
		else if (ms_memcmp(key, "master_host") == 0) {
			new_info.master_host = ms_strdup(new_info.pool, val);
		}
		else if (ms_memcmp(key, "master_port") == 0) {
            new_info.master_port = (uint16_t)ms_atou(val, ms_strlen(val));
		}
		else if (ms_memcmp(key, "master_user") == 0) {
			new_info.master_user = ms_strdup(new_info.pool, val);
		}
		else if (ms_memcmp(key, "master_password") == 0) {
			new_info.master_password = ms_strdup(new_info.pool, val);
		}
		else if (ms_memcmp(key, "qs_db") == 0) {
			new_info.qs_db = ms_strdup(new_info.pool, val);
		}
        else if (ms_memcmp(key, "ti_map_ratio") == 0) {
            new_info.ti_map_ratio = ms_atou(val, ms_strlen(val));
        }
		else if (ms_memcmp(key, "ti_bucket_size") == 0) {
			new_info.ti_bucket_size = ms_atou(val, ms_strlen(val));
		}
		else if (ms_memcmp(key, "ti_bucket_height") == 0) {
			new_info.ti_bucket_height = ms_atou(val, ms_strlen(val));
		}
		else if (ms_memcmp(key, "watch") == 0)
		{
			if (new_info.watch_dt == NULL) 
			{
				new_info.watch_dt = 
					ms_array_create(new_info.pool, 4, sizeof(ms_table_info_t));
				
				if (new_info.watch_dt == NULL) {
				    ms_error("ms_array_create fail, larger memory pool size is needed");
				    return -1;
                }
			}	

			ms_table_info_t *ti;
			ti = ms_array_push(new_info.watch_dt);
			if (ti == NULL) {
				ms_error("ms_array_push fail, larger memory pool size is needed");
				return -1;
			}

			ms_table_info_init(ti);
			ti->dt.data  = (uint8_t*)ms_strdup(new_info.pool, val);
			ti->dt.len   = ms_strlen(val);
		}
		else {
			ms_error("invaild config %s", buf);
			return -1;
		}
	}

    /* save cf */
    new_info.cf_path = ms_strdup(new_info.pool, cf);

	n = ms_array_n(new_info.watch_dt);
	if (n) {
		/* table info map init*/
		if (new_info.ti_map_ratio) 
		{
			new_info.ti_map = ms_array_create(new_info.pool,
					n * new_info.ti_map_ratio,
					sizeof(ms_table_info_t*));

			if (new_info.ti_map == NULL) {
				ms_error("ms_array_create fail, make sure memory pool size is enough");
				return -1;
			}

			new_info.ti_map->nelts = n * new_info.ti_map_ratio;
		}

		/* table info map prime calculate */
		new_info.ti_map_prime = cal_prime(n * new_info.ti_map_ratio);
		
		/* table hash init */
        new_info.ti_hash = ti_hash_init(&new_info);
		if (new_info.ti_hash == NULL) {
			return -1;
		}
	}
	else {
		ms_error("watch directive is need.");
		return -1;
	}

    if (mysync_info_verify(&new_info)) {
        return -1;
    }

	mysync_info_deinit(mi);

	*mi = new_info;

	return 0;
}
