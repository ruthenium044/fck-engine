// kll_heap.c

#define FCK_KLL_EXPORT
#include "kll_system.h"

#include "kll.h"

#include <fckc_inttypes.h>

#include <stdlib.h>

static void * system_realloc(kll_allocator *allocator, void *ptr, fckc_size_t size, const char* file, fckc_size_t line)
{
	if (ptr == NULL && size == 0)
	{
		return NULL;
	}
	// NULL && N > 0 == malloc
	// non-NULL && N > 0 == realloc
	// non-NULL && N == 0 == free
	return realloc(ptr, size);
}

static kll_allocator system_allocator = (kll_allocator){system_realloc};

kll_allocator *kll_system= &system_allocator;
