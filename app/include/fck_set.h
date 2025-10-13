#ifndef FCK_SET_H_INCLUDED
#define FCK_SET_H_INCLUDED

#include <fckc_inttypes.h>

struct fck_set_key;
struct fck_set_state;

typedef struct fck_set_info
{
	struct kll_allocator *allocator;
	fckc_size_t el_align;
	fckc_size_t el_size;
	fckc_size_t capacity;
	fckc_size_t size;

	struct fck_set_key *keys; // sizeof(fck_set_info) + (el_size * capacity)
	struct fck_set_state *states;
} fck_set_info;

fck_set_info *fck_opaque_set_inspect(void *ptr, fckc_size_t align);
fckc_size_t fck_set_suggested_align(fckc_size_t x);

void *fck_opaque_set_alloc(fck_set_info info);
void fck_opaque_set_free(void *ptr, fckc_size_t align);
fckc_size_t fck_opaque_set_weak_add(void **ptr, fckc_u64 hash, fckc_size_t align);
void fck_opaque_set_remove(void **ptr, fckc_u64 hash, fckc_size_t align);

#define fck_set_new(type, alloc, cap)                                                                                                      \
	(type *)fck_opaque_set_alloc(                                                                                                          \
		(fck_set_info){.allocator = (alloc), .el_align = sizeof(type), .el_size = sizeof(type), .capacity = (cap), .size = 0})

// Let's see if ptr to ptr makes sense
#define fck_set_destroy(ptr) fck_opaque_set_free((void *)(ptr), sizeof(*(ptr)))
#define fck_set_add(ptr, hash, value) ptr[fck_opaque_set_weak_add((void **)&(ptr), hash, fck_set_suggested_align(sizeof(*(ptr))))] = value
#define fck_set_remove(ptr, hash) fck_opaque_set_remove((void **)&(ptr), hash, fck_set_suggested_align(sizeof(*(ptr))))
#define fck_set_inspect(ptr) fck_opaque_set_inspect((void*)(ptr), fck_set_suggested_align(sizeof(*(ptr))))
#endif // FCK_SET_H_INCLUDED