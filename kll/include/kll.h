// kll.h
#ifndef FCK_KLL_H_INCLUDED
#define FCK_KLL_H_INCLUDED

#include <inttypes.h>

typedef struct kll_context kll_context;

typedef void *(kll_realloc_function)(kll_context *context, void *ptr, size_t size, size_t line, const char *file);

typedef struct kll_vt
{
	kll_realloc_function *realloc;
} kll_vt;

typedef struct kll_allocator
{
	// Opaque pointer for custom context
	kll_context *context;
	// VTable without indirection cause only one pointer.
	kll_vt vt;
} kll_allocator;

extern kll_allocator *kll_heap;

#endif // !FCK_KLL_H_INCLUDED
