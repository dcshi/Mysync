#ifndef MS_SIGNAL_INC
#define MS_SIGNAL_INC

struct signal {
    int  signo;
    char *signame;
    int  flags;
    void (*handler)(int signo);
};

int  signal_init(void);
void signal_deinit(void);
void signal_handler(int signo);

volatile extern int ms_stop;
volatile extern int ms_reload;
volatile extern int ms_restart;

#endif
