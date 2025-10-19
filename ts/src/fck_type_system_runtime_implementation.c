#include "fck_type_system.inl"

#include "fck_serialiser_vt.h"

// TODO: THIS FILE SUUUUCKS
// TODO: SERIOUSLY, FUCK THIS FILE. THIS FILE IS THE FUCKING WORST

#define fck_setup_base_primitive(types, serialisers, type, serialise)                                                                      \
	fck_marshal_add(serialisers, (fck_marshal_desc){fck_types_add(types, (fck_type_desc){fck_id(type)}), (fck_marshal_func *)(serialise)})

void fck_serialise_f32(fck_serialiser *serialiser, fck_marshal_params *params, fckc_f32 *value, fckc_size_t count)
{
	fck_serialiser_params p = (fck_serialiser_params){.name = params->name};
	serialiser->vt->f32(serialiser, &p, value, count);
}
void fck_serialise_f64(fck_serialiser *serialiser, fck_marshal_params *params, fckc_f64 *value, fckc_size_t count)
{
	fck_serialiser_params p = (fck_serialiser_params){.name = params->name};
	serialiser->vt->f64(serialiser, &p, value, count);
}
void fck_serialise_i8(fck_serialiser *serialiser, fck_marshal_params *params, fckc_i8 *value, fckc_size_t count)
{
	fck_serialiser_params p = (fck_serialiser_params){.name = params->name};
	serialiser->vt->i8(serialiser, &p, value, count);
}
void fck_serialise_i16(fck_serialiser *serialiser, fck_marshal_params *params, fckc_i16 *value, fckc_size_t count)
{
	fck_serialiser_params p = (fck_serialiser_params){.name = params->name};
	serialiser->vt->i16(serialiser, &p, value, count);
}
void fck_serialise_i32(fck_serialiser *serialiser, fck_marshal_params *params, fckc_i32 *value, fckc_size_t count)
{
	fck_serialiser_params p = (fck_serialiser_params){.name = params->name};
	serialiser->vt->i32(serialiser, &p, value, count);
}
void fck_serialise_i64(fck_serialiser *serialiser, fck_marshal_params *params, fckc_i64 *value, fckc_size_t count)
{
	fck_serialiser_params p = (fck_serialiser_params){.name = params->name};
	serialiser->vt->i64(serialiser, &p, value, count);
}
void fck_serialise_u8(fck_serialiser *serialiser, fck_marshal_params *params, fckc_u8 *value, fckc_size_t count)
{
	fck_serialiser_params p = (fck_serialiser_params){.name = params->name};
	serialiser->vt->u8(serialiser, &p, value, count);
}
void fck_serialise_u16(fck_serialiser *serialiser, fck_marshal_params *params, fckc_u16 *value, fckc_size_t count)
{
	fck_serialiser_params p = (fck_serialiser_params){.name = params->name};
	serialiser->vt->u16(serialiser, &p, value, count);
}
void fck_serialise_u32(fck_serialiser *serialiser, fck_marshal_params *params, fckc_u32 *value, fckc_size_t count)
{
	fck_serialiser_params p = (fck_serialiser_params){.name = params->name};
	serialiser->vt->u32(serialiser, &p, value, count);
}
void fck_serialise_u64(fck_serialiser *serialiser, fck_marshal_params *params, fckc_u64 *value, fckc_size_t count)
{
	fck_serialiser_params p = (fck_serialiser_params){.name = params->name};
	serialiser->vt->u64(serialiser, &p, value, count);
}

