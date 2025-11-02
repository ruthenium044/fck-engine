// kll.h
#ifndef FCK_KLL_H_INCLUDED
#define FCK_KLL_H_INCLUDED

#include <fckc_inttypes.h>

typedef struct kll_context kll_context;

typedef void *(kll_realloc_function)(kll_context *context, void *ptr, fckc_size_t size, fckc_size_t line, const char *file);

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

typedef void(kll_reset_func)(kll_context *context);

typedef struct kll_temp_vt
{
	// char *(*format)(kll_context *context);
	void (*reset)(kll_context *context);
} kll_temp_vt;

typedef struct kll_temp_allocator
{
	// Opaque pointer for custom context
	kll_context *context;
	// VTable without indirection cause only one pointer.
	kll_vt vt;
	kll_temp_vt temp;
} kll_temp_allocator;

#endif // !FCK_KLL_H_INCLUDED
