
#include "fck_type_system.h"
#include <SDL3/SDL_assert.h>

#include "fck_apis.h"
#include "fck_type_system.inl"

#include "fck_serialiser_vt.h"

static const char *fck_type_system_api_name = "FCK_TYPE_SYSTEM";

typedef struct fck_serialiser_type_size_of
{
	fck_serialiser_vt *vt;
	fckc_size_t size;
} fck_serialiser_type_size_of;

void fck_size_of_i8(fck_serialiser *s, fck_serialiser_params *p, fckc_i8 *v, fckc_size_t c)
{
	fck_serialiser_type_size_of *size_of = (fck_serialiser_type_size_of *)s;
	size_of->size = size_of->size + (sizeof(*v) * c);
}

void fck_size_of_i16(fck_serialiser *s, fck_serialiser_params *p, fckc_i16 *v, fckc_size_t c)
{
	fck_serialiser_type_size_of *size_of = (fck_serialiser_type_size_of *)s;
	size_of->size = size_of->size + (sizeof(*v) * c);
}

void fck_size_of_i32(fck_serialiser *s, fck_serialiser_params *p, fckc_i32 *v, fckc_size_t c)
{
	fck_serialiser_type_size_of *size_of = (fck_serialiser_type_size_of *)s;
	size_of->size = size_of->size + (sizeof(*v) * c);
}

void fck_size_of_i64(fck_serialiser *s, fck_serialiser_params *p, fckc_i64 *v, fckc_size_t c)
{
	fck_serialiser_type_size_of *size_of = (fck_serialiser_type_size_of *)s;
	size_of->size = size_of->size + (sizeof(*v) * c);
}

void fck_size_of_u8(fck_serialiser *s, fck_serialiser_params *p, fckc_u8 *v, fckc_size_t c)
{
	fck_serialiser_type_size_of *size_of = (fck_serialiser_type_size_of *)s;
	size_of->size = size_of->size + (sizeof(*v) * c);
}

void fck_size_of_u16(fck_serialiser *s, fck_serialiser_params *p, fckc_u16 *v, fckc_size_t c)
{
	fck_serialiser_type_size_of *size_of = (fck_serialiser_type_size_of *)s;
	size_of->size = size_of->size + (sizeof(*v) * c);
}

void fck_size_of_u32(fck_serialiser *s, fck_serialiser_params *p, fckc_u32 *v, fckc_size_t c)
{
	fck_serialiser_type_size_of *size_of = (fck_serialiser_type_size_of *)s;
	size_of->size = size_of->size + (sizeof(*v) * c);
}

void fck_size_of_u64(fck_serialiser *s, fck_serialiser_params *p, fckc_u64 *v, fckc_size_t c)
{
	fck_serialiser_type_size_of *size_of = (fck_serialiser_type_size_of *)s;
	size_of->size = size_of->size + (sizeof(*v) * c);
}

void fck_size_of_f32(fck_serialiser *s, fck_serialiser_params *p, float *v, fckc_size_t c)
{
	fck_serialiser_type_size_of *size_of = (fck_serialiser_type_size_of *)s;
	size_of->size = size_of->size + (sizeof(*v) * c);
}

void fck_size_of_f64(fck_serialiser *s, fck_serialiser_params *p, double *v, fckc_size_t c)
{
	fck_serialiser_type_size_of *size_of = (fck_serialiser_type_size_of *)s;
	size_of->size = size_of->size + (sizeof(*v) * c);
}

void fck_size_of_string(fck_serialiser *s, fck_serialiser_params *p, fck_lstring *v, fckc_size_t c)
{
	SDL_assert(false && "Not implemented");
}

static fck_serialiser_vt fck_size_of_vt = {
	.f64 = fck_size_of_f64,
	.f32 = fck_size_of_f32,
	.i8 = fck_size_of_i8,
	.i16 = fck_size_of_i16,
	.i32 = fck_size_of_i32,
	.i64 = fck_size_of_i64,
	.u8 = fck_size_of_u8,
	.u16 = fck_size_of_u16,
	.u32 = fck_size_of_u32,
	.u64 = fck_size_of_u64,
};

typedef struct fck_type_system_api_blob
{
	// Core
	fck_type_system type_system;

	// Children
	fck_identifier_api identifier_api;
	fck_type_api type_api;
	fck_member_api member_api;
	fck_type_system serialise_api;

	struct fck_identifiers *identifiers;
	struct fck_types *types;
	struct fck_members *members;
	struct fck_serialise_interfaces *serialisers;
} fck_type_system_api_blob;

static fck_type_system_api_blob fck_type_system_api_blob_private;

static struct fck_identifiers *get_identifiers(void)
{
	return fck_type_system_api_blob_private.identifiers;
}
static struct fck_types *get_types(void)
{
	return fck_type_system_api_blob_private.types;
}
static struct fck_members *get_members(void)
{
	return fck_type_system_api_blob_private.members;
}
static struct fck_serialise_interfaces *get_serialisers(void)
{
	return fck_type_system_api_blob_private.serialisers;
}

