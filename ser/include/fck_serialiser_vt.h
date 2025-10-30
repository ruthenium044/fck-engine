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
	// Maybe we should have fck_serialiser_o so we can always pass the serialiser around with type safety?
} fck_serialiser;

struct fck_type;
struct fck_type_system;

typedef struct fck_serialiser_params
{
	// Variable name - Makes sense
	const char *name;
} fck_serialiser_params;

// Prettifying as object or composed structure requires marshal param
struct fck_marshaller;
struct fck_marshal_params;

typedef struct fck_serialiser_prettify_vt
{
	// Label REALLY needs to get revised!
	int (*label)(struct fck_marshaller *, struct fck_marshal_params *p, void *data, fckc_size_t count, char *buffer, fckc_size_t size);
	int (*tree_push)(struct fck_marshaller *, struct fck_marshal_params *, void *data, fckc_size_t count);
	void (*tree_pop)(struct fck_marshaller *, struct fck_marshal_params *, void *data, fckc_size_t count);
} fck_serialiser_prettify_vt;

typedef struct fck_serialiser_vt
{
	void (*i8)(struct fck_serialiser *s, struct fck_serialiser_params *p, fckc_i8 *v, fckc_size_t c);
	void (*i16)(struct fck_serialiser *s, struct fck_serialiser_params *p, fckc_i16 *v, fckc_size_t c);
	void (*i32)(struct fck_serialiser *s, struct fck_serialiser_params *p, fckc_i32 *v, fckc_size_t c);
	void (*i64)(struct fck_serialiser *s, struct fck_serialiser_params *p, fckc_i64 *v, fckc_size_t c);
	void (*u8)(struct fck_serialiser *s, struct fck_serialiser_params *p, fckc_u8 *v, fckc_size_t c);
	void (*u16)(struct fck_serialiser *s, struct fck_serialiser_params *p, fckc_u16 *v, fckc_size_t c);
	void (*u32)(struct fck_serialiser *s, struct fck_serialiser_params *p, fckc_u32 *v, fckc_size_t c);
	void (*u64)(struct fck_serialiser *s, struct fck_serialiser_params *p, fckc_u64 *v, fckc_size_t c);
	void (*f32)(struct fck_serialiser *s, struct fck_serialiser_params *p, fckc_f32 *v, fckc_size_t c);
	void (*f64)(struct fck_serialiser *s, struct fck_serialiser_params *p, fckc_f64 *v, fckc_size_t c);

	// TODO: REMOVE THIS, REPLACE IT BEFORE IT IS TOO LATE!! FFS!!!
	void (*string)(struct fck_serialiser *s, struct fck_serialiser_params *p, char **v, fckc_size_t c);

	fck_serialiser_prettify_vt *pretty;
} fck_serialiser_vt;

//typedef struct fck_serialiser_api
//{
//	
//}fck_serialiser_api;

#endif // FCK_SERIALISER_VT_H_INCLUDED