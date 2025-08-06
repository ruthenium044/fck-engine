#ifndef FCK_SERIALISER_VT_H_INCLUDED
#define FCK_SERIALISER_VT_H_INCLUDED

#include <fckc_inttypes.h>

// Only needed if a serialiser implementation is desired!

struct fck_serialiser;
struct fck_serialiser_params;

// TODO: Remove member_info... It DOES NOT make sense

typedef struct fck_serialiser_vt
{
	void (*i8)(struct fck_serialiser *, struct fck_serialiser_params*, fckc_i8 *, fckc_size_t);
	void (*i16)(struct fck_serialiser *, struct fck_serialiser_params*, fckc_i16 *, fckc_size_t);
	void (*i32)(struct fck_serialiser *, struct fck_serialiser_params*, fckc_i32 *, fckc_size_t);
	void (*i64)(struct fck_serialiser *, struct fck_serialiser_params*, fckc_i64 *, fckc_size_t);
	void (*u8)(struct fck_serialiser *, struct fck_serialiser_params*, fckc_u8 *, fckc_size_t);
	void (*u16)(struct fck_serialiser *, struct fck_serialiser_params*, fckc_u16 *, fckc_size_t);
	void (*u32)(struct fck_serialiser *, struct fck_serialiser_params*, fckc_u32 *, fckc_size_t);
	void (*u64)(struct fck_serialiser *, struct fck_serialiser_params*, fckc_u64 *, fckc_size_t);
	void (*f32)(struct fck_serialiser *, struct fck_serialiser_params*, float *, fckc_size_t);
	void (*f64)(struct fck_serialiser *, struct fck_serialiser_params*, double *, fckc_size_t);
	void (*string)(struct fck_serialiser *, struct fck_serialiser_params*, fck_lstring *, fckc_size_t);
} fck_serialiser_vt;

#endif // FCK_SERIALISER_VT_H_INCLUDED