void fck_type_size_of(struct fck_serialiser_type_size_of *s, struct fck_serialiser_params *p, void *self, fckc_size_t c)
{
	// fck_serialiser_type_size_of *size_of = (fck_serialiser_type_size_of *)s;

	fck_serialise_func *serialise = fck_serialise_interfaces_get(fck_type_system_api_blob_private.serialisers, *p->type);
	if (serialise != NULL)
	{
		serialise((fck_serialiser *)s, p, self, c);
		return;
	}

	fck_type_info *info = fck_type_resolve(*p->type);
	fck_member members = fck_type_info_first_member(info);
	for (fckc_size_t index = 0; index < c; index++)
	{
		fck_member current = members;
		while (!fck_member_is_null(current))
		{
			struct fck_member_info *member = fck_member_resolve(current);
			fck_identifier member_identifier = fck_member_info_identify(member);
			fckc_u8 *offset_ptr = ((fckc_u8 *)(self)) + fck_member_info_stride(member);
			fck_type member_type = fck_member_info_type(member);
			fckc_size_t next_count = fck_member_info_count(member);
			fck_serialiser_params parameters = *p;
			parameters.name = fck_identifier_resolve(member_identifier);
			parameters.type = &member_type;

			p->caller((fck_serialiser *)s, &parameters, offset_ptr, next_count);
			current = fck_member_info_next(member);
		}
	}
}

fck_type fck_types_add_api(fck_type_desc desc)
{
	return fck_types_add(fck_type_system_api_blob_private.types, desc);
}

fck_type fck_types_find_from_hash_api(fckc_u64 hash)
{
	return fck_types_find_from_hash(fck_type_system_api_blob_private.types, hash);
}

fck_type fck_types_find_from_string_api(const char *name)
{
	return fck_types_find_from_string(fck_type_system_api_blob_private.types, name);
}

int fck_types_iterate_api(fck_type *type)
{
	return fck_types_iterate(fck_type_system_api_blob_private.types, type);
}

fck_member fck_members_add_api(fck_type owner, fck_member_desc desc)
{
	return fck_members_add(fck_type_system_api_blob_private.members, owner, desc);
}

void fck_serialise_interfaces_add_api(fck_serialise_desc desc)
{
	fck_serialise_interfaces_add(fck_type_system_api_blob_private.serialisers, desc);
}

fck_serialise_func *fck_serialise_interfaces_get_api(fck_type type)
{
	return fck_serialise_interfaces_get(fck_type_system_api_blob_private.serialisers, type);
}

int fck_members_is_stretchy(struct fck_member_info *info)
{
	return info->extra_count == (fckc_size_t)(~0LLU);
}

void fck_load_type_system(struct fck_apis *apis)
{
	// fck_type_system_api_blob *api = (fck_type_system_api_blob *)SDL_malloc(sizeof(*api));
	fck_type_system_api_blob *blob = &fck_type_system_api_blob_private;
	blob->identifiers = fck_identifiers_alloc(1);
	blob->members = fck_members_alloc(blob->identifiers, 1);
	blob->types = fck_types_alloc(blob->identifiers, 1);
	blob->serialisers = fck_serialise_interfaces_alloc(1);
	fck_type_system_setup_core(blob->types, blob->members, blob->serialisers);

	// Make sure nothing breaks this cast - This is how we arrange our memory!
	blob->type_system.identifier = &blob->identifier_api;
	blob->type_system.type = &blob->type_api;
	blob->type_system.member = &blob->member_api;
	blob->type_system.serialise = &blob->serialise_api;

	fck_type_system *ts = (fck_type_system *)blob;

	// Identifiers public API
	ts->identifier->null = fck_identifier_null;
	ts->identifier->is_null = fck_identifier_is_null;
	ts->identifier->is_same = fck_identifier_is_same;
	ts->identifier->resolve = fck_identifier_resolve;

	// Types public API
	ts->type->null = fck_type_null;
	ts->type->is_null = fck_type_is_null;
	ts->type->is_same = fck_type_is_same;
	ts->type->is = fck_type_is;
	ts->type->resolve = fck_type_resolve;

	ts->type->identify = fck_type_info_identify;
	ts->type->members_of = fck_type_info_first_member;

	ts->type->add = fck_types_add_api;
	ts->type->get = fck_types_find_from_hash_api;
	ts->type->find = fck_types_find_from_string_api;
	ts->type->iterate = fck_types_iterate_api;

	// Member public API
	ts->member->null = fck_member_null;
	ts->member->is_null = fck_member_is_null;
	ts->member->is_same = fck_member_is_same;
	ts->member->resolve = fck_member_resolve;
	ts->member->identify = fck_member_info_identify;
	ts->member->owner_of = fck_member_info_owner;
	ts->member->type_of = fck_member_info_type;
	ts->member->next_of = fck_member_info_next;
	ts->member->stride_of = fck_member_info_stride;
	ts->member->count_of = fck_member_info_count;
	ts->member->is_stretchy = fck_members_is_stretchy;

	ts->member->add = fck_members_add_api;
	// Serialiser public API
	ts->serialise->add = fck_serialise_interfaces_add_api;
	ts->serialise->get = fck_serialise_interfaces_get_api;

	apis->add(fck_type_system_api_name, ts);
}

void fck_unload_type_system(struct fck_apis *apis)
{
	apis->remove(fck_type_system_api_name);
}

fck_type_system *fck_get_type_system(struct fck_apis *apis)
{
	// TODO: Should be get
	return apis->find(fck_type_system_api_name);
}