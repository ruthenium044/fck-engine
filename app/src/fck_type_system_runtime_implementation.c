#include "fck_type_system.inl"

#include "fck_serialiser.h"
#include "fck_serialiser_vt.h"

#include <stdarg.h>

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
	fck_members_add(members, (fck_member_desc){.type = member_type, .name = name, .owner = owner, .stride = stride});
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

static fckc_size_t fck_type_add_members_n(struct fck_members *members, fck_type owner, const char *type_name, fckc_size_t size,
                                          fckc_size_t stride, ...)
{
	va_list args;

	va_start(args, owner);

	fck_type type = fck_types_find_from_string(owner.types, fck_id(fckc_f32));

	for (;;)
	{
		char *name = va_arg(args, char *);
		if (name == NULL)
		{
			break;
		}
		fck_members_add(members, (fck_member_desc){.type = type, .name = name, .owner = owner, .stride = stride});
		stride = stride + size;
	}
	va_end(args);
	return stride;
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

	struct fck_members *m = members;
	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_f32x2)), fck_id(fckc_f32), sizeof(fckc_f32), 0, "x", "y", NULL);
	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_f32x3)), fck_id(fckc_f32), sizeof(fckc_f32), 0, "x", "y", "z", NULL);
	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_f32x4)), fck_id(fckc_f32), sizeof(fckc_f32), 0, "x", "y", "z", "w", NULL);

	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_f64x2)), fck_id(fckc_f64), sizeof(fckc_f64), 0, "x", "y", NULL);
	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_f64x3)), fck_id(fckc_f64), sizeof(fckc_f64), 0, "x", "y", "z", NULL);
	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_f64x4)), fck_id(fckc_f64), sizeof(fckc_f64), 0, "x", "y", "z", "w", NULL);

	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_i8x2)), fck_id(fckc_i8), sizeof(fckc_i8), 0, "x", "y", NULL);
	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_i8x3)), fck_id(fckc_i8), sizeof(fckc_i8), 0, "x", "y", "z", NULL);
	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_i8x4)), fck_id(fckc_i8), sizeof(fckc_i8), 0, "x", "y", "z", "w", NULL);

	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_i16x2)), fck_id(fckc_i16), sizeof(fckc_i16), 0, "x", "y", NULL);
	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_i16x3)), fck_id(fckc_i16), sizeof(fckc_i16), 0, "x", "y", "z", NULL);
	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_i16x4)), fck_id(fckc_i16), sizeof(fckc_i16), 0, "x", "y", "z", "w", NULL);

	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_i32x2)), fck_id(fckc_i32), sizeof(fckc_i32), 0, "x", "y", NULL);
	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_i32x3)), fck_id(fckc_i32), sizeof(fckc_i32), 0, "x", "y", "z", NULL);
	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_i32x4)), fck_id(fckc_i32), sizeof(fckc_i32), 0, "x", "y", "z", "w", NULL);

	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_i64x2)), fck_id(fckc_i64), sizeof(fckc_i64), 0, "x", "y", NULL);
	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_i64x3)), fck_id(fckc_i64), sizeof(fckc_i64), 0, "x", "y", "z", NULL);
	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_i64x4)), fck_id(fckc_i64), sizeof(fckc_i64), 0, "x", "y", "z", "w", NULL);

	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_u8x2)), fck_id(fckc_u8), sizeof(fckc_u8), 0, "x", "y", NULL);
	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_u8x3)), fck_id(fckc_u8), sizeof(fckc_u8), 0, "x", "y", "z", NULL);
	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_u8x4)), fck_id(fckc_u8), sizeof(fckc_u8), 0, "x", "y", "z", "w", NULL);

	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_u16x2)), fck_id(fckc_u16), sizeof(fckc_u16), 0, "x", "y", NULL);
	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_u16x3)), fck_id(fckc_u16), sizeof(fckc_u16), 0, "x", "y", "z", NULL);
	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_u16x4)), fck_id(fckc_u16), sizeof(fckc_u16), 0, "x", "y", "z", "w", NULL);

	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_u32x2)), fck_id(fckc_u32), sizeof(fckc_u32), 0, "x", "y", NULL);
	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_u32x3)), fck_id(fckc_u32), sizeof(fckc_u32), 0, "x", "y", "z", NULL);
	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_u32x4)), fck_id(fckc_u32), sizeof(fckc_u32), 0, "x", "y", "z", "w", NULL);

	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_u64x2)), fck_id(fckc_u64), sizeof(fckc_u64), 0, "x", "y", NULL);
	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_u64x3)), fck_id(fckc_u64), sizeof(fckc_u64), 0, "x", "y", "z", NULL);
	fck_type_add_members_n(m, fck_declare(types, fck_id(fckc_u64x4)), fck_id(fckc_u64), sizeof(fckc_u64), 0, "x", "y", "z", "w", NULL);
} 