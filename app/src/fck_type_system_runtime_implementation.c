#include "fck_type_system.h"

#include "fck_serialiser.h"
#include "fck_serialiser_vt.h"

#define fck_setup_base_primitive(types, serialisers, type, serialise)                                                                      \
	fck_serialise_interfaces_add(                                                                                                          \
		serialisers, (fck_serialise_desc){fck_types_add(types, (fck_type_desc){fck_name(type)}), (fck_serialise_func *)(serialise)})

void fck_serialise_float(float *value, fckc_size_t count, fck_serialiser *serialiser, fck_serialiser_params *params)
{
	serialiser->vt->f32(serialiser, params, value, count);
}
void fck_serialise_double(double *value, fckc_size_t count, fck_serialiser *serialiser, fck_serialiser_params *params)
{
	serialiser->vt->f64(serialiser, params, value, count);
}
void fck_serialise_i8(fckc_i8 *value, fckc_size_t count, fck_serialiser *serialiser, fck_serialiser_params *params)
{
	serialiser->vt->i8(serialiser, params, value, count);
}
void fck_serialise_i16(fckc_i16 *value, fckc_size_t count, fck_serialiser *serialiser, fck_serialiser_params *params)
{
	serialiser->vt->i16(serialiser, params, value, count);
}
void fck_serialise_i32(fckc_i32 *value, fckc_size_t count, fck_serialiser *serialiser, fck_serialiser_params *params)
{
	serialiser->vt->i32(serialiser, params, value, count);
}
void fck_serialise_i64(fckc_i64 *value, fckc_size_t count, fck_serialiser *serialiser, fck_serialiser_params *params)
{
	serialiser->vt->i64(serialiser, params, value, count);
}
void fck_serialise_u8(fckc_u8 *value, fckc_size_t count, fck_serialiser *serialiser, fck_serialiser_params *params)
{
	serialiser->vt->u8(serialiser, params, value, count);
}
void fck_serialise_u16(fckc_u16 *value, fckc_size_t count, fck_serialiser *serialiser, fck_serialiser_params *params)
{
	serialiser->vt->u16(serialiser, params, value, count);
}
void fck_serialise_u32(fckc_u32 *value, fckc_size_t count, fck_serialiser *serialiser, fck_serialiser_params *params)
{
	serialiser->vt->u32(serialiser, params, value, count);
}
void fck_serialise_u64(fckc_u64 *value, fckc_size_t count, fck_serialiser *serialiser, fck_serialiser_params *params)
{
	serialiser->vt->u64(serialiser, params, value, count);
}

void fck_type_add_f32(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type member_type = fck_types_find_from_string(type.types, fck_name(float));
	fck_members_add(members, (fck_member_desc){.type = member_type, .name = name, .owner = type, .stride = stride});
}
void fck_type_add_f64(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type member_type = fck_types_find_from_string(type.types, fck_name(double));
	fck_members_add(members, (fck_member_desc){.type = member_type, .name = name, .owner = type, .stride = stride});
}

void fck_type_add_i8(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type member_type = fck_types_find_from_string(type.types, fck_name(fckc_i8));
	fck_members_add(members, (fck_member_desc){.type = member_type, .name = name, .owner = type, .stride = stride});
}
void fck_type_add_i16(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type member_type = fck_types_find_from_string(type.types, fck_name(fckc_i16));
	fck_members_add(members, (fck_member_desc){.type = member_type, .name = name, .owner = type, .stride = stride});
}
void fck_type_add_i32(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type member_type = fck_types_find_from_string(type.types, fck_name(fckc_i32));
	fck_members_add(members, (fck_member_desc){.type = member_type, .name = name, .owner = type, .stride = stride});
}
void fck_type_add_i64(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type member_type = fck_types_find_from_string(type.types, fck_name(fckc_i64));
	fck_members_add(members, (fck_member_desc){.type = member_type, .name = name, .owner = type, .stride = stride});
}

