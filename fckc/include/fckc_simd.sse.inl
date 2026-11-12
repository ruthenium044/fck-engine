#ifndef FCK_SIMD_SSE_H_INCLUDED
#define FCK_SIMD_SSE_H_INCLUDED

#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <emmintrin.h>
#endif

typedef __m128i fckc_i128;

#define fck_simd_i128_zero() _mm_setzero_si128()
#define fck_simd_i128_ones() _mm_set1_epi32(-1)

#define fck_simd_i128_set_i8(e15, e14, e13, e12, e11, e10, e9, e8, e7, e6, e5, e4, e3, e2, e1, e0)                                         \
	_mm_set_epi8((e15), (e14), (e13), (e12), (e11), (e10), (e9), (e8), (e7), (e6), (e5), (e4), (e3), (e2), (e1), (e0))

#define fck_simd_i128_set1_i8(val)  _mm_set1_epi8((char)(val))
#define fck_simd_i128_set1_i16(val) _mm_set1_epi16((short)(val))
#define fck_simd_i128_set1_i32(val) _mm_set1_epi32((int)(val))

#define fck_simd_i128_load(ptr)        _mm_load_si128((const __m128i *)(const void *)(ptr))
#define fck_simd_i128_loadu(ptr)       _mm_loadu_si128((const __m128i *)(const void *)(ptr))
#define fck_simd_i128_store(ptr, vec)  _mm_store_si128((__m128i *)(void *)(ptr), (vec))
#define fck_simd_i128_storeu(ptr, vec) _mm_storeu_si128((__m128i *)(void *)(ptr), (vec))

#define fck_simd_i128_not(a)       _mm_xor_si128((a), _mm_set1_epi32(-1))
#define fck_simd_i128_and(a, b)    _mm_and_si128((a), (b))
#define fck_simd_i128_or(a, b)     _mm_or_si128((a), (b))
#define fck_simd_i128_xor(a, b)    _mm_xor_si128((a), (b))
#define fck_simd_i128_andnot(a, b) _mm_andnot_si128((b), (a))

#define fck_simd_i128_ornot(a, b)        _mm_or_si128((a), _mm_xor_si128((b), _mm_set1_epi32(-1)))
#define fck_simd_i128_nand(a, b)         _mm_xor_si128(_mm_and_si128((a), (b)), _mm_set1_epi32(-1))
#define fck_simd_i128_nor(a, b)          _mm_xor_si128(_mm_or_si128((a), (b)), _mm_set1_epi32(-1))
#define fck_simd_i128_xnor(a, b)         _mm_xor_si128(_mm_xor_si128((a), (b)), _mm_set1_epi32(-1))
#define fck_simd_i128_select(mask, a, b) _mm_or_si128(_mm_and_si128((mask), (a)), _mm_andnot_si128((mask), (b)))

// _mm_slli_si128 is supported on all SSE2 compilers...
#define fck_simd_i128_bsll(a, imm) _mm_slli_si128((a), (imm))
#define fck_simd_i128_bsrl(a, imm) _mm_srli_si128((a), (imm))

#define fck_simd_i128_add_i8(a, b)  _mm_add_epi8((a), (b))
#define fck_simd_i128_add_i16(a, b) _mm_add_epi16((a), (b))
#define fck_simd_i128_add_i32(a, b) _mm_add_epi32((a), (b))
#define fck_simd_i128_sub_i8(a, b)  _mm_sub_epi8((a), (b))
#define fck_simd_i128_sub_i16(a, b) _mm_sub_epi16((a), (b))
#define fck_simd_i128_sub_i32(a, b) _mm_sub_epi32((a), (b))

#define fck_simd_i128_cmpeq_i8(a, b)  _mm_cmpeq_epi8((a), (b))
#define fck_simd_i128_cmplt_i8(a, b)  _mm_cmplt_epi8((a), (b))
#define fck_simd_i128_cmpgt_i8(a, b)  _mm_cmpgt_epi8((a), (b))
#define fck_simd_i128_cmpeq_i16(a, b) _mm_cmpeq_epi16((a), (b))
#define fck_simd_i128_cmpeq_i32(a, b) _mm_cmpeq_epi32((a), (b))

#define fck_simd_i128_movemask_i8(vec) _mm_movemask_epi8((vec))

#endif /* !FCK_SIMD_SSE_H_INCLUDED */