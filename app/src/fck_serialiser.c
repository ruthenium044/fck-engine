
#include "fck_serialiser.h"

#include <kll.h>
#include <kll_heap.h>
#include <kll_malloc.h>

#include <SDL3/SDL_assert.h>

static fckc_u64 fck_serialiser_buffer_next_capacity(fckc_u64 n)
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

fck_memory_serialiser fck_memory_serialiser_create(struct fck_serialiser_vt *vt, fckc_u8 *bytes, fckc_size_t capacity)
{
	fck_memory_serialiser serialiser;
	serialiser.vt = vt;
	serialiser.allocator = NULL;
	serialiser.capacity = capacity;
	serialiser.at = 0;
	serialiser.bytes = bytes;
	return serialiser;
};

fck_memory_serialiser fck_memory_serialiser_alloc(struct kll_allocator *allocator, struct fck_serialiser_vt *vt, fckc_size_t capacity)
{
	fck_memory_serialiser serialiser;
	serialiser.vt = vt;
	serialiser.allocator = allocator;
	serialiser.capacity = capacity;
	serialiser.at = 0;
	serialiser.bytes = kll_malloc(allocator, capacity);
	return serialiser;
};

void fck_memory_serialiser_realloc(fck_memory_serialiser *serialiser, fckc_size_t capacity)
{
	SDL_assert(serialiser);
	SDL_assert(serialiser->allocator);

	fckc_u8 *bytes = (fckc_u8 *)kll_malloc(serialiser->allocator, capacity);
	SDL_memcpy(bytes, serialiser->allocator, SDL_min(capacity, serialiser->capacity));

	kll_free(serialiser->allocator, serialiser->bytes);
	serialiser->capacity = capacity;
	serialiser->bytes = bytes;
};

void fck_memory_serialiser_maybe_realloc(fck_memory_serialiser *serialiser, fckc_size_t extra)
{
	SDL_assert(serialiser);

	fckc_size_t next_capacity = serialiser->at + extra;
	if (next_capacity > serialiser->capacity)
	{
		SDL_assert(serialiser->allocator);
		next_capacity = fck_serialiser_buffer_next_capacity(next_capacity);
		fck_memory_serialiser_realloc(serialiser, next_capacity);
	}
};

void fck_memory_serialiser_free(fck_memory_serialiser *serialiser)
{
	SDL_assert(serialiser);
	SDL_assert(serialiser->allocator);

	kll_free(serialiser->allocator, serialiser->bytes);

	serialiser->allocator = NULL;
	serialiser->vt = NULL;
	serialiser->bytes = NULL;
	serialiser->at = serialiser->capacity = 0;
};