// kll_scratch.h

#include "kll_temp.h"

#include "kll.h"
#include "kll_impl_util.h"
#include "kll_malloc.h"

#include <assert.h>

typedef struct kll_pointer_lookup_element
{
	void *pointer;
	size_t size;
} kll_pointer_lookup_element;

typedef struct kll_pointer_lookup
{
	uint64_t *hints;
	kll_pointer_lookup_element *elements;

	size_t count;
	size_t capacity;
} kll_pointer_lookup;

kll_pointer_lookup *kll_pointer_lookup_alloc(kll_allocator *allocator, size_t capacity)
{
	kll_pointer_lookup *lookup;

	const size_t hint_capacity = sizeof(*lookup->hints) * capacity;
	const size_t elements_capacity = sizeof(*lookup->elements) * capacity;
	
	lookup = (kll_pointer_lookup *)kll_malloc(allocator, sizeof(*lookup) + hint_capacity + elements_capacity);
	
	return NULL;
}

typedef struct kll_temp_context
{
	uint8_t *memory;
	size_t capacity;
} kll_temp_context;

typedef struct kll_temp_allocator
{
	// Base first
	kll_allocator allocator;
	kll_temp_context context;
} kll_temp_allocator;
//
//static size_t align_size(size_t size, size_t alignment)
//{
//	return (size + alignment - 1) & ~(alignment - 1);
//}
//
//static void *temp_realloc(kll_scratch_context *context, void *ptr, size_t size, size_t line, const char *file)
//{
//	size_t count = align_size(size, context->alignment);
//	assert(context->count + count <= context->capacity);
//
//	unsigned char *target = context->memory + context->count;
//	context->count = context->count + count;
//	return (void *)target;
//}
//
//kll_allocator *kll_temp_create(kll_allocator *parent)
//{
//}
//
//kll_allocator *kll_temp_reset(kll_allocator *allocator)
//{
//
//	return allocator;
//}
