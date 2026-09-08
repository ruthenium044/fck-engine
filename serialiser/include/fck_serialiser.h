#ifndef FCK_SERIALISER_H_INCLUDED
#define FCK_SERIALISER_H_INCLUDED

#include <fckc_inttypes.h>

struct kll_allocator;

// TODO: Make these flags... Since we can only use them for querying...
typedef enum fck_serialiser_primitive
{
	fck_serialiser_push,
	fck_serialiser_pop,
	fck_serialiser_i8,
	fck_serialiser_i16,
	fck_serialiser_i32,
	fck_serialiser_i64,
	fck_serialiser_u8,
	fck_serialiser_u16,
	fck_serialiser_u32,
	fck_serialiser_u64,
	fck_serialiser_f32,
	fck_serialiser_f64,
	fck_serialiser_string,
} fck_serialiser_primitive;

typedef union fck_serialiser_value {
	fckc_i8 as_i8;
	fckc_i16 as_i16;
	fckc_i32 as_i32;
	fckc_i64 as_i64;
	fckc_u8 as_u8;
	fckc_u16 as_u16;
	fckc_u32 as_u32;
	fckc_u64 as_u64;
	fckc_f32 as_f32;
	fckc_f64 as_f64;
	fckc_char *as_string;
} fck_serialiser_value;

typedef struct fck_serialiser_element
{
	const char *name;
	fck_serialiser_value *values;
	fckc_size_t count;
	fck_serialiser_primitive type;
} fck_serialiser_element;

struct fck_serialiser_iterator;
typedef struct fck_serialiser_iterator
{
	fck_serialiser_element *(*next)(struct fck_serialiser_iterator *iterator, fck_serialiser_element *element);
	void (*destroy)(struct fck_serialiser_iterator *iterator);
} fck_serialiser_iterator;

struct fck_serialiser;
typedef struct fck_serialiser
{
	// If allocators and such are needed, the fck_serialiser implementation can provide queries and iterators with them
	// Optional - Can be NULL
	fck_serialiser_iterator *(*iterator)(struct fck_serialiser *s);

	// Optional - Can be NULL - By convention only valid inbetween the query calls!
	// Returned pointer can get freed via kll_free or reset by arena...
	fck_serialiser_element *(*query)(struct fck_serialiser *s, const char *path);

	void (*destroy)(struct fck_serialiser *s);

	//TODO: add array thaaaanks
	void (*push)(struct fck_serialiser *s, const char *name);
	void (*pop)(struct fck_serialiser *s);

	void (*i8)(struct fck_serialiser *s, const char *name, fckc_i8 *v, fckc_size_t c);
	void (*i16)(struct fck_serialiser *s, const char *name, fckc_i16 *v, fckc_size_t c);
	void (*i32)(struct fck_serialiser *s, const char *name, fckc_i32 *v, fckc_size_t c);
	void (*i64)(struct fck_serialiser *s, const char *name, fckc_i64 *v, fckc_size_t c);
	void (*u8)(struct fck_serialiser *s, const char *name, fckc_u8 *v, fckc_size_t c);
	void (*u16)(struct fck_serialiser *s, const char *name, fckc_u16 *v, fckc_size_t c);
	void (*u32)(struct fck_serialiser *s, const char *name, fckc_u32 *v, fckc_size_t c);
	void (*u64)(struct fck_serialiser *s, const char *name, fckc_u64 *v, fckc_size_t c);
	void (*f32)(struct fck_serialiser *s, const char *name, fckc_f32 *v, fckc_size_t c);
	void (*f64)(struct fck_serialiser *s, const char *name, fckc_f64 *v, fckc_size_t c);

	void (*string)(struct fck_serialiser *s, const char *name, void **v, fckc_size_t c);

	void *(*buffer)(struct fck_serialiser *s);
	fckc_size_t (*at)(struct fck_serialiser *s);
} fck_serialiser;

#endif // FCK_SERIALISER_H_INCLUDED