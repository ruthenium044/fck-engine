// fckc_inttypes.h

#ifndef FCKC_INTTYPES_H_INCLUDED
#define FCKC_INTTYPES_H_INCLUDED

#include <inttypes.h>
#include <stddef.h>

// Semantics types...
typedef float fckc_f32;
typedef double fckc_f64;

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

// TODO: Remove all this junk below... it is part of math...

#define FCK_NAMED_VECTOR_TYPE(name, type)                                                                                                  \
	typedef union fckc_##name##x1 {                                                                                                        \
		struct                                                                                                                             \
		{                                                                                                                                  \
			type x;                                                                                                                        \
		};                                                                                                                                 \
		type v[1];                                                                                                                         \
	} fckc_##name##x1;                                                                                                                     \
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
FCK_NAMED_VECTOR_TYPE(f32, fckc_f32);
FCK_NAMED_VECTOR_TYPE(f64, fckc_f64);
FCK_NAMED_VECTOR_TYPE(i8, fckc_i8);
FCK_NAMED_VECTOR_TYPE(i16, fckc_i16);
FCK_NAMED_VECTOR_TYPE(i32, fckc_i32);
FCK_NAMED_VECTOR_TYPE(i64, fckc_i64);
FCK_NAMED_VECTOR_TYPE(u8, fckc_u8);
FCK_NAMED_VECTOR_TYPE(u16, fckc_u16);
FCK_NAMED_VECTOR_TYPE(u32, fckc_u32);
FCK_NAMED_VECTOR_TYPE(u64, fckc_u64);

#define fck_scope_str_concat(lhs, rhs) lhs##rhs
#define fck_scope_unique(lhs, rhs) fck_scope_str_concat(lhs, rhs)
#define fck_scope(ctor, dtor)                                                                                                              \
	for (int fck_scope_unique(i, __LINE__) = ((ctor) ? 0 : 1); fck_scope_unique(i, __LINE__) == 0;                                         \
	     fck_scope_unique(i, __LINE__) += 1, (dtor))

#ifndef offsetof
#define offsetof(st, m) ((fckc_uintptr) & (((st *)0)->m))
#endif

#ifndef alignof
#define alignof(type) ((size_t)((char*)&((struct { char c; type t; }*)0)->t))
#endif

#endif // !FCKC_INTTYPES_H_INCLUDED