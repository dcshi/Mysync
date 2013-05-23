#ifndef  MS_LOG_INC
#define  MS_LOG_INC

#include "ms_core.h"

#define LV_TRACE 1
#define LV_DEBUG 2
#define LV_INFO  4
#define LV_ERROR 8
#define LV_ALERT 16

#define log(level, format, ...) \
    do { \
        if ( level >= LOG_LEVEL ) {\
            fprintf(stderr, "[%c] [%s:%d] "format"\n", \
                     #level[3],  __FUNCTION__, __LINE__, ##__VA_ARGS__); \
        } \
    } while(0)

#define ms_trace(format, ...) log(L1_CACHE_ALIGNV_TRACE, format, ##__VA_ARGS__)
#define ms_debug(format, ...) log(LV_DEBUG, format, ##__VA_ARGS__)
#define ms_info(format, ...)  log(LV_INFO , format, ##__VA_ARGS__)
#define ms_error(format, ...) log(LV_ERROR, format, ##__VA_ARGS__)
#define ms_alert(format, ...) log(LV_ALERT, format, ##__VA_ARGS__)

#endif   
