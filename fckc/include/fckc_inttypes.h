// fckc_inttypes.h

#ifndef FCKC_INTTYPES_H_INCLUDED
#define FCKC_INTTYPES_H_INCLUDED

#include <inttypes.h>
#include <stddef.h>

typedef int8_t fckc_i8;
typedef uint8_t fckc_u8;

typedef int16_t fckc_i16;
typedef uint16_t fckc_u16;

typedef int32_t fckc_i32;
typedef uint32_t fckc_u32;

typedef int64_t fckc_i64;
typedef uint64_t fckc_u64;

typedef size_t fckc_size_t;
typedef uintptr_t fckc_uintptr;

#define FCK_VECTOR_TYPE_DECLARATION(name, type)                                                                                            \
	typedef union fckc_##name##x2 {                                                                                                        \
		struct                                                                                                                             \
		{                                                                                                                                  \
			type x;                                                                                                                        \
			type y;                                                                                                                        \
		};                                                                                                                                 \
		type v[2];                                                                                                                         \
	} fckc_##name##x2;                                                                                                                     \
	typedef union fckc_##name##x3 {                                                                                                        \
		struct                                                                                                                             \
		{                                                                                                                                  \
			type x;                                                                                                                        \
			type y;                                                                                                                        \
			type z;                                                                                                                        \
		};                                                                                                                                 \
		type v[3];                                                                                                                         \
	} fckc_##name##x3;                                                                                                                     \
	typedef union fckc_##name##x4 {                                                                                                        \
		struct                                                                                                                             \
		{                                                                                                                                  \
			type x;                                                                                                                        \
			type y;                                                                                                                        \
			type z;                                                                                                                        \
			type w;                                                                                                                        \
		};                                                                                                                                 \
		type v[4];                                                                                                                         \
	} fckc_##name##x4

// Maybe just expand them...
FCK_VECTOR_TYPE_DECLARATION(f32, float);
FCK_VECTOR_TYPE_DECLARATION(f64, float);
FCK_VECTOR_TYPE_DECLARATION(i8, fckc_i8);
FCK_VECTOR_TYPE_DECLARATION(i16, fckc_i16);
FCK_VECTOR_TYPE_DECLARATION(i32, fckc_i32);
FCK_VECTOR_TYPE_DECLARATION(i64, fckc_i64);
FCK_VECTOR_TYPE_DECLARATION(u8, fckc_u8);
FCK_VECTOR_TYPE_DECLARATION(u16, fckc_u16);
FCK_VECTOR_TYPE_DECLARATION(u32, fckc_u32);
FCK_VECTOR_TYPE_DECLARATION(u64, fckc_u64);

typedef struct fck_lstring
{
	char *data; // Should this actually be char*? Urgh
	fckc_size_t capacity;
	fckc_size_t size;
} fck_lstring;

#ifndef offsetof
#define offsetof(st, m) ((fckc_uintptr) & (((st *)0)->m))
#endif

#endif // !FCKC_INTTYPES_H_INCLUDED