#ifndef FCK_SERIALISER_EXT_H_INCLUDED
#define FCK_SERIALISER_EXT_H_INCLUDED

#include <fckc_inttypes.h>

struct fck_serialiser_vt;

typedef struct fck_memory_serialiser
{
	struct fck_serialiser_vt* vt;
	struct kll_allocator* allocator;

	fckc_size_t capacity;
	fckc_size_t at;

	fckc_u8* bytes;
} fck_memory_serialiser;

typedef struct fck_serialiser_byte_vts
{
	struct fck_serialiser_vt *writer;
	struct fck_serialiser_vt *reader;
} fck_serialiser_byte_vts;

typedef struct fck_serialiser_string_vts
{
	struct fck_serialiser_vt *writer;
	struct fck_serialiser_vt *reader;
} fck_serialiser_string_vts;

typedef struct fck_serialiser_ext_vts
{
	fck_serialiser_byte_vts byte;
	fck_serialiser_string_vts string;
} fck_serialiser_ext_vts;

typedef struct fck_serialiser_ext_api
{
	fck_serialiser_ext_vts vts;

	fck_memory_serialiser(*create)(struct fck_serialiser_vt* vt, fckc_u8* bytes, fckc_size_t capacity);
	fck_memory_serialiser(*alloc)(struct kll_allocator* allocator, struct fck_serialiser_vt* vt, fckc_size_t capacity);
	void (*realloc)(fck_memory_serialiser* serialiser, fckc_size_t capacity);
	void (*maybe_realloc)(fck_memory_serialiser* serialiser, fckc_size_t extra);
	void (*free)(fck_memory_serialiser* serialiser);
}fck_serialiser_ext_api;



#endif // !FCK_SERIALISER_EXT_H_INCLUDED
