#ifndef FCK_MEMORY_SERIALISER_H_INCLUDED
#define FCK_MEMORY_SERIALISER_H_INCLUDED

#include <fckc_inttypes.h>

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

fck_memory_serialiser fck_memory_serialiser_create(struct fck_serialiser_vt *vt, fckc_u8 *bytes, fckc_size_t capacity);
fck_memory_serialiser fck_memory_serialiser_alloc(struct kll_allocator *allocator, struct fck_serialiser_vt *vt, fckc_size_t capacity);
void fck_memory_serialiser_realloc(fck_memory_serialiser *serialiser, fckc_size_t capacity);
void fck_memory_serialiser_maybe_realloc(fck_memory_serialiser *serialiser, fckc_size_t extra);
void fck_memory_serialiser_free(fck_memory_serialiser *serialiser);

#endif // !FCK_MEMORY_SERIALISER_H_INCLUDED
