#include "fck_type_system.inl"

#include "fck_serialiser.h"
#include "fck_serialiser_vt.h"

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_stdinc.h>

// TODO: THIS FILE SUUUUCKS
// TODO: SERIOUSLY, FUCK THIS FILE. THIS FILE IS THE FUCKING WORST

#define fck_setup_base_primitive(types, serialisers, type, serialise)                                                                      \
	fck_serialise_interfaces_add(                                                                                                          \
		serialisers, (fck_serialise_desc){fck_types_add(types, (fck_type_desc){fck_id(type)}), (fck_serialise_func *)(serialise)})

void fck_serialise_f32(fck_serialiser *serialiser, fck_serialiser_params *params, fckc_f32 *value, fckc_size_t count)
{
	serialiser->vt->f32(serialiser, params, value, count);
}
void fck_serialise_f64(fck_serialiser *serialiser, fck_serialiser_params *params, fckc_f64 *value, fckc_size_t count)
{
	serialiser->vt->f64(serialiser, params, value, count);
}
void fck_serialise_i8(fck_serialiser *serialiser, fck_serialiser_params *params, fckc_i8 *value, fckc_size_t count)
{
	serialiser->vt->i8(serialiser, params, value, count);
}
void fck_serialise_i16(fck_serialiser *serialiser, fck_serialiser_params *params, fckc_i16 *value, fckc_size_t count)
{
	serialiser->vt->i16(serialiser, params, value, count);
}
void fck_serialise_i32(fck_serialiser *serialiser, fck_serialiser_params *params, fckc_i32 *value, fckc_size_t count)
{
	serialiser->vt->i32(serialiser, params, value, count);
}
void fck_serialise_i64(fck_serialiser *serialiser, fck_serialiser_params *params, fckc_i64 *value, fckc_size_t count)
{
	serialiser->vt->i64(serialiser, params, value, count);
}
void fck_serialise_u8(fck_serialiser *serialiser, fck_serialiser_params *params, fckc_u8 *value, fckc_size_t count)
{
	serialiser->vt->u8(serialiser, params, value, count);
}
void fck_serialise_u16(fck_serialiser *serialiser, fck_serialiser_params *params, fckc_u16 *value, fckc_size_t count)
{
	serialiser->vt->u16(serialiser, params, value, count);
}
void fck_serialise_u32(fck_serialiser *serialiser, fck_serialiser_params *params, fckc_u32 *value, fckc_size_t count)
{
	serialiser->vt->u32(serialiser, params, value, count);
}
void fck_serialise_u64(fck_serialiser *serialiser, fck_serialiser_params *params, fckc_u64 *value, fckc_size_t count)
{
	serialiser->vt->u64(serialiser, params, value, count);
}

static void fck_type_add_member(struct fck_members *members, fck_type owner, const char *type_name, const char *name, fckc_size_t stride)
{
	// TODO: string kind of dumb tho
	fck_type member_type = fck_types_find_from_string(owner.types, type_name);
	fck_members_add(members, owner, (fck_member_desc){.type = member_type, .name = name, .stride = stride});
}

void fck_type_add_f32(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type_add_member(members, type, fck_id(fckc_f32), name, stride);
}

void fck_type_add_f64(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type_add_member(members, type, fck_id(fckc_f64), name, stride);
}

void fck_type_add_i8(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type_add_member(members, type, fck_id(fckc_i8), name, stride);
}
void fck_type_add_i16(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type_add_member(members, type, fck_id(fckc_i16), name, stride);
}
void fck_type_add_i32(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type_add_member(members, type, fck_id(fckc_i32), name, stride);
}
void fck_type_add_i64(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type_add_member(members, type, fck_id(fckc_i64), name, stride);
}

void fck_type_add_u8(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type_add_member(members, type, fck_id(fckc_u8), name, stride);
}
void fck_type_add_u16(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type_add_member(members, type, fck_id(fckc_u16), name, stride);
}
void fck_type_add_u32(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type_add_member(members, type, fck_id(fckc_u32), name, stride);
}
void fck_type_add_u64(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type_add_member(members, type, fck_id(fckc_u64), name, stride);
}

void fck_type_add_f32x2(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type_add_member(members, type, fck_id(fckc_f32x2), name, stride);
}
void fck_type_add_f32x3(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type_add_member(members, type, fck_id(fckc_f32x3), name, stride);
}
void fck_type_add_f32x4(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type_add_member(members, type, fck_id(fckc_f32x4), name, stride);
}

void fck_type_add_f64x2(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type_add_member(members, type, fck_id(fckc_f64x2), name, stride);
}
void fck_type_add_f64x3(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type_add_member(members, type, fck_id(fckc_f64x3), name, stride);
}
void fck_type_add_f64x4(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type_add_member(members, type, fck_id(fckc_f64x4), name, stride);
}

void fck_type_add_i8x2(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type_add_member(members, type, fck_id(fckc_i8x2), name, stride);
}
void fck_type_add_i8x3(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type_add_member(members, type, fck_id(fckc_i8x3), name, stride);
}
void fck_type_add_i8x4(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type_add_member(members, type, fck_id(fckc_i8x4), name, stride);
}

void fck_type_add_i16x2(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type_add_member(members, type, fck_id(fckc_i16x2), name, stride);
}
void fck_type_add_i16x3(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type_add_member(members, type, fck_id(fckc_i16x3), name, stride);
}
void fck_type_add_i16x4(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type_add_member(members, type, fck_id(fckc_i16x4), name, stride);
}

