#ifndef FCK_SIMD_NEON_H_INCLUDED
#define FCK_SIMD_NEON_H_INCLUDED

#include <arm_neon.h>

typedef int8x16_t fckc_i128;

#define fck_simd_i128_zero() vdupq_n_s8(0)
#define fck_simd_i128_ones() vdupq_n_s8(-1)

#define fck_simd_i128_set1_i8(val)  vdupq_n_s8((int8_t)(val))
#define fck_simd_i128_set1_i16(val) vreinterpretq_s8_s16(vdupq_n_s16((int16_t)(val)))
#define fck_simd_i128_set1_i32(val) vreinterpretq_s8_s32(vdupq_n_s32((int32_t)(val)))

#define fck_simd_i128_load(ptr)        vld1q_s8((const int8_t *)(ptr))
#define fck_simd_i128_loadu(ptr)       vld1q_s8((const int8_t *)(ptr))
#define fck_simd_i128_store(ptr, vec)  vst1q_s8((int8_t *)(ptr), (vec))
#define fck_simd_i128_storeu(ptr, vec) vst1q_s8((int8_t *)(ptr), (vec))

#define fck_simd_i128_not(a)             vmvnq_s8((a))
#define fck_simd_i128_and(a, b)          vandq_s8((a), (b))
#define fck_simd_i128_or(a, b)           vorrq_s8((a), (b))
#define fck_simd_i128_xor(a, b)          veorq_s8((a), (b))
#define fck_simd_i128_andnot(a, b)       vbicq_s8((a), (b))

#define fck_simd_i128_ornot(a, b)        vornq_s8((a), (b))
#define fck_simd_i128_nand(a, b)         vmvnq_s8(vandq_s8((a), (b)))
#define fck_simd_i128_nor(a, b)          vmvnq_s8(vorrq_s8((a), (b)))
#define fck_simd_i128_xnor(a, b)         vmvnq_s8(veorq_s8((a), (b)))
#define fck_simd_i128_select(mask, a, b) vbslq_s8(vreinterpretq_u8_s8(mask), (a), (b))

// Byte shifts using vextq
#define fck_simd_i128_bsll(a, imm) vextq_s8(vdupq_n_s8(0), (a), (16 - (imm)) & 15)
#define fck_simd_i128_bsrl(a, imm) vextq_s8((a), vdupq_n_s8(0), (imm) & 15)

#define fck_simd_i128_add_i8(a, b)  vaddq_s8((a), (b))
#define fck_simd_i128_add_i16(a, b) vreinterpretq_s8_s16(vaddq_s16(vreinterpretq_s16_s8(a), vreinterpretq_s16_s8(b)))
#define fck_simd_i128_add_i32(a, b) vreinterpretq_s8_s32(vaddq_s32(vreinterpretq_s32_s8(a), vreinterpretq_s32_s8(b)))
#define fck_simd_i128_sub_i8(a, b)  vsubq_s8((a), (b))
#define fck_simd_i128_sub_i16(a, b) vreinterpretq_s8_s16(vsubq_s16(vreinterpretq_s16_s8(a), vreinterpretq_s16_s8(b)))
#define fck_simd_i128_sub_i32(a, b) vreinterpretq_s8_s32(vsubq_s32(vreinterpretq_s32_s8(a), vreinterpretq_s32_s8(b)))

#define fck_simd_i128_cmpeq_i8(a, b)  vreinterpretq_s8_u8(vceqq_s8((a), (b)))
#define fck_simd_i128_cmplt_i8(a, b)  vreinterpretq_s8_u8(vcltq_s8((a), (b)))
#define fck_simd_i128_cmpgt_i8(a, b)  vreinterpretq_s8_u8(vcgtq_s8((a), (b)))
#define fck_simd_i128_cmpeq_i16(a, b) vreinterpretq_s8_u16(vceqq_s16(vreinterpretq_s16_s8(a), vreinterpretq_s16_s8(b)))
#define fck_simd_i128_cmpeq_i32(a, b) vreinterpretq_s8_u32(vceqq_s32(vreinterpretq_s32_s8(a), vreinterpretq_s32_s8(b)))

static inline int fck_simd_i128_movemask_i8(fckc_i128 vec)
{
	uint8x16_t input = vreinterpretq_u8_s8(vec);
	static const uint8_t mask_table[16] = { 1, 2, 4, 8, 16, 32, 64, 128, 1, 2, 4, 8, 16, 32, 64, 128 };
	uint8x16_t bit_mask = vld1q_u8(mask_table);
	uint8x16_t masked = vandq_u8(vshrq_n_u8(input, 7), bit_mask);
	uint8x8_t res = vpadd_u8(vget_low_u8(masked), vget_high_u8(masked));
	res = vpadd_u8(res, res);
	res = vpadd_u8(res, res);
	return vget_lane_u8(res, 0) | (vget_lane_u8(res, 1) << 8);
}

#endif /* !FCK_SIMD_NEON_H_INCLUDED */