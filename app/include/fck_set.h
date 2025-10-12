#ifndef FCK_SET_H_INCLUDED
#define FCK_SET_H_INCLUDED

#include <fckc_inttypes.h>

typedef enum fck_set_key_state
{
	FCK_SET_KEY_EMPTY = 0, // 00
	FCK_SET_KEY_TAKEN = 1, // 01
	FCK_SET_KEY_STALE = 2, // 10 (11 is unsed... - 1 wasted bit)
} fck_set_key_state;

typedef struct fck_set_state
{
	fckc_u64 state; // Bits for 16 elements...
} fck_set_state;

typedef struct fck_set_key
{
	fckc_u64 hash; // Good enough
	// TODO: Maybe a string representation
} fck_set_key;

typedef struct fck_set_info
{
	struct kll_allocator *allocator;
	fckc_size_t el_align;
	fckc_size_t el_size;
	fckc_size_t capacity;
	fckc_size_t size;


	fck_set_key* keys;
} fck_set_info;

#endif // FCK_SET_H_INCLUDED