#ifndef MS_CORE_INC
#define MS_CORE_INC

typedef struct ms_str_s ms_str_t;
typedef struct ms_array_s ms_array_t;
typedef struct ms_shmtx_s ms_shmtx_t;
typedef struct ms_slab_pool_s ms_slab_pool_t;

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <unistd.h>
#include <inttypes.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include "ms_log.h"
#include "ms_string.h"
#include "ms_array.h"
#include "ms_hash.h"
#include "ms_lock.h"
#include "ms_slab.h"

#ifndef MS_ALIGNMENT
#define MS_ALIGNMENT   sizeof(unsigned long)    /* platform word */
#endif

#define ms_align(d, a)     (((d) + (a - 1)) & ~(a - 1))
#define ms_align_ptr(p, a)                                                   \
	    (u_char *) (((uintptr_t) (p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1))

#endif
