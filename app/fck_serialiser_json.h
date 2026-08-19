#ifndef FCK_SERIALISER_JSON_H_INCLUDED
#define FCK_SERIALISER_JSON_H_INCLUDED

#include <fckc_inttypes.h>

struct fck_serialiser;
struct kll_allocator;

// This one is porbably full with leaks lol

typedef struct fck_serialiser_json_api
{
	struct fck_serialiser *(*writer)(struct kll_allocator *allocator);
	struct fck_serialiser *(*reader)(struct kll_allocator *allocator, const fckc_char *buffer, fckc_size_t length);
} fck_serialiser_json_api;

extern fck_serialiser_json_api *serialiser_json;

#endif // !FCK_SERIALISER_JSON_H_INCLUDED