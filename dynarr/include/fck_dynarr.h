#ifndef FCK_DYNARR_H_INCLUDED
#define FCK_DYNARR_H_INCLUDED

#include <fckc_inttypes.h>
#include <fckc_apidef.h>

#if defined(FCK_TS_DYNARR_EXPORT)
#define FCK_TS_DYNARR_API FCK_EXPORT_API
#else
#define FCK_TS_DYNARR_API FCK_IMPORT_API
#endif

struct kll_allocator;

// TODO: Rename this junk! Really, rename this shit lol
// TODO: Align, add slop data for padding before, not after!
typedef struct fck_dynarr_info
{
	struct kll_allocator *allocator;
	fckc_size_t element_size;
	fckc_u32 capacity;
	fckc_u32 size;
} fck_dynarr_info;

FCK_TS_DYNARR_API void *fck_dynarr_alloc(struct kll_allocator *allocator, fckc_size_t element_size, fckc_size_t capacity);
FCK_TS_DYNARR_API fck_dynarr_info *fck_dynarr_get_info(void *ptr);
FCK_TS_DYNARR_API void fck_dynarr_free(void *ptr);
FCK_TS_DYNARR_API fckc_size_t fck_dynarr_size(void *ptr);
FCK_TS_DYNARR_API void fck_dynarr_expand(void **ref_ptr, fckc_size_t element_size);


// TODO: sizeof(*ptr) > sizeof(fck_dynarr_info) not handled!!
#define fck_dynarr_new(type, allocator, size) (type *)fck_dynarr_alloc((allocator), (sizeof(type)), (size))
#define fck_dynarr_destroy(ptr) fck_dynarr_free(ptr)

#define fck_dynarr_add(ptr, value) fck_dynarr_expand((void **)&(ptr), sizeof(value)), (ptr)[fck_dynarr_size(ptr) - 1] = value
#define fck_dynarr_clear(ptr) fck_dynarr_get_info(ptr)->size = 0

#endif // FCK_DYNARR_H_INCLUDED