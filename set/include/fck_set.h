#ifndef FCK_SET_H_INCLUDED
#define FCK_SET_H_INCLUDED

// #define FCK_SET_NO_KLL
// #define FCK_SET_NO_FCKC

#ifndef FCK_SET_NO_FCKC
#include <fckc_apidef.h>
#include <fckc_inttypes.h>

#if defined(FCK_SET_EXPORT)
#define FCK_SET_API FCK_EXPORT_API
#else
#define FCK_SET_API FCK_IMPORT_API
#endif
#else
// Standalone mode for the header and source file...
// This requires that the required stuff from fckc_inttypes and fckc_apidefs is getting
// inlined
#include <inttypes.h>
#include <stddef.h>
typedef size_t fckc_size_t;
typedef uint64_t fckc_u64;
// No EXPORT/IMPORT macros
#define FCK_SET_API
#endif

#ifndef FCK_SET_NO_KLL
// Just a small memoriser in the shape of a forward declaration what happens if NO_KLL is defined
// The coding standard disallows typedefs of forward declarations
struct kll_allocator;
#endif

// TODO: Can also do a deferred subscript instead...
struct fck_set_key;
struct fck_set_state;

// Not a fan of making this public, but whatever
// We can put this shit AT THE END of the block and pad the front,
// this way we can always step back from the pointer, I think!! :)
typedef struct fck_set_info
{
	fckc_size_t cookie;
#ifndef FCK_SET_NO_KLL
	struct kll_allocator *allocator;
#endif
	fckc_size_t capacity;
	fckc_size_t size;
	// Let's fuck this for now - max_align of this is 64 bytes
	// fckc_size_t el_align;
	fckc_size_t el_size;
	fckc_size_t stale;
	struct fck_set_key *keys;
	struct fck_set_state *states;
} fck_set_info;

FCK_SET_API fckc_size_t fck_set_suggested_align(fckc_size_t x);

FCK_SET_API fck_set_info *fck_opaque_set_inspect(void const *ptr);

FCK_SET_API void *fck_opaque_set_alloc(fck_set_info info);
FCK_SET_API void fck_opaque_set_free(void *ptr);
FCK_SET_API void fck_opaque_set_clear(void *ptr);

FCK_SET_API fckc_size_t fck_opaque_set_strong_add(void **ptr, fckc_u64 hash);
FCK_SET_API fckc_size_t fck_opaque_set_weak_add(void **ptr, fckc_u64 hash);
FCK_SET_API fckc_size_t fck_opaque_set_find(void *ptr, fckc_u64 hash);
FCK_SET_API void fck_opaque_set_remove(void *ptr, fckc_u64 hash);

FCK_SET_API fckc_size_t fck_opaque_set_begin(void const *ptr);
FCK_SET_API int fck_opaque_set_next(void const *ptr, fckc_size_t *index);

FCK_SET_API int fck_opaque_set_valid_at(void const *ptr, fckc_size_t at);
FCK_SET_API struct fck_set_key *fck_opaque_set_keys_at(void const *ptr, fckc_size_t at);

FCK_SET_API fckc_u64 fck_set_key_resolve(struct fck_set_key *key);

// Let's see if ptr to ptr makes sense
#define fck_set_inspect(ptr) fck_opaque_set_inspect((void *)(ptr))
#define fck_set_count(ptr) fck_set_inspect(ptr)->size

#ifndef FCK_SET_NO_KLL
#define fck_set_new(type, alloc, cap)                                                                                                      \
	(type *)fck_opaque_set_alloc((fck_set_info){.allocator = (alloc), .el_size = sizeof(type), .capacity = (cap)})
#else
#define fck_set_new(type, cap) (type *)fck_opaque_set_alloc((fck_set_info){.allocator = NULL, .el_size = sizeof(type), .capacity = (cap)})
#endif

#define fck_set_destroy(ptr) fck_opaque_set_free((void *)(ptr))
#define fck_set_clear(ptr) fck_opaque_set_clear(ptr)

// TODO: A bit stronger type for index/iterator
#define fck_set_begin(ptr) fck_opaque_set_begin(ptr)
#define fck_set_next(ptr, iterator) fck_opaque_set_next((ptr), &(iterator))

// fck_set_at(set, hash(key)) = value;
#define fck_set_at(ptr, hash) fck_opaque_set_weak_add((void **)&(ptr), hash)[(ptr)]

// TODO: Maybe making this a strong_add might be more appropiate
#define fck_set_add(ptr, hash, value) fck_set_at(ptr, hash) = value
#define fck_set_remove(ptr, hash) fck_opaque_set_remove((void *)(ptr), hash)

//	fckc_size_t has = fck_set_probe(set, hash(k));
//	if(has) {
//		set[has - 1] = value;
//	}
#define fck_set_find(ptr, hash) fck_opaque_set_find((void *)(ptr), hash)

#define fck_set_valid_at(ptr, at) fck_opaque_set_valid_at((ptr), (at))
#define fck_set_keys_at(ptr, at) fck_opaque_set_keys_at((ptr), (at))
#define fck_set_index_of(ptr, entry) (fckc_size_t)((entry) - (ptr))
#endif // FCK_SET_H_INCLUDED