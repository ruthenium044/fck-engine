// kll_scratch.h

#include "kll_scratch.h"

#include "kll.h"
#include "kll_impl_util.h"

#include <assert.h>

typedef struct kll_scratch_context
{
	uint8_t *memory;
	size_t count;
	size_t capacity;
	size_t alignment;
} kll_scratch_context;

typedef struct kll_scratch_allocator
{
	// Base first
	kll_allocator allocator;
	kll_scratch_context context;
} kll_scratch_allocator;

static size_t align_size(size_t size, size_t alignment)
{
	return (size + alignment - 1) & ~(alignment - 1);
}

static void *scratch_realloc(kll_scratch_context *context, void *ptr, size_t size, size_t line, const char *file)
{
	size_t count = align_size(size, context->alignment);
	assert(context->count + count <= context->capacity);

	unsigned char *target = context->memory + context->count;
	context->count = context->count + count;
	return (void *)target;
}

kll_allocator *kll_scratch_create(void *memory, size_t size, size_t alignment)
{
	kll_scratch_allocator *scratch_allocator = (kll_scratch_allocator *)memory;
	if (size < sizeof(*scratch_allocator))
	{
		return NULL;
	}

	scratch_allocator->context.memory = (uint8_t *)memory;
	scratch_allocator->context.capacity = size;
	scratch_allocator->context.count = sizeof(*scratch_allocator);
	scratch_allocator->context.alignment = alignment;

	scratch_allocator->allocator.context = (kll_context *)&scratch_allocator->context;
	scratch_allocator->allocator.vt.realloc = (kll_realloc_function *)scratch_realloc;

	return &scratch_allocator->allocator;
}

kll_allocator *kll_scratch_reset(kll_allocator *allocator)
{
	kll_scratch_allocator *scratch_allocator = (kll_scratch_allocator *)allocator;
	scratch_allocator->context.count = sizeof(*scratch_allocator);
	return allocator;
}
