// kll.h
// kll.h
#ifndef FCK_KLL_H_INCLUDED
#define FCK_KLL_H_INCLUDED

#include <fckc_inttypes.h>
#include <fckc_apidef.h>

#define kll_api_name "kll"

#if defined(FCK_KLL_EXPORT)
#define FCK_KLL_API FCK_EXPORT_API
#else
#define FCK_KLL_API FCK_IMPORT_API
#endif


struct kll_allocator;

typedef void *(kll_realloc_function)(struct kll_allocator * allocator, void *ptr, fckc_size_t size, const char *file, fckc_size_t line);

typedef struct kll_allocator
{
	kll_realloc_function *realloc;
	// User data follows!
} kll_allocator;

struct kll_arena;

typedef struct kll_arena
{
	kll_realloc_function *realloc;
	void (*reset)(struct kll_arena* arena);
	const char* (*format)(struct kll_arena* arena, const char* format, ...);
	// User data follows!
} kll_arena;

// TODO: we have no interface for alignment yet
typedef struct kll_arena_api
{
	kll_arena *(*create)(kll_allocator *allocator, fckc_size_t capacity);
	void (*destroy)(kll_arena *arena);
} kll_arena_api;

typedef struct kll_api
{
	struct kll_arena_api *arena;

	kll_allocator *system;
} kll_api;

FCK_KLL_API extern struct kll_api* kll;

#endif // !FCK_KLL_H_INCLUDED
