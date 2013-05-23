#ifndef  MS_INFO_INC
#define  MS_INFO_INC

#include "ms_core.h"

typedef struct {
	uint32_t		slave_id;
	uint32_t		daemonize;
    char 		    *master_host;
	uint16_t	    master_port;
    char		    *master_user;
    char 		    *master_password;
	char            *cf_path;
	char            *qs_db;
	char		    *binlog_name;
	uint32_t		binlog_pos;
	uint32_t        ti_map_ratio;  
	uint32_t        ti_map_prime;
	uint32_t		ti_bucket_size;
	uint32_t		ti_bucket_height;
    ms_array_t 		*watch_dt; 		//dt for database and table
    ms_array_t      *ti_map;        //table id to table info map
    ms_hash_t       *ti_hash;       //table info hash struct
	ms_slab_pool_t	*pool;

	#define mysync_info_init(mi)				\
	(mi)->slave_id 		= 0;				\
	(mi)->daemonize 	= 0;				\
	(mi)->master_host 	= NULL;		        \
	(mi)->master_port 	= 0;		        \
	(mi)->master_user 	= NULL; 		    \
	(mi)->master_password	= NULL;	        \
	(mi)->cf_path       = NULL;             \
	(mi)->qs_db         = NULL;             \
	(mi)->binlog_name	= NULL;		        \
	(mi)->binlog_pos 	= 0;				\
	(mi)->ti_map_ratio  = 0;                \
	(mi)->ti_map_prime  = 0;                \
	(mi)->ti_bucket_size   = 0;				\
	(mi)->ti_bucket_height = 0;				\
	(mi)->watch_dt		= NULL;				\
	(mi)->ti_map        = NULL;             \
	(mi)->ti_hash       = NULL;             \
	(mi)->pool			= NULL;
} mysync_info_t;

typedef struct {
	uint8_t 	type;
	uint8_t		is_null;
	ms_str_t    name; 		/* the column name */
	ms_str_t 	org_data;	/* the column orginal data */
	ms_str_t	data;	
	ms_array_t	*meta;		/* for special type, enum, set etc */

	#define ms_col_info_init(ci)			\
	(ci)->type			= 0;				\
	(ci)->is_null		= 1;				\
	(ci)->name.len		= 0;				\
	(ci)->name.data		= NULL;				\
	(ci)->org_data.len	= 0;				\
	(ci)->org_data.data	= NULL;				\
	(ci)->data.len		= 0;				\
	(ci)->data.data		= NULL;				\
	(ci)->meta			= NULL;	
} ms_col_info_t;

typedef struct {
    ms_str_t   	dt;	
	ms_str_t	meta;	/* init by table map event */
    ms_array_t 	*cols;

	#define	ms_table_info_init(ti)			\
	(ti)->dt.len		= 0;				\
	(ti)->dt.data		= NULL;				\
	(ti)->meta.len		= 0;				\
	(ti)->meta.data		= NULL;				\
	(ti)->cols			= NULL;				
} ms_table_info_t;

int mysync_conf_load(mysync_info_t *ms_info, const char *cf);

void mysync_info_deinit(mysync_info_t *mi);
void ms_col_info_deinit(mysync_info_t *mi, ms_col_info_t *ci);
void ms_table_info_deinit(mysync_info_t *mi, ms_table_info_t *ti);

#endif 

