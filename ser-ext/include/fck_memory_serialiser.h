#ifndef FCK_MEMORY_SERIALISER_H_INCLUDED
#define FCK_MEMORY_SERIALISER_H_INCLUDED

#include <fckc_inttypes.h>
#include <fckc_apidef.h>

#if defined(FCK_SER_EXT_MEM_EXPORT)
#define FCK_SER_EXT_MEM_API FCK_EXPORT_API
#else
#define FCK_SER_EXT_MEM_API FCK_IMPORT_API
#endif

// TODO: Rename this to memory serialiser...
// Or buffer serialiser?

typedef struct fck_memory_serialiser
{
	struct fck_serialiser_vt *vt;
	struct kll_allocator *allocator;

	fckc_size_t capacity;
	fckc_size_t at;

	fckc_u8 *bytes;
} fck_memory_serialiser;

typedef struct fck_memory_serialiser_api
{
	fck_memory_serialiser (*create)(struct fck_serialiser_vt *vt, fckc_u8 *bytes, fckc_size_t capacity);
	fck_memory_serialiser (*alloc)(struct kll_allocator *allocator, struct fck_serialiser_vt *vt, fckc_size_t capacity);
	void (*realloc)(fck_memory_serialiser *serialiser, fckc_size_t capacity);
	void (*maybe_realloc)(fck_memory_serialiser *serialiser, fckc_size_t extra);
	void (*free)(fck_memory_serialiser *serialiser);
}fck_memory_serialiser_api;

FCK_SER_EXT_MEM_API extern fck_memory_serialiser_api* fck_ser_mem;


#endif // !FCK_MEMORY_SERIALISER_H_INCLUDED
