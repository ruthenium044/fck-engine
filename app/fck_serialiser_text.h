#ifndef FCK_SERIALISER_TEXT_H_INCLUDED
#define FCK_SERIALISER_TEXT_H_INCLUDED

#include <fckc_inttypes.h>

struct fck_serialiser;
struct kll_allocator;

typedef struct fck_serialiser_text_api
{
	struct fck_serialiser *(*writer)(struct kll_allocator *allocator, fckc_size_t capacity);
	struct fck_serialiser *(*reader)(struct kll_allocator *allocator, const fckc_char *buffer);
} fck_serialiser_text_api;

extern fck_serialiser_text_api *serialiser_text;

#endif // !FCK_SERIALISER_TEXT_H_INCLUDED
