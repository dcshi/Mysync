#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

#include "ms_core.h"
#include "ms_info.h"
#include "ms_net.h"
#include "ms_qs.h"
#include "ms_daemon.h"
#include "ms_signal.h"
#include "ms_event.h"

static mysync_info_t ms_info;
static char * cf_path = NULL;

void run_prepare()
{
    if (connect_to_master(&ms_info)) {
		exit(-1);
    }

    if (sync_master_version()) {
		exit(-1);
    }

    if (sync_master_binfmt()) {
		exit(-1);
    }

    if (qs_db_init(&ms_info)) {
        exit(-1);
    }

    if (sync_table_info(&ms_info)) {
		exit(-1);
    }

    if (sync_binlog_info(&ms_info)) {
		exit(-1);
    }
}

void run_loop()
{
	while (!ms_stop) 
	{
		/* do main logic */
		event_handler(&ms_info);

		if (ms_reload) 
		{
			ms_reload = 0;
			ms_debug("mysync reloading now...");

			/* ignore the new error config */
			if (mysync_conf_load(&ms_info, cf_path)) {
				ms_error("reload config failed...");
				continue;
			}

			disconnect_to_master();

			run_prepare(&ms_info);
		}	
	}

	disconnect_to_master();

	ms_free(ms_info.pool);

	ms_debug("bye mysync...");	
}

void run_post()
{
	disconnect_to_master();
}

struct option longopts[] = {
    { "config", required_argument, NULL, 'c' },
    { 0, 0, 0, 0 }
};

void parse_options(int argc, char **argv)
{
    int opt = 0;
    while ((opt = getopt_long(argc, argv, "c:h", longopts, NULL)) != -1)
    {
        switch (opt) {
            case 'c':
                cf_path = optarg;
				break;
            case 'h':
            default:
                printf("Usage: mysync -c <config>\n");
                exit(1);
        }
    }
}

int main(int argc, char **argv)
{
    //mysync_info_init(&ms_info);

	signal_init();

    parse_options(argc, argv);

	if (mysync_conf_load(&ms_info, cf_path)) {
	    return -1;
    }

	if (ms_info.daemonize) {
		ms_daemon();
		printf("mysync pid is %d\n", ms_pid);
	}

	/* log_init */

    run_prepare();

    run_loop();

	run_post();

    return 0;
}
