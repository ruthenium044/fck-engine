// kll.h
#ifndef FCK_KLL_H_INCLUDED
#define FCK_KLL_H_INCLUDED

#include <fckc_inttypes.h>

#define kll_api_name "kll"

struct kll_allocator;

typedef void *(kll_realloc_function)(struct kll_allocator * allocator, void *ptr, fckc_size_t size, const char *file, fckc_size_t line);

typedef struct kll_allocator
{
	kll_realloc_function *realloc;
	// User data follows!
} kll_allocator;

struct kll_arena;

typedef void *(kll_arena_reset_function)(struct kll_arena * arena);
typedef struct kll_arena
{
	kll_realloc_function *realloc;
	kll_arena_reset_function *reset;
	// User data follows!
} kll_arena;

struct kll_arena_api
{
	kll_arena *(*create)(kll_allocator *allocator);
	void (*destroy)(kll_arena *arena);

	char *(*format)(struct kll_arena*arena, const char *format, ...);
};

struct kll_api
{
	struct kll_arena_api *arena;

	kll_allocator *system;
	kll_arena *frame;
};

#endif // !FCK_KLL_H_INCLUDED
