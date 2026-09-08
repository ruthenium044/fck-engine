
#define FCK_KLL_EXPORT
#include "kll.h"
#include "kll_malloc.h"

#include <fckc_inttypes.h>

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <fckc_assert.h>
#include <fck_os.h>

struct kll_memory_buffer_page;
typedef struct kll_memory_buffer_page
{
	fckc_size_t capacity;
	fckc_size_t count;

	struct kll_memory_buffer_page *prev;

	fckc_u8 values[1];
} kll_memory_buffer_page;

typedef struct kll_memory_buffer
{
	kll_memory_buffer_page *current;
} kll_memory_buffer;

static kll_memory_buffer_page *kll_memory_buffer_page_alloc(kll_allocator *allocator, fckc_size_t size)
{
	const fckc_size_t total = offsetof(kll_memory_buffer_page, values[size]);
	kll_memory_buffer_page *page = (kll_memory_buffer_page *)kll_malloc(allocator, total);
	page->capacity = size;
	page->count = 0;
	page->prev = NULL;
	return page;
}

static void *kll_memory_buffer_page_push(kll_memory_buffer_page *page, fckc_size_t size)
{
	if (page->count + size >= page->capacity)
	{
		return NULL;
	}

	void *destination = page->values + page->count;
	// Maybe ifndef this bad boy - AF == as fuck!
	memset(destination, 0xAF, size);
	page->count = page->count + size;
	return destination;
}

static void kll_memory_buffer_page_free(kll_allocator *allocator, kll_memory_buffer_page *page)
{
	kll_free(allocator, page);
}

static void kll_memory_buffer_create(kll_allocator *allocator, kll_memory_buffer *buffer, fckc_size_t size)
{
	buffer->current = kll_memory_buffer_page_alloc(allocator, size);
}

static fckc_size_t kll_memory_buffer_destroy(kll_allocator *allocator, kll_memory_buffer *buffer)
{
	fckc_size_t count = 0;
	kll_memory_buffer_page *current = buffer->current;
	while (current)
	{
		count = count + current->count;
		kll_memory_buffer_page *prev = current->prev;
		kll_memory_buffer_page_free(allocator, current);
		current = prev;
	}
	buffer->current = NULL;
	return count;
}

static void *kll_memory_buffer_push(kll_allocator *allocator, kll_memory_buffer *buffer, fckc_size_t size)
{
	kll_memory_buffer_page *current = buffer->current;
	if (current->count + size >= current->capacity)
	{
		fckc_size_t capacity = current->capacity * 2;
		if (capacity < size)
		{
			// If this is happening, I swear to God
			capacity = size * 2;
		}

		buffer->current = kll_memory_buffer_page_alloc(allocator, capacity);
		buffer->current->prev = current;
	}

	void *result = kll_memory_buffer_page_push(buffer->current, size);
	return result;
}

typedef struct kLl_arena_implementation
{
	kll_arena base;
	kll_allocator *allocator;
	kll_memory_buffer buffer;
} kLl_arena_implementation;

// kll_allocator is base of kll_arena - Polymorphic in layout
static void *kll_arena_realloc(struct kll_allocator *arena, void *ptr, fckc_size_t size, const char *file, fckc_size_t line)
{
	if (size == 0)
	{
		return NULL;
	}
	
	fck_assert(ptr == NULL && "Not supporting realloc in arena yet");

	(void)file;
	(void)line;

	// This arena (or allocator for this sake) never frees. It is bump-y
	(void)ptr;
	kLl_arena_implementation *impl = (kLl_arena_implementation *)arena;

	void *result = kll_memory_buffer_push(impl->allocator, &impl->buffer, size);
	return result;
}

static void kll_arena_reset(struct kll_arena *arena)
{
	kLl_arena_implementation *impl = (kLl_arena_implementation *)arena;

	const fckc_size_t allocated = kll_memory_buffer_destroy(impl->allocator, &impl->buffer);
	kll_memory_buffer_create(impl->allocator, &impl->buffer, allocated);
}

static const char *kll_arena_format(struct kll_arena *arena, const char *format, ...)
{
	kLl_arena_implementation *impl = (kLl_arena_implementation *)arena;

	va_list args;
	va_start(args, format);

	va_list args_copy;
	va_copy(args_copy, args);

	const int required_size = vsnprintf(NULL, 0, format, args_copy);
	va_end(args_copy);
	if (required_size < 0)
	{
		va_end(args);
		return NULL;
	}

	const fckc_size_t alloc_size = (fckc_size_t)required_size + 1;
	char *string_buffer = (char *)kll_malloc(arena, alloc_size);

	if (string_buffer == NULL)
	{
		va_end(args);
		return NULL;
	}

	vsnprintf(string_buffer, alloc_size, format, args);
	va_end(args);

	return string_buffer;
}

static kll_arena *kll_arena_api_create(kll_allocator *allocator, fckc_size_t capacity)
{
	kLl_arena_implementation *impl = (kLl_arena_implementation *)kll_malloc(allocator, sizeof(*impl));
	impl->allocator = allocator;
	kll_memory_buffer_create(impl->allocator, &impl->buffer, capacity);
	impl->base.realloc = kll_arena_realloc;
	impl->base.reset = kll_arena_reset;
	impl->base.format = kll_arena_format;
	return &impl->base;
}

static void kll_arena_api_create_destroy(kll_arena *arena)
{
	kLl_arena_implementation *impl = (kLl_arena_implementation *)arena;
	kll_memory_buffer_destroy(impl->allocator, &impl->buffer);
	kll_free(impl->allocator, impl);
}

static kll_arena_api arena_api = {
	.create = kll_arena_api_create,
	.destroy = kll_arena_api_create_destroy,
};

#include <stdlib.h>

static void *system_realloc(kll_allocator *allocator, void *ptr, fckc_size_t size, const char *file, fckc_size_t line)
{
	(void)allocator;
	(void)file;
	(void)line;

	if (ptr == NULL && size == 0)
	{
		return NULL;
	}
	if (size == 0)
	{
		free(ptr);
		return NULL;
	}
	// NULL && N > 0 == malloc
	// non-NULL && N > 0 == realloc
	// non-NULL && N == 0 == free
	return realloc(ptr, size);
}

static kll_allocator system_allocator = {system_realloc};

static kll_api kll_api_implementation = {
	.arena = &arena_api,
	.system = &system_allocator,
};

kll_api *kll = &kll_api_implementation;
