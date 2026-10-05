#ifndef FCK_DYNARR_H_INCLUDED
#define FCK_DYNARR_H_INCLUDED

#ifndef FCK_KLL_H_INCLUDED
#error kll.h not included before set
#endif // !FCK_KLL_H_INCLUDED

#ifndef FCK_KLL_MALLOC_H_INCLUDED
#error kll_malloc.h not included before set
#endif // !FCK_KLL_MALLOC_H_INCLUDED

#include <fckc_inttypes.h>

struct kll_allocator;

// TODO: Align, add data for padding before, not after!
typedef struct fck_dynarr_info
{
	struct kll_allocator* allocator;
	fckc_size_t element_size;
	fckc_u32 capacity;
	fckc_u32 size;
} fck_dynarr_info;

// Ok, this is all a bit bs, 
// We should do a [-1] on the ptr to get the count... YES! YES! That is it.
inline void* fck_dynarr_alloc(struct kll_allocator* allocator, fckc_size_t element_size, fckc_size_t capacity);
inline fck_dynarr_info* fck_dynarr_get_info(void* ptr);
inline void fck_dynarr_free(void* ptr);
inline fckc_size_t fck_dynarr_count(void* ptr);
inline void fck_dynarr_expand(void** ref_ptr, fckc_size_t element_size);

#define fck_dynarr_new(type, allocator, size) (type *)fck_dynarr_alloc((allocator), (sizeof(type)), (size))
#define fck_dynarr_destroy(ptr) fck_dynarr_free(ptr)

#define fck_dynarr_add(ptr, value) fck_dynarr_expand((void **)&(ptr), sizeof(value)), (ptr)[fck_dynarr_count(ptr) - 1] = value
#define fck_dynarr_clear(ptr) fck_dynarr_get_info(ptr)->size = 0

#include <fck_dynarr.inl>

#endif // !FCK_DYNARR_H_INCLUDED
