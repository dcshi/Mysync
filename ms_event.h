#ifndef  MS_EVENT_INC
#define  MS_EVENT_INC

#include "ms_core.h"
#include "ms_info.h"

#define LOG_EVENT_OFFSET 4
#define ST_SERVER_VER_LEN 50
#define LOG_EVENT_HEADER_LEN 19     /* the fixed header length */
#define OLD_HEADER_LEN       13     /* the fixed header length in 3.23 */
#define LOG_EVENT_MINIMAL_HEADER_LEN 19
/* event-specific post-header sizes */
// where 3.23, 4.x and 5.0 agree
#define QUERY_HEADER_MINIMAL_LEN     (4 + 4 + 1 + 2)
// where 5.0 differs: 2 for len of N-bytes vars.
#define QUERY_HEADER_LEN     (QUERY_HEADER_MINIMAL_LEN + 2)
#define STOP_HEADER_LEN      0
#define LOAD_HEADER_LEN      (4 + 4 + 4 + 1 +1 + 4)
#define SLAVE_HEADER_LEN     0
#define START_V3_HEADER_LEN     (2 + ST_SERVER_VER_LEN + 4)
#define ROTATE_HEADER_LEN    8 // this is FROZEN (the Rotate post-header is frozen)
#define INTVAR_HEADER_LEN      0
#define CREATE_FILE_HEADER_LEN 4
#define APPEND_BLOCK_HEADER_LEN 4
#define EXEC_LOAD_HEADER_LEN   4
#define DELETE_FILE_HEADER_LEN 4
#define NEW_LOAD_HEADER_LEN    LOAD_HEADER_LEN
#define RAND_HEADER_LEN        0
#define USER_VAR_HEADER_LEN    0
#define FORMAT_DESCRIPTION_HEADER_LEN (START_V3_HEADER_LEN+1+LOG_EVENT_TYPES)
#define XID_HEADER_LEN         0
#define BEGIN_LOAD_QUERY_HEADER_LEN APPEND_BLOCK_HEADER_LEN
#define ROWS_HEADER_LEN        8
#define TABLE_MAP_HEADER_LEN   8
#define EXECUTE_LOAD_QUERY_EXTRA_HEADER_LEN (4 + 4 + 4 + 1)
#define EXECUTE_LOAD_QUERY_HEADER_LEN  (QUERY_HEADER_LEN + EXECUTE_LOAD_QUERY_EXTRA_HEADER_LEN)
#define INCIDENT_HEADER_LEN    2
/*
  Max number of possible extra bytes in a replication event compared to a
  packet (i.e. a query) sent from client to master;
  First, an auxiliary log_event status vars estimation:
*/
#define MAX_SIZE_LOG_EVENT_STATUS (1 + 4          /* type, flags2 */   + \
                                   1 + 8          /* type, sql_mode */ + \
                                   1 + 1 + 255    /* type, length, catalog */ + \
                                   1 + 4          /* type, auto_increment */ + \
                                   1 + 6          /* type, charset */ + \
                                   1 + 1 + 255    /* type, length, time_zone */ + \
                                   1 + 2          /* type, lc_time_names_number */ + \
                                   1 + 2          /* type, charset_database_number */ + \
                                   1 + 8          /* type, table_map_for_update */ + \
                                   1 + 4          /* type, master_data_written */ + \
                                   1 + 16 + 1 + 60/* type, user_len, user, host_len, host */)

#define MAX_LOG_EVENT_HEADER   ( /* in order of Query_log_event::write */ \
  LOG_EVENT_HEADER_LEN + /* write_header */ \
  QUERY_HEADER_LEN     + /* write_data */   \
  EXECUTE_LOAD_QUERY_EXTRA_HEADER_LEN + /*write_post_header_for_derived */ \
  MAX_SIZE_LOG_EVENT_STATUS + /* status */ \
  NAME_LEN + 1)

/*
   Event header offsets;
   these point to places inside the fixed header.
*/
#define EVENT_TYPE_OFFSET    4
#define SERVER_ID_OFFSET     5
#define EVENT_LEN_OFFSET     9
#define LOG_POS_OFFSET       13
#define FLAGS_OFFSET         17

/* query event post-header */
#define Q_THREAD_ID_OFFSET	0
#define Q_EXEC_TIME_OFFSET	4
#define Q_DB_LEN_OFFSET		8
#define Q_ERR_CODE_OFFSET	9
#define Q_STATUS_VARS_LEN_OFFSET 11
#define Q_DATA_OFFSET		QUERY_HEADER_LEN

/* Intvar event data */
#define I_TYPE_OFFSET        0
#define I_VAL_OFFSET         1

/* Rotate event post-header */
#define R_POS_OFFSET       0
#define R_IDENT_OFFSET     8

/* TM = "Table Map" */
#define TM_MAPID_OFFSET    0
#define TM_FLAGS_OFFSET    6

/* RW = "RoWs" */
#define RW_MAPID_OFFSET    0
#define RW_FLAGS_OFFSET    6

/* 4 bytes which all binlogs should begin with */
#define BINLOG_MAGIC        "\xfe\x62\x69\x6e"

enum Log_event_type
{
	// Every time you update this enum (when you add a type), you have to fix CFormatDescriptionLogEvent
	UNKNOWN_EVENT = 0,
	START_EVENT_V3 = 1,
	QUERY_EVENT = 2,
	STOP_EVENT = 3,
	ROTATE_EVENT = 4,
	INTVAR_EVENT = 5,
	LOAD_EVENT = 6,
	SLAVE_EVENT = 7,
	CREATE_FILE_EVENT = 8,
	APPEND_BLOCK_EVENT = 9,
	EXEC_LOAD_EVENT = 10,
	DELETE_FILE_EVENT = 11,
	/*
	NEW_LOAD_EVENT is like LOAD_EVENT except that it has a longer
	sql_ex, allowing multibyte TERMINATED BY etc; both types share the
	same class (Load_log_event)
	*/
	NEW_LOAD_EVENT = 12,
	RAND_EVENT = 13,
	USER_VAR_EVENT = 14,
	FORMAT_DESCRIPTION_EVENT = 15,
	XID_EVENT = 16,
	BEGIN_LOAD_QUERY_EVENT = 17,
	EXECUTE_LOAD_QUERY_EVENT = 18,

	TABLE_MAP_EVENT = 19,

	// These event numbers were used for 5.1.0 to 5.1.15 and are therefore obsolete 
	PRE_GA_WRITE_ROWS_EVENT = 20,
	PRE_GA_UPDATE_ROWS_EVENT = 21,
	PRE_GA_DELETE_ROWS_EVENT = 22,

	// These event numbers are used from 5.1.16 and forward 
	WRITE_ROWS_EVENT = 23,
	UPDATE_ROWS_EVENT = 24,
	DELETE_ROWS_EVENT = 25,

	// Something out of the ordinary happened on the master
	INCIDENT_EVENT = 26,

	/*
	Add new events here - right above this comment!
	Existing events (except ENUM_END_EVENT) should never change their numbers
	*/
	ENUM_END_EVENT /* end marker */
};

#define LOG_EVENT_TYPES (ENUM_END_EVENT-1)

int event_handler(mysync_info_t *ms_info);

#endif 

