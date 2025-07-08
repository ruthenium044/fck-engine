// fckc_inttypes.h

#ifndef FCKC_INTTYPES_H_INCLUDED
#define FCKC_INTTYPES_H_INCLUDED

#include <stddef.h>

typedef signed char fckc_i8;
typedef unsigned char fckc_u8;

typedef short fckc_i16;
typedef unsigned short fckc_u16;

typedef int fckc_i32;
typedef unsigned int fckc_u32;

// We do not support anything else but clang and msvc for now...
#if defined(_MSC_VER)
typedef __int64 fckc_i64;
typedef unsigned __int64 fckc_u64;
#elif defined(__clang__)
typedef long long fckc_i64;
typedef unsigned long long fckc_u64;
#else
#warning "No 64-bit integer type defined for this compiler - We try anyway!"
typedef long long fckc_i64;
typedef unsigned long long fckc_u64;
#endif

typedef size_t fckc_size_t;

#endif // !FCKC_INTTYPES_H_INCLUDED