void fck_serialise_type(fck_serialiser *serialiser, fck_marshal_params *params, fck_type *value, fckc_size_t count)
{
	for (fckc_size_t index = 0; index < count; index++)
	{
		fck_serialise_u64(serialiser, params, &value[index].hash, 1);
	}
}
void fck_serialise_member(fck_serialiser *serialiser, fck_marshal_params *params, fck_member *value, fckc_size_t count)
{
	for (fckc_size_t index = 0; index < count; index++)
	{
		fck_serialise_u64(serialiser, params, &value[index].hash, 1);
	}
}
void fck_serialise_identifier(fck_serialiser *serialiser, fck_marshal_params *params, fck_identifier *value, fckc_size_t count)
{
	for (fckc_size_t index = 0; index < count; index++)
	{
		fck_serialise_u64(serialiser, params, &value[index].hash, 1);
	}
}
void fck_serialise_type_info(fck_serialiser *serialiser, fck_marshal_params *params, fck_type_info *value, fckc_size_t count)
{
	for (fckc_size_t index = 0; index < count; index++)
	{
		fck_type_info *info = value + index;
		// params->name = "hash";
		// fck_serialise_u64(serialiser, params, &info->hash, 1);
		params->name = "identifier";
		fck_serialise_identifier(serialiser, params, &info->identifier, 1);
		params->name = "first";
		fck_serialise_member(serialiser, params, &info->first_member, 1);
		params->name = "last";
		fck_serialise_member(serialiser, params, &info->last_member, 1);
	}
}

void fck_serialise_member_info(fck_serialiser *serialiser, fck_marshal_params *params, fck_member_info *value, fckc_size_t count)
{
	char buffer[128];
	for (fckc_size_t index = 0; index < count; index++)
	{
		fck_member_info *info = value + index;
		// params->name = "hash";
		// fck_serialise_u64(serialiser, params, &info->hash, 1);
		params->name = "owner";
		fck_serialise_type(serialiser, params, &info->owner, 1);
		params->name = "type";
		fck_serialise_type(serialiser, params, &info->type, 1);
		params->name = "identifier";
		fck_serialise_identifier(serialiser, params, &info->identifier, 1);

		params->name = "extra count";
		fckc_u64 extra_count = info->extra_count;
		fck_serialise_u64(serialiser, params, &extra_count, 1);
		info->extra_count = (fckc_size_t)extra_count;

		params->name = "stride";
		fckc_u64 stride = info->stride;
		fck_serialise_u64(serialiser, params, &stride, 1);
		info->stride = (fckc_size_t)stride;

		params->name = "next";
		fck_serialise_member(serialiser, params, &info->next, 1);
	}
}

static fck_type fck_declare(struct fck_types *types, const char *name)
{
	return fck_types_add(types, (fck_type_desc){.name = name});
}

void fck_type_system_setup_core(struct fck_types *types, struct fck_members *members, struct fck_marshal *serialisers)
{
	struct fck_types *t = types;
	struct fck_members *m = members;
	struct fck_marshal *s = serialisers;

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

	// fck_setup_base_primitive(t, s, fck_identifier, fck_serialise_identifier);
	// fck_setup_base_primitive(t, s, fck_type, fck_serialise_type);
	// fck_setup_base_primitive(t, s, fck_member, fck_serialise_member);
	// fck_setup_base_primitive(t, s, fck_type_info, fck_serialise_type_info);
	// fck_setup_base_primitive(t, s, fck_member_info, fck_serialise_member_info);

	fck_setup_base_primitive(t, s, fck_identifiers, fck_serialise_identifiers);
	fck_setup_base_primitive(t, s, fck_types, fck_serialise_types);
	fck_setup_base_primitive(t, s, fck_members, fck_serialise_members);

	fck_type id_type = fck_types_find_from_string(t, fck_id(fck_identifier));
	fck_type type_type = fck_types_find_from_string(t, fck_id(fck_type));
	fck_type member_type = fck_types_find_from_string(t, fck_id(fck_member));

	fck_type type_info_type = fck_types_find_from_string(t, fck_id(fck_type_info));
	fck_type member_info_type = fck_types_find_from_string(t, fck_id(fck_member_info));

	fck_type identifiers_type = fck_types_find_from_string(t, fck_id(fck_identifiers));
	fck_type types_type = fck_types_find_from_string(t, fck_id(fck_types));
	fck_type members_type = fck_types_find_from_string(t, fck_id(fck_members));

	fck_type assembly_type = fck_declare(t, fck_id(fck_assembly));
	fck_members_add(m, assembly_type, fck_value_decl(fck_assembly, identifiers_type, identifiers));
	fck_members_add(m, assembly_type, fck_value_decl(fck_assembly, types_type, types));
	fck_members_add(m, assembly_type, fck_value_decl(fck_assembly, members_type, members));

	// fck_setup_base_primitive(t, s, fck_assembly, fck_serialise_assembly);
}