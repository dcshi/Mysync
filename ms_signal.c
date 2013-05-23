#include <stdlib.h>
#include <signal.h>

#include "ms_signal.h"
#include "ms_core.h"

volatile int ms_stop 	= 0;
volatile int ms_reload 	= 0;
volatile int ms_restart	= 0;

static struct signal signals[] = {
    { SIGUSR1, "SIGUSR1", 0,                 signal_handler },
    { SIGUSR2, "SIGUSR2", 0,                 signal_handler },
    { SIGTERM, "SIGTERM", 0,                 signal_handler },
    { SIGQUIT, "SIGQUIT", 0,                 signal_handler },
    { SIGINT,  "SIGINT",  0,                 signal_handler },
    { SIGPIPE, "SIGPIPE", 0,                 SIG_IGN },
    { 0,        NULL,     0,                 NULL }
};

int signal_init(void)
{
    struct signal *sig;

    for (sig = signals; sig->signo != 0; sig++) {
        int status;
        struct sigaction sa;

        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = sig->handler;
        sa.sa_flags = sig->flags;
        sigemptyset(&sa.sa_mask);

        status = sigaction(sig->signo, &sa, NULL);
        if (status < 0) {
            ms_error("sigaction(%s) failed: %s", 
					sig->signame, strerror(errno));
            return -1;
        }
    }

    return 0;
}

void signal_deinit(void)
{
	/* void */
}

void signal_handler(int signo)
{
	const char		*action;
    struct signal 	*sig;
	
    for (sig = signals; sig->signo != 0; sig++) {
        if (sig->signo == signo) {
            break;
        }
    }

    //ASSERT(sig->signo != 0);

    switch (signo) {
    case SIGUSR1:
		ms_reload = 1;
		action = ", reloading";
        break;

    case SIGUSR2:
    case SIGHUP:
		ms_restart = 1;
		action = ", restarting"; 
        break;

	case SIGINT:
	case SIGTERM:
	case SIGQUIT:
		ms_stop = 1;
		action = ", exiting"; 
        break;

    default:
		return;
    }

    ms_info("signal %d (%s) received%s", signo, sig->signame, action);
}
