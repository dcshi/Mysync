#ifndef _MS_LOCK_H_
#define _MS_LOCK_H_

#include "ms_core.h"

#define ms_shmtx_lock(x)   { /*void*/ }
#define ms_shmtx_unlock(x) { /*void*/ }

typedef unsigned char 	u_char;
typedef uintptr_t       ms_uint_t; 
typedef intptr_t        ms_int_t; 

struct ms_shmtx_s{

	ms_uint_t spin;
	
};

#endif
