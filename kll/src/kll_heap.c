// kll_heap.c

#include "kll_heap.h"

#include "kll.h"
#include "kll_impl_util.h"

#include <fckc_inttypes.h>

#include <stdlib.h>

static void *heap_realloc(kll_context *context, void *ptr, fckc_size_t size, fckc_size_t line, const char *file)
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

static kll_allocator heap = kll_make_allocator(NULL, heap_realloc);

kll_allocator *kll_heap = &heap;