void fck_type_add_i32x2(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type_add_member(members, type, fck_id(fckc_i32x2), name, stride);
}
void fck_type_add_i32x3(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type_add_member(members, type, fck_id(fckc_i32x3), name, stride);
}
void fck_type_add_i32x4(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type_add_member(members, type, fck_id(fckc_i32x4), name, stride);
}

void fck_type_add_i64x2(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type_add_member(members, type, fck_id(fckc_i64x2), name, stride);
}
void fck_type_add_i64x3(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type_add_member(members, type, fck_id(fckc_i64x3), name, stride);
}
void fck_type_add_i64x4(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type_add_member(members, type, fck_id(fckc_i64x4), name, stride);
}

void fck_type_add_u8x2(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type_add_member(members, type, fck_id(fckc_u8x2), name, stride);
}
void fck_type_add_u8x3(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type_add_member(members, type, fck_id(fckc_u8x3), name, stride);
}
void fck_type_add_u8x4(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type_add_member(members, type, fck_id(fckc_u8x4), name, stride);
}

void fck_type_add_u16x2(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type_add_member(members, type, fck_id(fckc_u16x2), name, stride);
}
void fck_type_add_u16x3(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type_add_member(members, type, fck_id(fckc_u16x3), name, stride);
}
void fck_type_add_u16x4(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type_add_member(members, type, fck_id(fckc_u16x4), name, stride);
}

void fck_type_add_u32x2(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type_add_member(members, type, fck_id(fckc_u32x2), name, stride);
}
void fck_type_add_u32x3(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type_add_member(members, type, fck_id(fckc_u32x3), name, stride);
}
void fck_type_add_u32x4(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type_add_member(members, type, fck_id(fckc_u32x4), name, stride);
}

void fck_type_add_u64x2(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type_add_member(members, type, fck_id(fckc_u64x2), name, stride);
}
void fck_type_add_u64x3(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type_add_member(members, type, fck_id(fckc_u64x3), name, stride);
}
void fck_type_add_u64x4(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type_add_member(members, type, fck_id(fckc_u64x4), name, stride);
}

static fck_member fck_type_add_members_n(struct fck_members *members, fck_type owner, const char *type_name, fckc_size_t count)
{
	fck_type type = fck_types_find_from_string(owner.types, type_name);
	return fck_members_add(members, owner,
	                       (fck_member_desc){.type = type, .name = fck_name(values), .stride = 0, .extra_count = count - 1});
}

static fck_type fck_declare(struct fck_types *types, const char *name)
{
	return fck_types_add(types, (fck_type_desc){.name = name});
}

void fck_type_system_setup_core(struct fck_types *types, struct fck_members *members, struct fck_serialise_interfaces *serialisers)
{
	struct fck_types *t = types;
	struct fck_serialise_interfaces *s = serialisers;

	// The backbone :frown:
	fck_setup_base_primitive(t, s, fckc_f32, fck_serialise_f32);
	fck_setup_base_primitive(t, s, fckc_f64, fck_serialise_f64);
	fck_setup_base_primitive(t, s, fckc_i8, fck_serialise_i8);
	fck_setup_base_primitive(t, s, fckc_i16, fck_serialise_i16);
	fck_setup_base_primitive(t, s, fckc_i32, fck_serialise_i32);
	fck_setup_base_primitive(t, s, fckc_i64, fck_serialise_i64);
	fck_setup_base_primitive(t, s, fckc_u8, fck_serialise_u8);
	fck_setup_base_primitive(t, s, fckc_u16, fck_serialise_u16);
	fck_setup_base_primitive(t, s, fckc_u32, fck_serialise_u32);
	fck_setup_base_primitive(t, s, fckc_u64, fck_serialise_u64);

	return;
	struct fck_members *m = members;
	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_f32x2)), fck_id(fckc_f32), 2);
	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_f32x3)), fck_id(fckc_f32), 3);
	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_f32x4)), fck_id(fckc_f32), 4);

	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_f64x2)), fck_id(fckc_f64), 2);
	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_f64x3)), fck_id(fckc_f64), 3);
	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_f64x4)), fck_id(fckc_f64), 4);

	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_i8x2)), fck_id(fckc_i8), 2);
	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_i8x3)), fck_id(fckc_i8), 3);
	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_i8x4)), fck_id(fckc_i8), 4);

	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_i16x2)), fck_id(fckc_i16), 2);
	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_i16x3)), fck_id(fckc_i16), 3);
	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_i16x4)), fck_id(fckc_i16), 4);

	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_i32x2)), fck_id(fckc_i32), 2);
	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_i32x3)), fck_id(fckc_i32), 3);
	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_i32x4)), fck_id(fckc_i32), 4);

	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_i64x2)), fck_id(fckc_i64), 2);
	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_i64x3)), fck_id(fckc_i64), 3);
	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_i64x4)), fck_id(fckc_i64), 4);

	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_u8x2)), fck_id(fckc_u8), 2);
	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_u8x3)), fck_id(fckc_u8), 3);
	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_u8x4)), fck_id(fckc_u8), 4);

	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_u16x2)), fck_id(fckc_u16), 2);
	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_u16x3)), fck_id(fckc_u16), 3);
	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_u16x4)), fck_id(fckc_u16), 4);

	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_u32x2)), fck_id(fckc_u32), 2);
	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_u32x3)), fck_id(fckc_u32), 3);
	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_u32x4)), fck_id(fckc_u32), 4);

	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_u64x2)), fck_id(fckc_u64), 2);
	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_u64x3)), fck_id(fckc_u64), 3);
	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_u64x4)), fck_id(fckc_u64), 4);
}