#ifndef FCK_SIMD_FALLBACK_H_INCLUDED
#define FCK_SIMD_FALLBACK_H_INCLUDED

#include "fckc_inttypes.h"
#include <string.h>

typedef union fckc_i128 {
	fckc_i8  i8[16];
	fckc_u8  u8[16];
	fckc_i16 i16[8];
	fckc_u16 u16[8];
	fckc_i32 i32[4];
	fckc_u32 u32[4];
	fckc_i64 i64[2];
	fckc_u64 u64[2];
} fckc_i128;

static inline fckc_i128 fck_simd_i128_zero(void)
{
	fckc_i128 res;
	memset(&res, 0, sizeof(res));
	return res;
}

static inline fckc_i128 fck_simd_i128_ones(void)
{
	fckc_i128 res;
	memset(&res, 0xFF, sizeof(res));
	return res;
}

static inline fckc_i128 fck_simd_i128_set1_i8(fckc_i8 val)
{
	fckc_i128 res;
	int       i;
	for (i = 0; i < 16; ++i)
	{
		res.i8[i] = val;
	}
	return res;
}

static inline fckc_i128 fck_simd_i128_set1_i16(fckc_i16 val)
{
	fckc_i128 res;
	int       i;
	for (i = 0; i < 8; ++i)
	{
		res.i16[i] = val;
	}
	return res;
}

static inline fckc_i128 fck_simd_i128_set1_i32(fckc_i32 val)
{
	fckc_i128 res;
	int       i;
	for (i = 0; i < 4; ++i)
	{
		res.i32[i] = val;
	}
	return res;
}

static inline fckc_i128 fck_simd_i128_load(const void* ptr)
{
	fckc_i128 res;
	memcpy(&res, ptr, 16);
	return res;
}

static inline fckc_i128 fck_simd_i128_loadu(const void* ptr)
{
	fckc_i128 res;
	memcpy(&res, ptr, 16);
	return res;
}

static inline void fck_simd_i128_store(void* ptr, fckc_i128 vec)
{
	memcpy(ptr, &vec, 16);
}

static inline void fck_simd_i128_storeu(void* ptr, fckc_i128 vec)
{
	memcpy(ptr, &vec, 16);
}

static inline fckc_i128 fck_simd_i128_not(fckc_i128 a)
{
	fckc_i128 res;
	int       i;
	for (i = 0; i < 4; ++i)
	{
		res.u32[i] = ~a.u32[i];
	}
	return res;
}

static inline fckc_i128 fck_simd_i128_and(fckc_i128 a, fckc_i128 b)
{
	fckc_i128 res;
	int       i;
	for (i = 0; i < 4; ++i)
	{
		res.u32[i] = a.u32[i] & b.u32[i];
	}
	return res;
}

static inline fckc_i128 fck_simd_i128_or(fckc_i128 a, fckc_i128 b)
{
	fckc_i128 res;
	int       i;
	for (i = 0; i < 4; ++i)
	{
		res.u32[i] = a.u32[i] | b.u32[i];
	}
	return res;
}

static inline fckc_i128 fck_simd_i128_xor(fckc_i128 a, fckc_i128 b)
{
	fckc_i128 res;
	int       i;
	for (i = 0; i < 4; ++i)
	{
		res.u32[i] = a.u32[i] ^ b.u32[i];
	}
	return res;
}

// Computes: a & ~b
static inline fckc_i128 fck_simd_i128_andnot(fckc_i128 a, fckc_i128 b)
{
	fckc_i128 res;
	int       i;
	for (i = 0; i < 4; ++i)
	{
		res.u32[i] = a.u32[i] & (~b.u32[i]);
	}
	return res;
}

// Computes: a | ~b
static inline fckc_i128 fck_simd_i128_ornot(fckc_i128 a, fckc_i128 b)
{
	fckc_i128 res;
	int       i;
	for (i = 0; i < 4; ++i)
	{
		res.u32[i] = a.u32[i] | (~b.u32[i]);
	}
	return res;
}

static inline fckc_i128 fck_simd_i128_nand(fckc_i128 a, fckc_i128 b)
{
	fckc_i128 res;
	int       i;
	for (i = 0; i < 4; ++i)
	{
		res.u32[i] = ~(a.u32[i] & b.u32[i]);
	}
	return res;
}

static inline fckc_i128 fck_simd_i128_nor(fckc_i128 a, fckc_i128 b)
{
	fckc_i128 res;
	int       i;
	for (i = 0; i < 4; ++i)
	{
		res.u32[i] = ~(a.u32[i] | b.u32[i]);
	}
	return res;
}

static inline fckc_i128 fck_simd_i128_xnor(fckc_i128 a, fckc_i128 b)
{
	fckc_i128 res;
	int       i;
	for (i = 0; i < 4; ++i)
	{
		res.u32[i] = ~(a.u32[i] ^ b.u32[i]);
	}
	return res;
}

static inline fckc_i128 fck_simd_i128_select(fckc_i128 mask, fckc_i128 a, fckc_i128 b)
{
	fckc_i128 res;
	int       i;
	for (i = 0; i < 4; ++i)
	{
		res.u32[i] = (a.u32[i] & mask.u32[i]) | (b.u32[i] & ~mask.u32[i]);
	}
	return res;
}

