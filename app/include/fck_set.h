#ifndef FCK_SET_H_INCLUDED
#define FCK_SET_H_INCLUDED

#include <fckc_inttypes.h>

#define fck_set_alignof(x) fck_set_suggested_align(sizeof(x))

struct fck_set_key;
struct fck_set_state;

// Not a fan of making this public, but whatever
typedef struct fck_set_info
{
	struct kll_allocator *allocator;
	fckc_size_t capacity;
	fckc_size_t size;
	fckc_size_t el_align;
	fckc_size_t el_size;
	fckc_size_t stale;

	struct fck_set_key *keys;
	struct fck_set_state *states;
} fck_set_info;

fckc_size_t fck_set_suggested_align(fckc_size_t x);

fck_set_info *fck_opaque_set_inspect(void *ptr, fckc_size_t align);

void *fck_opaque_set_alloc(fck_set_info info);
void fck_opaque_set_free(void *ptr, fck_set_info *info);
void fck_opaque_set_clear(void *ptr, fck_set_info *info);

fckc_size_t fck_opaque_set_strong_add(void **ptr, fckc_u64 hash, fck_set_info *info);
fckc_size_t fck_opaque_set_weak_add(void **ptr, fckc_u64 hash, fck_set_info *info);
fckc_size_t fck_opaque_set_probe(void *ptr, fckc_u64 hash, fck_set_info *info);

void fck_opaque_set_remove(void *ptr, fckc_u64 hash, fck_set_info *info);

fckc_size_t fck_opaque_set_begin(void const *ptr, fck_set_info const *info);
int fck_opaque_set_next(fck_set_info const *info, fckc_size_t *index);

// Let's see if ptr to ptr makes sense
#define fck_set_inspect(ptr) fck_opaque_set_inspect((void *)(ptr), (fck_set_alignof(*(ptr))))
#define fck_set_new(type, alloc, cap)                                                                                                      \
	(type *)fck_opaque_set_alloc(                                                                                                          \
		(fck_set_info){.allocator = (alloc), .el_align = fck_set_alignof(type), .el_size = sizeof(type), .capacity = (cap)})
#define fck_set_destroy(ptr) fck_opaque_set_free((void *)(ptr), fck_set_inspect((ptr)))

// TODO: A bit stronger type for index/iterator
#define fck_set_begin(ptr) fck_opaque_set_begin(ptr, fck_set_inspect((ptr)))
#define fck_set_next(ptr, iterator) fck_opaque_set_next(fck_set_inspect((ptr)), &(iterator))

// TODO: Maybe making this a strong_add might be more appropiate
#define fck_set_add(ptr, hash, value) ptr[fck_opaque_set_weak_add((void **)&(ptr), hash, (fck_set_inspect((ptr))))] = value
#define fck_set_remove(ptr, hash) fck_opaque_set_remove((void *)(ptr), hash, (fck_set_inspect((ptr))))

// fck_set_at(set, hash(key)) = value;
#define fck_set_at(ptr, hash) ptr[fck_opaque_set_weak_add((void **)&(ptr), hash, (fck_set_inspect((ptr))))]

//	fckc_size_t has = fck_set_probe(set, hash(k));
//	if(has) {
//		set[has - 1] = value;
//	}
#define fck_set_probe(ptr, hash) fck_opaque_set_probe((void *)(ptr), hash, (fck_set_inspect((ptr))))

#define fck_set_clear(ptr) fck_opaque_set_clear(ptr, (fck_set_inspect((ptr))))

#endif // FCK_SET_H_INCLUDED