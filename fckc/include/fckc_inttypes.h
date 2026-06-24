// fckc_inttypes.h

#ifndef FCKC_INTTYPES_H_INCLUDED
#define FCKC_INTTYPES_H_INCLUDED

#include <stddef.h>
#include <stdint.h>

#define fck_arraysize(array) (sizeof(array) / sizeof((array)[0]))

#define fck_alias(original, alias) alias

// Semantics types...
// I start doubting these
typedef float fckc_f32;
typedef double fckc_f64;

typedef char fckc_char;

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

// Prefer these - I do not doubt these
#define to_f32(x) ((fckc_f32)(x))
#define to_f64(x) ((fckc_f64)(x))
#define to_u8(x) ((fckc_u8)(x))
#define to_u16(x) ((fckc_u16)(x))
#define to_u32(x) ((fckc_u32)(x))
#define to_u64(x) ((fckc_u64)(x))
#define to_i8(x) ((fckc_i8)(x))
#define to_i16(x) ((fckc_i16)(x))
#define to_i32(x) ((fckc_i32)(x))
#define to_i64(x) ((fckc_i64)(x))

#define to_size_t(x) ((fckc_size_t)(x))
#define to_int(x) ((int)(x))

#define fck_scope_str_concat(lhs, rhs) lhs##rhs
#define fck_scope_unique(lhs, rhs) fck_scope_str_concat(lhs, rhs)
#define fck_scope(ctor, dtor)                                                                                                              \
	for (int fck_scope_unique(i, __LINE__) = ((ctor) ? 0 : 1); fck_scope_unique(i, __LINE__) == 0;                                         \
	     fck_scope_unique(i, __LINE__) += 1, (dtor))

#ifndef offsetof
#define offsetof(st, m) ((fckc_uintptr) & (((st *)0)->m))
#endif

#ifndef alignof
// The issue with this shit here is that alignof behaves slightly different
// The fallback using offsetof actually does not allow passing in non-type input
// while the compiler extensions do...
#if defined(__GNUC__) || defined(__clang__)
#define alignof(type) __alignof__(type)
#elif defined(_MSC_VER)
#define alignof(type) __alignof(type)
#else
#define alignof(type) offsetof(struct { char c; type t; }, t)
#endif
#endif

#define ALIGNOF_C99(type) offsetof(struct { char c; type t; }, t)

#define fckc_align(offset, align) (((offset) + (align) - 1) & ~((align) - 1))

#define fckc_concat_implementation(x, y) x##y
#define fckc_concat(x, y) fckc_concat_implementation(x, y)
#define fckc_pad(n) char fckc_concat(_padding_, __LINE__)[n]

#define fck_kilobytes(x) ((fckc_size_t)(x) * 1024UL)
#define fck_megabytes(x) ((fckc_size_t)(x) * 1024UL * 1024UL)
#define fck_gigabytes(x) ((fckc_size_t)(x) * 1024UL * 1024UL * 1024UL)

// Rename to fck_test
#define sht_test(mask, flag) (((mask) & (flag)) == (flag))

#endif // !FCKC_INTTYPES_H_INCLUDED