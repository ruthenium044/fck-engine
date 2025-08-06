#ifndef FCK_SERIALISER_H_INCLUDED
#define FCK_SERIALISER_H_INCLUDED

#include <fckc_inttypes.h>

typedef struct fck_serialiser
{
	struct fck_serialiser_vt *vt;
	struct kll_allocator *allocator;

	fckc_size_t capacity;
	fckc_size_t at;

	fckc_u8 *bytes;
	void* user;
} fck_serialiser;

typedef struct fck_serialiser_params
{
	const char* name;
	void* user;
	// ...

} fck_serialiser_params;

fck_serialiser fck_serialiser_create(struct fck_serialiser_vt *vt, fckc_u8 *bytes, fckc_size_t capacity);
fck_serialiser fck_serialiser_alloc(struct kll_allocator *allocator, struct fck_serialiser_vt *vt, fckc_size_t capacity);
void fck_serialiser_realloc(fck_serialiser *serialiser, fckc_size_t capacity);
void fck_serialiser_maybe_realloc(fck_serialiser* serialiser, fckc_size_t extra);
void fck_serialiser_free(fck_serialiser *serialiser);

#endif // !FCK_SERIALISER_H_INCLUDED
