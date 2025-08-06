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
typedef uintptr_t fckc_uptr;

typedef struct fck_lstring
{
	char *data; // Should this actually be char*? Urgh
	fckc_size_t capacity;
	fckc_size_t size;
} fck_lstring;

#endif // !FCKC_INTTYPES_H_INCLUDED