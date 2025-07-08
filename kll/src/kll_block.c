// kll_block.cpp

#include "kll_block.h"
#include "kll.h"
#include "kll_impl_util.h"
#include "kll_malloc.h"
#include <malloc.h>

typedef struct kll_memory_block
{
	uint8_t *data;
	size_t size;
} kll_memory_block;

typedef struct kll_block_allocator_context
{
	kll_allocator *parent;

	size_t count;
	size_t capacity;

	kll_memory_block *blocks;
} kll_block_allocator_context;

typedef struct kll_block_allocator
{
	kll_allocator allocator;
	kll_block_allocator_context context;
} kll_block_allocator;

static size_t next_capacity(size_t n)
{
	if (n == 0)
		return 1;

	n--;
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	n |= n >> 32;
	n++;

	return n;
}

kll_memory_block *kll_memory_block_create(kll_allocator *allocator, size_t size)
{
	kll_memory_block *block = NULL;
	return block;
}

static void *block_malloc(kll_block_allocator_context *allocator, size_t size)
{
	return NULL;
}

static void block_free(kll_block_allocator_context *allocator, void *ptr)
{
}

static void *block_realloc(kll_block_allocator_context *allocator, void *ptr, size_t size)
{
	return NULL;
}

static void *block_allocator_realloc(kll_context* context, void *ptr, size_t size, size_t line, const char *file)
{
	kll_block_allocator_context* allocator = (kll_block_allocator_context*)context;

	if (ptr == NULL && size == 0)
	{
		return NULL;
	}
	if (ptr == NULL)
	{
		return block_malloc(allocator, size);
	}
	if (size == 0)
	{
		block_free(allocator, ptr);
		return NULL;
	}
	return block_realloc(allocator, ptr, size);
}

kll_block_allocator *kll_block_alloc(kll_allocator *parent)
{
	// TODO: Create initial blocks!
	kll_block_allocator *block_allocator;
	const size_t block_size = 0;
	const size_t total_size = sizeof(*block_allocator) + block_size;

	const size_t block_allocator_offset = 0;
	const size_t blocks_offset = sizeof(*block_allocator);

	uint8_t *memory = (uint8_t *)kll_malloc(parent, total_size);
	block_allocator = (kll_block_allocator *)(memory + block_allocator_offset);
	block_allocator->allocator.context = (kll_context*)(&block_allocator->context);
	block_allocator->allocator.vt.realloc = block_allocator_realloc;
	block_allocator->context.blocks = (kll_memory_block *)(memory + blocks_offset);
	block_allocator->context.capacity = 0;
	block_allocator->context.count = 0;
	block_allocator->context.parent = parent;
	return block_allocator;
}

void kll_block_free(kll_block_allocator *alloactor)
{
	// Free all the blocks
	kll_free(alloactor->context.parent, alloactor);
}
