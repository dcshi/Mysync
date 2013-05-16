#include <my_global.h>
#include "ms_event.h"
#include "ms_binlog.h"
#include "ms_net.h"
#include "ms_type.h"

bool event_supported(uint32_t event_type)
{
	return (event_type <= LOG_EVENT_TYPES);
	/*
	return (event_type <= LOG_EVENT_TYPES ||
			event_type == FORMAT_DESCRIPTION_EVENT);
	*/
}

int event_dispatch(mysync_info_t *mi, 
		uint32_t event_type, ms_str_t *buf)
{
	comm_header_t ch;
	
	comm_header_parse(&ch, buf);
	mi->binlog_pos = ch.log_pos;

	ms_debug("event type %u accepted", event_type);

	switch (event_type)
	{   
		case ROTATE_EVENT: 
		{
			rotate_event_t e;
			rotate_event_parse(&e, buf);

			ms_pfree(mi->pool, mi->binlog_name);
			mi->binlog_name = ms_strdup(mi->pool, (char*)e.new_log);
			mi->binlog_pos = e.pos;

			/*
			 * only when the row-data is flushed to the disk.
			 * flush_binlog_info(mi->binlog_name, mi->binlog_pos);
			 */

			break;
		}
		case FORMAT_DESCRIPTION_EVENT:
		{
			/* ignore */
			break;
		}
		case QUERY_EVENT:
		{
			/* ignore */
			break;
		}
		case TABLE_MAP_EVENT:
		{
			table_map_event_t e;

			table_map_event_parse(&e, buf);

			if (!table_map_event_verify(&e)) break;

			table_map_event_handler(mi, &e);

			break;
		}
		case  WRITE_ROWS_EVENT:
		{
			write_rows_event_t e;

			write_rows_event_parse(&e, buf);

			write_rows_event_handler(mi, &e);

			break;
		}
		case UPDATE_ROWS_EVENT:
		{
			update_rows_event_t e;

			update_rows_event_parse(&e, buf);

			update_rows_event_handler(mi, &e);

			break;
		}
		case DELETE_ROWS_EVENT:
		{
			delete_rows_event_t e;

			delete_rows_event_parse(&e, buf);

			delete_rows_event_handler(mi, &e);

			break;
		}
		default:
		{
			ms_info("unkown event type %u", event_type);
			break;
		}
	}

	return 0;
}

int event_handler(mysync_info_t *mi)
{
	ms_str_t buf;
	uint32_t event_type;

	ms_debug("event handing....");

	if (!is_connected && reconnect_to_master(mi)) {
		sleep(1);
		return -1;
	}

	/* blocked when waiting binglog data */
	if (master_save_read(&buf)) {
		return -1;
	}

	if (buf.len < EVENT_LEN_OFFSET ||
			buf.len != uint4korr(buf.data + EVENT_LEN_OFFSET))
	{
		ms_error("event sanity check failed");
		return -1;
	}

	event_type = buf.data[EVENT_TYPE_OFFSET]; 
	if (!event_supported(event_type))
	{
		ms_info("event_type(%u) is not supported", event_type);	
		return 0;  /* not an error*/
	}   

	return event_dispatch(mi, event_type, &buf);
}
