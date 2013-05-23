#ifndef MS_JSON_INC
#define MS_JSON_INC

#include "ms_core.h"

int ms_json_enstring_len(ms_str_t *src);

int ms_json_enstring(uint8_t **dst, ms_str_t *src);


#endif