// Shifts whole 128-bit vector left by 'imm' bytes (imm <= 16)
static inline fckc_i128 fck_simd_i128_bsll(fckc_i128 a, int imm)
{
	fckc_i128 res;
	int       i;
	memset(&res, 0, sizeof(res));
	if (imm >= 16 || imm < 0) return res;
	for (i = imm; i < 16; ++i)
	{
		res.u8[i] = a.u8[i - imm];
	}
	return res;
}

// Shifts whole 128-bit vector right by 'imm' bytes (imm <= 16)
static inline fckc_i128 fck_simd_i128_bsrl(fckc_i128 a, int imm)
{
	fckc_i128 res;
	int       i;
	memset(&res, 0, sizeof(res));
	if (imm >= 16 || imm < 0) return res;
	for (i = 0; i < 16 - imm; ++i)
	{
		res.u8[i] = a.u8[i + imm];
	}
	return res;
}

static inline fckc_i128 fck_simd_i128_add_i8(fckc_i128 a, fckc_i128 b)
{
	fckc_i128 res;
	int       i;
	for (i = 0; i < 16; ++i)
	{
		res.i8[i] = (fckc_i8)(a.i8[i] + b.i8[i]);
	}
	return res;
}

static inline fckc_i128 fck_simd_i128_add_i16(fckc_i128 a, fckc_i128 b)
{
	fckc_i128 res;
	int       i;
	for (i = 0; i < 8; ++i)
	{
		res.i16[i] = (fckc_i16)(a.i16[i] + b.i16[i]);
	}
	return res;
}

static inline fckc_i128 fck_simd_i128_add_i32(fckc_i128 a, fckc_i128 b)
{
	fckc_i128 res;
	int       i;
	for (i = 0; i < 4; ++i)
	{
		res.i32[i] = a.i32[i] + b.i32[i];
	}
	return res;
}

static inline fckc_i128 fck_simd_i128_sub_i8(fckc_i128 a, fckc_i128 b)
{
	fckc_i128 res;
	int       i;
	for (i = 0; i < 16; ++i)
	{
		res.i8[i] = (fckc_i8)(a.i8[i] - b.i8[i]);
	}
	return res;
}

static inline fckc_i128 fck_simd_i128_sub_i16(fckc_i128 a, fckc_i128 b)
{
	fckc_i128 res;
	int       i;
	for (i = 0; i < 8; ++i)
	{
		res.i16[i] = (fckc_i16)(a.i16[i] - b.i16[i]);
	}
	return res;
}

static inline fckc_i128 fck_simd_i128_sub_i32(fckc_i128 a, fckc_i128 b)
{
	fckc_i128 res;
	int       i;
	for (i = 0; i < 4; ++i)
	{
		res.i32[i] = a.i32[i] - b.i32[i];
	}
	return res;
}

static inline fckc_i128 fck_simd_i128_cmpeq_i8(fckc_i128 a, fckc_i128 b)
{
	fckc_i128 res;
	int       i;
	for (i = 0; i < 16; ++i)
	{
		res.i8[i] = (a.i8[i] == b.i8[i]) ? (fckc_i8)-1 : 0;
	}
	return res;
}

static inline fckc_i128 fck_simd_i128_cmplt_i8(fckc_i128 a, fckc_i128 b)
{
	fckc_i128 res;
	int       i;
	for (i = 0; i < 16; ++i)
	{
		res.i8[i] = (a.i8[i] < b.i8[i]) ? (fckc_i8)-1 : 0;
	}
	return res;
}

static inline fckc_i128 fck_simd_i128_cmpgt_i8(fckc_i128 a, fckc_i128 b)
{
	fckc_i128 res;
	int       i;
	for (i = 0; i < 16; ++i)
	{
		res.i8[i] = (a.i8[i] > b.i8[i]) ? (fckc_i8)-1 : 0;
	}
	return res;
}

static inline fckc_i128 fck_simd_i128_cmpeq_i16(fckc_i128 a, fckc_i128 b)
{
	fckc_i128 res;
	int       i;
	for (i = 0; i < 8; ++i)
	{
		res.i16[i] = (a.i16[i] == b.i16[i]) ? (fckc_i16)-1 : 0;
	}
	return res;
}

static inline fckc_i128 fck_simd_i128_cmpeq_i32(fckc_i128 a, fckc_i128 b)
{
	fckc_i128 res;
	int       i;
	for (i = 0; i < 4; ++i)
	{
		res.i32[i] = (a.i32[i] == b.i32[i]) ? -1 : 0;
	}
	return res;
}

static inline int fck_simd_i128_movemask_i8(fckc_i128 vec)
{
	int mask = 0;
	int i;
	for (i = 0; i < 16; ++i)
	{
		if (vec.u8[i] & 0x80)
		{
			mask |= (1 << i);
		}
	}
	return mask;
}

#endif /* !FCK_SIMD_FALLBACK_H_INCLUDED */