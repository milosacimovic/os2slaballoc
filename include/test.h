#ifndef _TEST_H_
#define _TEST_H_

#define MASK (0xA5)
#include "slab.h"

struct data_s {
	int id;
	kmem_cache_t *shared;
	int iterations;
};

#endif