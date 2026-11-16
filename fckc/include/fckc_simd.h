#ifndef FCK_SIMD_H_INCLUDED
#define FCK_SIMD_H_INCLUDED

// TODO: Decide on better names. The SSE names aint cutting it

#if !defined(FCK_SIMD_FORCE_FALLBACK)
#if defined(__SSE2__) || defined(_M_X64) || defined(_M_AMD64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#include "fckc_simd.sse.inl"
#elif defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(_M_ARM) || defined(_M_ARM64)
#include "fckc_simd.neon.inl"
#else
#include "fckc_simd.fallback.inl"
#endif
#else
#include "fckc_simd.fallback.inl"
#endif

// ...      [ 0,  0, 0xFF,  0,  0, 0xFF,  0,  0,  ...,  0 ]
// ... ->   [ 0,  0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  ..., 0xFF ]
static inline fckc_i128 simd_prefix_or(fckc_i128 v)
{
	v = fck_simd_i128_or(v, fck_simd_i128_bsll(v, 1));
	v = fck_simd_i128_or(v, fck_simd_i128_bsll(v, 2));
	v = fck_simd_i128_or(v, fck_simd_i128_bsll(v, 4));
	v = fck_simd_i128_or(v, fck_simd_i128_bsll(v, 8));
	return v;
}

// ...      [ 0,  0, 0xFF,  0,  0, 0xFF,  0,  0,  ...,  0 ]
// ... ->   [ 0,  0, 0xFF, 0xFF, 0xFF, 0, 0, 0,  ...,  0 ]
static inline fckc_i128 simd_prefix_xor(fckc_i128 v)
{
	v = fck_simd_i128_xor(v, fck_simd_i128_bsll(v, 1));
	v = fck_simd_i128_xor(v, fck_simd_i128_bsll(v, 2));
	v = fck_simd_i128_xor(v, fck_simd_i128_bsll(v, 4));
	v = fck_simd_i128_xor(v, fck_simd_i128_bsll(v, 8));
	return v;
}

// set:     [  0,    0,   0xFF, 0xFF, 0xFF,  0,   0xFF, 0xFF, ...,  0   ]
// reset:   [ 0xFF, 0xFF,  0,    0,    0,   0xFF,  0,    0,   ..., 0xFF ]
// ... ->   [  0,    0,   0xFF,  0,   0xFF,  0,   0xFF,  0,   ...,  0   ]
static inline fckc_i128 simd_prefix_xor_reset(fckc_i128 set, fckc_i128 reset)
{
	fckc_i128 result = set;
	fckc_i128 value  = fck_simd_i128_not(reset);

	result = fck_simd_i128_xor(result, fck_simd_i128_and(value, fck_simd_i128_bsll(result, 1)));
	value  = fck_simd_i128_and(value, fck_simd_i128_bsll(value, 1));
	result = fck_simd_i128_xor(result, fck_simd_i128_and(value, fck_simd_i128_bsll(result, 2)));
	value  = fck_simd_i128_and(value, fck_simd_i128_bsll(value, 2));
	result = fck_simd_i128_xor(result, fck_simd_i128_and(value, fck_simd_i128_bsll(result, 4)));
	value  = fck_simd_i128_and(value, fck_simd_i128_bsll(value, 4));
	result = fck_simd_i128_xor(result, fck_simd_i128_and(value, fck_simd_i128_bsll(result, 8)));

	return result;
}

// set:     [ 0,  0, 0xFF,  0,    0,    0,    0,  0,  ...,  0 ]
// reset:   [ 0,  0,  0,    0,    0,   0xFF,  0,  0,  ...,  0 ]
// ... ->   [ 0,  0, 0xFF, 0xFF, 0xFF,  0,    0,  0,  ...,  0 ]
static inline fckc_i128 simd_prefix_or_reset(fckc_i128 set, fckc_i128 reset)
{
	fckc_i128 result = set;
	fckc_i128 value  = fck_simd_i128_not(reset);

	result = fck_simd_i128_or(result, fck_simd_i128_and(value, fck_simd_i128_bsll(result, 1)));
	value  = fck_simd_i128_and(value, fck_simd_i128_bsll(value, 1));
	result = fck_simd_i128_or(result, fck_simd_i128_and(value, fck_simd_i128_bsll(result, 2)));
	value  = fck_simd_i128_and(value, fck_simd_i128_bsll(value, 2));
	result = fck_simd_i128_or(result, fck_simd_i128_and(value, fck_simd_i128_bsll(result, 4)));
	value  = fck_simd_i128_and(value, fck_simd_i128_bsll(value, 4));
	result = fck_simd_i128_or(result, fck_simd_i128_and(value, fck_simd_i128_bsll(result, 8)));

	return result;
}

#endif // !FCK_SIMD_H_INCLUDED
