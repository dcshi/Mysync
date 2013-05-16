#ifndef MS_BINLOG_INC
#define MS_BINLOG_INC

#include "ms_core.h"
#include "ms_info.h"
#include "ms_type.h"

typedef struct comm_header_s 		comm_header_t;
typedef struct rotate_event_s 		rotate_event_t;
typedef struct table_map_event_s 	table_map_event_t;
typedef struct rows_event_s  		rows_event_t;
typedef struct write_rows_event_s  	write_rows_event_t;
typedef struct update_rows_event_s 	update_rows_event_t;
typedef struct delete_rows_event_s  delete_rows_event_t;

struct comm_header_s {
	uint32_t	when;
	uint8_t		type;
	uint32_t	server_id;
	uint32_t	data_written;
	uint32_t	log_pos;
	uint16_t	flags;
};

struct rotate_event_s {
	uint64_t 	pos;
	uint8_t		*new_log;
};

struct table_map_event_s {
	uint64_t	table_id;
	uint16_t 	flags;
	ms_str_t	db_name;
	ms_str_t	table_name;
	ms_str_t	cols;
	ms_str_t	metadata;
};

struct rows_event_s {
	uint64_t	table_id;
	uint16_t	flags;
	uint16_t	ncols;
	ms_str_t	data;
};

struct write_rows_event_s {
	uint64_t	table_id;
	uint16_t	flags;
	uint16_t	ncols;
	ms_str_t	data;
	uint64_t	cols_mask;  /* test nbit == ncols */
};

struct update_rows_event_s {
	uint64_t	table_id;
	uint16_t	flags;
	uint16_t	ncols;
	ms_str_t	data;
	uint64_t	pre_cols_mask;	/* pre update */
	uint64_t 	post_cols_mask; /* post update */
};

struct delete_rows_event_s {
	uint64_t	table_id;
	uint16_t	flags;
	uint16_t	ncols;
	ms_str_t	data;
	uint64_t	cols_mask;  /* test nbit == ncols */
};

void comm_header_parse(comm_header_t *h, ms_str_t *buf);

void rotate_event_parse(rotate_event_t *e, ms_str_t *buf);

void table_map_event_parse(table_map_event_t *e, ms_str_t *buf);
bool table_map_event_verify(table_map_event_t *e);

void write_rows_event_parse(write_rows_event_t *e, ms_str_t *buf);

void update_rows_event_parse(update_rows_event_t *e, ms_str_t *buf);

void delete_rows_event_parse(delete_rows_event_t *e, ms_str_t *buf);

void
table_map_event_handler(mysync_info_t *mi, table_map_event_t *e);

void
write_rows_event_handler(mysync_info_t *mi,	write_rows_event_t *e);

void
update_rows_event_handler(mysync_info_t *mi, update_rows_event_t *e);

void
delete_rows_event_handler(mysync_info_t *mi, delete_rows_event_t *e);


#endif
