#define FCK_TS_DYNARR_EXPORT
#include "fck_dynarr.h"

#include <kll.h>
#include <kll_malloc.h>

#include <assert.h>
#include <fck_os.h>

void *fck_dynarr_alloc(kll_allocator *allocator, fckc_size_t element_size, fckc_size_t capacity)
{
	fck_dynarr_info *info;
	fckc_size_t header_size = sizeof(*info);
	fckc_u8 *buffer = (fckc_u8 *)kll_malloc(allocator, header_size + (capacity * element_size));

	info = (fck_dynarr_info *)&buffer[0];
	info->allocator = allocator;
	info->element_size = element_size;
	info->capacity = capacity;
	info->size = 0;

	void *ptr = (void *)(((fckc_size_t)&buffer[sizeof(*info)]));
	return ptr;
}

fck_dynarr_info *fck_dynarr_get_info(void *ptr)
{
	fckc_u8 *buffer = (fckc_u8 *)ptr;
	fck_dynarr_info *info = (fck_dynarr_info *)(buffer - sizeof(*info));
	return info;
}

void fck_dynarr_free(void *ptr)
{
	fck_dynarr_info *info = fck_dynarr_get_info(ptr);
	kll_free(info->allocator, info);
}

fckc_size_t fck_dynarr_size(void *ptr)
{
	fck_dynarr_info *info = fck_dynarr_get_info(ptr);
	return info->size;
}

void fck_dynarr_realloc(void **ref_ptr, fckc_size_t extra)
{
	if (extra == 0)
	{
		return;
	}

	fck_dynarr_info *info = fck_dynarr_get_info(*ref_ptr);

	fckc_size_t next_size = info->size + extra;
	if (info->size != 0)
	{
		next_size--;
		next_size |= next_size >> 1;
		next_size |= next_size >> 2;
		next_size |= next_size >> 4;
		next_size |= next_size >> 8;
		next_size |= next_size >> 16;
		next_size |= next_size >> 32;
		next_size++;
	}

	void *result = fck_dynarr_alloc(info->allocator, info->element_size, next_size);
	fck_dynarr_info *result_info = fck_dynarr_get_info(result);

	result_info->size = info->size;
	os->mem->cpy(result, *ref_ptr, info->size * info->element_size);
	fck_dynarr_free(*ref_ptr);
	*ref_ptr = result;
}

void fck_dynarr_expand(void **ref_ptr, fckc_size_t element_size)
{
	fck_dynarr_info *info = fck_dynarr_get_info(*ref_ptr);
	assert(element_size == info->element_size);

	if (info->size >= info->capacity)
	{
		fck_dynarr_realloc(ref_ptr, 1);
		// Overwrite info and ref_ptr since realloc happened
		info = fck_dynarr_get_info(*ref_ptr);
	}

	fckc_size_t offset = info->element_size * info->size;
	fckc_u8 *dst = ((fckc_u8 *)*ref_ptr) + offset;
	info->size = info->size + 1;
}

FCK_EXPORT_API int fck_main()
{
	return 0;
}
