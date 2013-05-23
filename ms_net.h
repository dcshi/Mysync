#ifndef  MS_NET_INC
#define  MS_NET_INC

#include "ms_core.h"
#include "ms_info.h"
#include "ms_type.h"

int connect_to_master(mysync_info_t *mi);

int reconnect_to_master(mysync_info_t *mi);

void disconnect_to_master();

int sync_master_version();

int sync_master_binfmt();

int sync_table_info(mysync_info_t *mi);
int sync_one_table_info(mysync_info_t *mi, ms_table_info_t *ti);

int sync_binlog_info(mysync_info_t *mi);

int master_save_read(ms_str_t *buf);

extern bool  is_connected;

#endif