void fck_type_add_u8(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type member_type = fck_types_find_from_string(type.types, fck_name(fckc_u8));
	fck_members_add(members, (fck_member_desc){.type = member_type, .name = name, .owner = type, .stride = stride});
}
void fck_type_add_u16(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type member_type = fck_types_find_from_string(type.types, fck_name(fckc_u16));
	fck_members_add(members, (fck_member_desc){.type = member_type, .name = name, .owner = type, .stride = stride});
}
void fck_type_add_u32(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type member_type = fck_types_find_from_string(type.types, fck_name(fckc_u32));
	fck_members_add(members, (fck_member_desc){.type = member_type, .name = name, .owner = type, .stride = stride});
}
void fck_type_add_u64(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type member_type = fck_types_find_from_string(type.types, fck_name(fckc_u64));
	fck_members_add(members, (fck_member_desc){.type = member_type, .name = name, .owner = type, .stride = stride});
}

void fck_type_add_f32x2(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type member_type = fck_types_find_from_string(type.types, fck_name(fckc_f32x2));
	fck_members_add(members, (fck_member_desc){.type = member_type, .name = name, .owner = type, .stride = stride});
}
void fck_type_add_f32x3(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type member_type = fck_types_find_from_string(type.types, fck_name(fckc_f32x3));
	fck_members_add(members, (fck_member_desc){.type = member_type, .name = name, .owner = type, .stride = stride});
}

void fck_type_add_i32x2(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type member_type = fck_types_find_from_string(type.types, fck_name(i32x2));
	fck_members_add(members, (fck_member_desc){.type = member_type, .name = name, .owner = type, .stride = stride});
}
void fck_type_add_i32x3(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type member_type = fck_types_find_from_string(type.types, fck_name(i32x3));
	fck_members_add(members, (fck_member_desc){.type = member_type, .name = name, .owner = type, .stride = stride});
}

void fck_type_system_setup_primitives(struct fck_types *types, struct fck_members *members, struct fck_serialise_interfaces *serialisers)
{
	fck_setup_base_primitive(types, serialisers, float, fck_serialise_float);
	fck_setup_base_primitive(types, serialisers, double, fck_serialise_double);
	fck_setup_base_primitive(types, serialisers, fckc_f32, fck_serialise_float);
	fck_setup_base_primitive(types, serialisers, fckc_f64, fck_serialise_double);
	fck_setup_base_primitive(types, serialisers, fckc_i8, fck_serialise_i8);
	fck_setup_base_primitive(types, serialisers, fckc_i16, fck_serialise_i16);
	fck_setup_base_primitive(types, serialisers, fckc_i32, fck_serialise_i32);
	fck_setup_base_primitive(types, serialisers, fckc_i64, fck_serialise_i64);
	fck_setup_base_primitive(types, serialisers, fckc_u8, fck_serialise_u8);
	fck_setup_base_primitive(types, serialisers, fckc_u16, fck_serialise_u16);
	fck_setup_base_primitive(types, serialisers, fckc_u32, fck_serialise_u32);
	fck_setup_base_primitive(types, serialisers, fckc_u64, fck_serialise_u64);

	// TODO: Add all the others
	fck_type f32x2_type_handle = fck_types_add(types, (fck_type_desc){fck_name(fckc_f32x2)});
	fck_type_add_f32(members, f32x2_type_handle, "x", sizeof(float) * 0);
	fck_type_add_f32(members, f32x2_type_handle, "y", sizeof(float) * 1);

	fck_type f32x3_type_handle = fck_types_add(types, (fck_type_desc){fck_name(fckc_f32x3)});
	fck_type_add_f32(members, f32x3_type_handle, "x", sizeof(float) * 0);
	fck_type_add_f32(members, f32x3_type_handle, "y", sizeof(float) * 1);
	fck_type_add_f32(members, f32x3_type_handle, "z", sizeof(float) * 2);

	fck_type i32x2_type_handle = fck_types_add(types, (fck_type_desc){fck_name(fckc_i32x2)});
	fck_type_add_i32(members, i32x2_type_handle, "x", sizeof(fckc_i32) * 0);
	fck_type_add_i32(members, i32x2_type_handle, "y", sizeof(fckc_i32) * 1);

	fck_type i32x3_type_handle = fck_types_add(types, (fck_type_desc){fck_name(fckc_i32x3)});
	fck_type_add_i32(members, i32x3_type_handle, "x", sizeof(fckc_i32) * 0);
	fck_type_add_i32(members, i32x3_type_handle, "y", sizeof(fckc_i32) * 1);
	fck_type_add_i32(members, i32x3_type_handle, "z", sizeof(fckc_i32) * 2);
}