#ifndef FCK_BITS_H_INCLUDED
#define FCK_BITS_H_INCLUDED

#include "fckc_inttypes.h"

// Maybe it will catch on
// lsz -> least significant zeroes
// msz -> most significant zeroes
// lso -> least significant ones
// mso -> most significant ones

inline int fck_lszcnt_u32(fckc_u32);
inline int fck_mszcnt_u32(fckc_u32);
inline int fck_lsocnt_u32(fckc_u32);
inline int fck_msocnt_u32(fckc_u32);
inline int fck_popcnt_u32(fckc_u32);

inline int fck_lszcnt_u64(fckc_u64);
inline int fck_mszcnt_u64(fckc_u64);
inline int fck_lsocnt_u64(fckc_u64);
inline int fck_msocnt_u64(fckc_u64);
inline int fck_popcnt_u64(fckc_u64);

#if defined(_MSC_VER) && !defined(__clang__)
#include <intrin.h>
#pragma intrinsic(_BitScanForward, _BitScanReverse, __popcnt)
#if defined(_WIN64)
#pragma intrinsic(_BitScanForward64, _BitScanReverse64, __popcnt64)
#endif
#endif

inline int fck_lszcnt_u32(fckc_u32 x)
{
	if (x == 0)
	{
		return 32;
	}
#if defined(__clang__) || defined(__GNUC__)
	return __builtin_ctz(x);
#elif defined(_MSC_VER)
	unsigned long index;
	_BitScanForward(&index, x);
	return (int)index;
#else
	int n = 0;
	if (!(x & 0x0000FFFF))
	{
		n += 16;
		x >>= 16;
	}
	if (!(x & 0x000000FF))
	{
		n += 8;
		x >>= 8;
	}
	if (!(x & 0x0000000F))
	{
		n += 4;
		x >>= 4;
	}
	if (!(x & 0x00000003))
	{
		n += 2;
		x >>= 2;
	}
	if (!(x & 0x00000001))
	{
		n += 1;
	}
	return n;
#endif
}

inline int fck_mszcnt_u32(fckc_u32 x)
{
	if (x == 0)
	{
		return 32;
	}
#if defined(__clang__) || defined(__GNUC__)
	return __builtin_clz(x);
#elif defined(_MSC_VER)
	unsigned long index;
	_BitScanReverse(&index, x);
	return (int)(31 - index);
#else
	int n = 0;
	if (!(x & 0xFFFF0000))
	{
		n += 16;
		x <<= 16;
	}
	if (!(x & 0xFF000000))
	{
		n += 8;
		x <<= 8;
	}
	if (!(x & 0xF0000000))
	{
		n += 4;
		x <<= 4;
	}
	if (!(x & 0xC0000000))
	{
		n += 2;
		x <<= 2;
	}
	if (!(x & 0x80000000))
	{
		n += 1;
	}
	return n;
#endif
}

inline int fck_lsocnt_u32(fckc_u32 x)
{
	return fck_lszcnt_u32(~x);
}

inline int fck_msocnt_u32(fckc_u32 x)
{
	return fck_mszcnt_u32(~x);
}

inline int fck_popcnt_u32(fckc_u32 x)
{
#if defined(__clang__) || defined(__GNUC__)
	return __builtin_popcount(x);
#elif defined(_MSC_VER)
	return (int)__popcnt(x);
#else
	x = x - ((x >> 1) & 0x55555555);
	x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
	x = (x + (x >> 4)) & 0x0F0F0F0F;
	x = x + (x >> 8);
	x = x + (x >> 16);
	return (int)(x & 0x0000003F);
#endif
}

inline int fck_lszcnt_u64(fckc_u64 x)
{
	if (x == 0)
	{
		return 64;
	}
#if defined(__clang__) || defined(__GNUC__)
	return __builtin_ctzll(x);
#elif defined(_MSC_VER)
	unsigned long index;
#if defined(_WIN64)
	_BitScanForward64(&index, x);
	return (int)index;
#else
	if ((fckc_u32)x != 0)
	{
		_BitScanForward(&index, (fckc_u32)x);
		return (int)index;
	}
	else
	{
		_BitScanForward(&index, (fckc_u32)(x >> 32));
		return (int)(index + 32);
	}
#endif
#else
	int n = 0;
	if (!(x & 0x00000000FFFFFFFFULL))
	{
		n += 32;
		x >>= 32;
	}
	if (!(x & 0x000000000000FFFFULL))
	{
		n += 16;
		x >>= 16;
	}
	if (!(x & 0x00000000000000FFULL))
	{
		n += 8;
		x >>= 8;
	}
	if (!(x & 0x000000000000000FULL))
	{
		n += 4;
		x >>= 4;
	}
	if (!(x & 0x0000000000000003ULL))
	{
		n += 2;
		x >>= 2;
	}
	if (!(x & 0x0000000000000001ULL))
	{
		n += 1;
	}
	return n;
#endif
}

inline int fck_mszcnt_u64(fckc_u64 x)
{
	if (x == 0)
	{
		return 64;
	}
#if defined(__clang__) || defined(__GNUC__)
	return __builtin_clzll(x);
#elif defined(_MSC_VER)
	unsigned long index;
#if defined(_WIN64)
	_BitScanReverse64(&index, x);
	return (int)(63 - index);
#else
	if ((x >> 32) != 0)
	{
		_BitScanReverse(&index, (fckc_u32)(x >> 32));
		return (int)(31 - index);
	}
	else
	{
		_BitScanReverse(&index, (fckc_u32)x);
		return (int)(63 - index);
	}
#endif
#else
	int n = 0;
	if (!(x & 0xFFFFFFFF00000000ULL))
	{
		n += 32;
		x <<= 32;
	}
	if (!(x & 0xFFFF000000000000ULL))
	{
		n += 16;
		x <<= 16;
	}
	if (!(x & 0xFF00000000000000ULL))
	{
		n += 8;
		x <<= 8;
	}
	if (!(x & 0xF000000000000000ULL))
	{
		n += 4;
		x <<= 4;
	}
	if (!(x & 0xC000000000000000ULL))
	{
		n += 2;
		x <<= 2;
	}
	if (!(x & 0x8000000000000000ULL))
	{
		n += 1;
	}
	return n;
#endif
}

inline int fck_lsocnt_u64(fckc_u64 x)
{
	return fck_lszcnt_u64(~x);
}

inline int fck_msocnt_u64(fckc_u64 x)
{
	return fck_mszcnt_u64(~x);
}

inline int fck_popcnt_u64(fckc_u64 x)
{
#if defined(__clang__) || defined(__GNUC__)
	return __builtin_popcountll(x);
#elif defined(_MSC_VER)
#if defined(_WIN64)
	return (int)__popcnt64(x);
#else
	return (int)(__popcnt((fckc_u32)x) + __popcnt((fckc_u32)(x >> 32)));
#endif
#else
	x = x - ((x >> 1) & 0x5555555555555555ULL);
	x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
	x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
	x = x + (x >> 8);
	x = x + (x >> 16);
	x = x + (x >> 32);
	return (int)(x & 0x000000000000007FULL);
#endif
}

#endif // !FCK_BITS_H_INCLUDED