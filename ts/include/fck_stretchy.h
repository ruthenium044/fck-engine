#ifndef FCK_STRETCHY_H_INCLUDED
#define FCK_STRETCHY_H_INCLUDED

#include <fckc_inttypes.h>
#include <fckc_apidef.h>

#if defined(FCK_TS_STRETCHY_EXPORT)
#define FCK_TS_STRETCHY_API FCK_EXPORT_API
#else
#define FCK_TS_STRETCHY_API FCK_IMPORT_API
#endif

struct kll_allocator;

// TODO: Rename this junk! Really, rename this shit lol
typedef struct fck_stretchy_info
{
	struct kll_allocator *allocator;
	fckc_size_t element_size;
	fckc_u32 capacity;
	fckc_u32 size;
} fck_stretchy_info;

FCK_TS_STRETCHY_API void *fck_stretchy_alloc(struct kll_allocator *allocator, fckc_size_t element_size, fckc_size_t capacity);
FCK_TS_STRETCHY_API fck_stretchy_info *fck_stretchy_get_info(void *ptr);
FCK_TS_STRETCHY_API void fck_stretchy_free(void *ptr);
FCK_TS_STRETCHY_API fckc_size_t fck_stretchy_size(void *ptr);
FCK_TS_STRETCHY_API void fck_stretchy_expand(void **ref_ptr, fckc_size_t element_size);

// TODO: sizeof(*ptr) > sizeof(fck_stretchy_info) not handled!!
#define fck_stretchy_new(type, allocator, size) (type *)fck_stretchy_alloc((allocator), (sizeof(type)), (size))
#define fck_stretchy_destroy(ptr) fck_stretchy_free(ptr)

#define fck_stretchy_add(ptr, value) fck_stretchy_expand((void **)&(ptr), sizeof(value)), (ptr)[fck_stretchy_size(ptr) - 1] = value
#define fck_stretchy_clear(ptr) fck_stretchy_get_info(ptr)->size = 0

#endif // FCK_STRETCHY_H_INCLUDED