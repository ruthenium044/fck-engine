#ifndef FCK_SERIALISER_VT_H_INCLUDED
#define FCK_SERIALISER_VT_H_INCLUDED

#include <fckc_inttypes.h>

// Only needed if a serialiser implementation is desired!
struct fck_serialiser_params;

// TODO: Remove member_info... It DOES NOT make sense

// Every implemenator has to ensure that the vt is ON TOP!!
// TODO: Maybe implement fck_serialiser as first member? Meh, does not really matter...
typedef struct fck_serialiser
{
	struct fck_serialiser_vt *vt;
} fck_serialiser;

struct fck_type;
struct fck_type_system;

typedef struct fck_serialiser_params
{
	// The previous params if recursive
	struct fck_serialiser_params *parent;

	// Variable name - Makes sense
	const char *name;

	// Reference to the fck type... sure, can get used for tagging
	struct fck_type *type;

	// The type system iteself, else the type is useless - works...
	// Maybe it makes more sense to just hand around the type info itself?
	struct fck_type_system *type_system;

	// TODO: Propagate parameters for special serialisers might be nice
	// We can also easily bind them...
	// void *user;
	//   ...

} fck_serialiser_params;

static inline fck_serialiser_params fck_serialiser_params_next(fck_serialiser_params *prev, const char *name, struct fck_type *type)
{
	fck_serialiser_params p = *prev;
	p.type = type;
	p.name = name;
	return p;
}

typedef struct fck_serialiser_prettify_vt
{
	int (*label)(char *buffer, fckc_size_t size, struct fck_serialiser_params *p, const char *name, fckc_size_t count);
	int (*tree_push)(struct fck_serialiser *, struct fck_serialiser_params *, void *data, fckc_size_t count);
	void (*tree_pop)(struct fck_serialiser *, struct fck_serialiser_params *, void *data, fckc_size_t count);
} fck_serialiser_prettify_vt;

typedef struct fck_serialiser_vt
{
	void (*i8)(struct fck_serialiser *, struct fck_serialiser_params *, fckc_i8 *, fckc_size_t);
	void (*i16)(struct fck_serialiser *, struct fck_serialiser_params *, fckc_i16 *, fckc_size_t);
	void (*i32)(struct fck_serialiser *, struct fck_serialiser_params *, fckc_i32 *, fckc_size_t);
	void (*i64)(struct fck_serialiser *, struct fck_serialiser_params *, fckc_i64 *, fckc_size_t);
	void (*u8)(struct fck_serialiser *, struct fck_serialiser_params *, fckc_u8 *, fckc_size_t);
	void (*u16)(struct fck_serialiser *, struct fck_serialiser_params *, fckc_u16 *, fckc_size_t);
	void (*u32)(struct fck_serialiser *, struct fck_serialiser_params *, fckc_u32 *, fckc_size_t);
	void (*u64)(struct fck_serialiser *, struct fck_serialiser_params *, fckc_u64 *, fckc_size_t);
	void (*f32)(struct fck_serialiser *, struct fck_serialiser_params *, float *, fckc_size_t);
	void (*f64)(struct fck_serialiser *, struct fck_serialiser_params *, double *, fckc_size_t);
	void (*string)(struct fck_serialiser *, struct fck_serialiser_params *, fck_lstring *, fckc_size_t);

	fck_serialiser_prettify_vt *pretty;
} fck_serialiser_vt;

#endif // FCK_SERIALISER_VT_H_INCLUDED