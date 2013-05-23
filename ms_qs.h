#ifndef MS_QS_INC
#define MS_QS_INC

#include "ms_core.h"
#include "ms_info.h"
#include "ms_type.h"

int 
qs_db_init(mysync_info_t *mi);

int 
qs_db_deinit(mysync_info_t *mi);

void 
flush_binlog_info(mysync_info_t *mi,
		const char *binlog_name, const uint32_t binlog_pos);

int 
load_binlog_info(mysync_info_t *mi,
		char **binlog_name, uint32_t *binlog_pos);

int 
flush_row_info(mysync_info_t *mi, ms_str_t *k, ms_str_t *v);

ms_str_t 
conv_2_json(mysync_info_t *mi, ms_table_info_t *ti, ms_str_t *meta);

#endif
