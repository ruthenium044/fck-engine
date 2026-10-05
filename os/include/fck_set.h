#ifndef FCK_SET_H_INCLUDED
#define FCK_SET_H_INCLUDED

#ifndef FCK_KLL_H_INCLUDED
#error kll.h not included before set
#endif // !FCK_KLL_H_INCLUDED

#ifndef FCK_KLL_MALLOC_H_INCLUDED
#error kll_malloc.h not included before set
#endif // !FCK_KLL_MALLOC_H_INCLUDED

#include <fckc_inttypes.h>

struct kll_allocator;

typedef struct fck_set_info
{
	fckc_size_t           cookie;
	struct kll_allocator *allocator;
	fckc_size_t           capacity;
	fckc_size_t           size;
	fckc_size_t           el_size;
	fckc_size_t           stale;
	struct fck_set_key   *keys;
	struct fck_set_state *states;
} fck_set_info;

inline fck_set_info *fck_set_untyped_inspect(void const *ptr);

inline void *fck_set_untyped_alloc(fck_set_info info);
inline void  fck_set_untyped_free(void *ptr);
inline void  fck_set_untyped_clear(void *ptr);

inline fckc_size_t fck_set_untyped_strong_add(void **ptr, fckc_u64 hash);
inline fckc_size_t fck_set_untyped_weak_add(void **ptr, fckc_u64 hash);
inline fckc_size_t fck_set_untyped_find(void *ptr, fckc_u64 hash);
inline void        fck_set_untyped_remove(void *ptr, fckc_u64 hash);

inline fckc_size_t fck_set_untyped_begin(void const *ptr);
inline int         fck_set_untyped_next(void const *ptr, fckc_size_t *index);

inline int                 fck_set_untyped_valid_at(void const *ptr, fckc_size_t at);
inline struct fck_set_key *fck_set_untyped_keys_at(void const *ptr, fckc_size_t at);

inline fckc_u64 fck_set_key_resolve(struct fck_set_key *key);

// Let's see if ptr to ptr makes sense
#define fck_set_inspect(ptr) fck_set_untyped_inspect((void *)(ptr))
#define fck_set_count(ptr)   fck_set_inspect(ptr)->size

#define fck_set_new(type, alloc, cap) (type *)fck_set_untyped_alloc((fck_set_info){.allocator = (alloc), .el_size = sizeof(type), .capacity = (cap)})

#define fck_set_destroy(ptr) fck_set_untyped_free((void *)(ptr))
#define fck_set_clear(ptr)   fck_set_untyped_clear(ptr)

// TODO: A bit stronger type for index/iterator
#define fck_set_begin(ptr)          fck_set_untyped_begin(ptr)
#define fck_set_next(ptr, iterator) fck_set_untyped_next((ptr), &(iterator))

// fck_set_at(set, hash(key)) = value;
#define fck_set_at(ptr, hash) fck_set_untyped_strong_add((void **)&(ptr), hash)[(ptr)]

// TODO: Maybe making this a strong_add might be more appropiate
#define fck_set_add(ptr, hash, value) fck_set_at(ptr, hash) = value
#define fck_set_remove(ptr, hash)     fck_set_untyped_remove((void *)(ptr), hash)

//	fckc_size_t has = fck_set_probe(set, hash(k));
//	if(has) {
//		set[has - 1] = value;
//	}
#define fck_set_find(ptr, hash) fck_set_untyped_find((void *)(ptr), hash)

#define fck_set_valid_at(ptr, at)    fck_set_untyped_valid_at((ptr), (at))
#define fck_set_keys_at(ptr, at)     fck_set_untyped_keys_at((ptr), (at))
#define fck_set_index_of(ptr, entry) (fckc_size_t)((entry) - (ptr))

#include "fck_set.inl"

#endif //! FCK_SET_H_INCLUDED