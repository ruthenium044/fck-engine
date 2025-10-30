#include "fck_serialiser_ext.h"

#include <fck_apis.h>
#include <fckc_apidef.h>

#include <kll.h>
#include <kll_heap.h>
#include <kll_malloc.h>

#include <fck_os.h>
#include <fckc_math.h>

#include <fckc_assert.h>

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
	serialiser.bytes = (fckc_u8 *)kll_malloc(allocator, capacity);
	return serialiser;
};

void fck_memory_serialiser_realloc(fck_memory_serialiser *serialiser, fckc_size_t capacity)
{
	assert(serialiser);
	assert(serialiser->allocator);

	fckc_u8 *bytes = (fckc_u8 *)kll_malloc(serialiser->allocator, capacity);
	os->mem->cpy(bytes, serialiser->allocator, fck_min(capacity, serialiser->capacity));

	kll_free(serialiser->allocator, serialiser->bytes);
	serialiser->capacity = capacity;
	serialiser->bytes = bytes;
};

void fck_memory_serialiser_maybe_realloc(fck_memory_serialiser *serialiser, fckc_size_t extra)
{
	assert(serialiser);

	fckc_size_t next_capacity = serialiser->at + extra;
	if (next_capacity > serialiser->capacity)
	{
		assert(serialiser->allocator);
		next_capacity = fck_serialiser_buffer_next_capacity(next_capacity);
		fck_memory_serialiser_realloc(serialiser, next_capacity);
	}
};

void fck_memory_serialiser_free(fck_memory_serialiser *serialiser)
{
	assert(serialiser);
	assert(serialiser->allocator);

	kll_free(serialiser->allocator, serialiser->bytes);

	serialiser->allocator = NULL;
	serialiser->vt = NULL;
	serialiser->bytes = NULL;
	serialiser->at = serialiser->capacity = 0;
};


extern struct fck_serialiser_vt* fck_byte_writer_vt;
extern struct fck_serialiser_vt* fck_byte_reader_vt;

extern struct fck_serialiser_vt* fck_string_writer_vt;
extern struct fck_serialiser_vt* fck_string_reader_vt;

static fck_serialiser_ext_api fck_ser_mem_api = {
	.alloc = fck_memory_serialiser_alloc,
	.create = fck_memory_serialiser_create,
	.free = fck_memory_serialiser_free,
	.maybe_realloc = fck_memory_serialiser_maybe_realloc,
	.realloc = fck_memory_serialiser_realloc,
};

fck_serialiser_ext_api *fck_ser_mem = &fck_ser_mem_api;

FCK_EXPORT_API fck_serialiser_ext_api *fck_main(fck_api_registry *apis, void *params)
{
	fck_ser_mem_api.vts.byte.reader = fck_byte_reader_vt;
	fck_ser_mem_api.vts.byte.writer = fck_byte_writer_vt;
	fck_ser_mem_api.vts.string.reader = fck_string_reader_vt;
	fck_ser_mem_api.vts.string.writer = fck_string_writer_vt;

	apis->add("FCK_SERIALISER_EXT", &fck_ser_mem_api);
	return &fck_ser_mem_api;
}