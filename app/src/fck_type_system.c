
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

// TODO: fck_module? fck_unit? Any of these instead of the blob... This way we can get rid of the static
// TODO: Add user-provided kll allocator!
// TODO: fck_memory_api? Something like this for the type system. We use it to create objects through it!

fck_assembly *fck_assembly_alloc()
{
	// Assembly
	// -- Identifiers
	// -- Types
	// -- Members
	// -- Serialisers
	// TODO: Make sub-modules aware of the assembly. This way it may or may not be possible to
	// traverse upward? Idk.
	fck_assembly *assembly = (fck_assembly *)SDL_malloc(sizeof(*assembly));
	assembly->identifiers.value = fck_identifier_registry_alloc(assembly, 1);
	assembly->types.value = fck_type_registry_alloc(assembly, &assembly->identifiers, 1);
	assembly->members.value = fck_member_registry_alloc(assembly, &assembly->identifiers, 1);
	assembly->serialisers.value = fck_serialiser_registry_alloc(assembly, 1);
	fck_type_system_setup_core(&assembly->types, &assembly->members, &assembly->serialisers);

	return assembly;
}

void fck_assembly_free(fck_assembly *assembly)
{
	fck_serialise_interfaces_free(&assembly->serialisers);
	fck_members_free(&assembly->members);
	fck_types_free(&assembly->types);
	fck_identifiers_free(&assembly->identifiers);
	SDL_free(assembly);
}

typedef struct fck_type_system_api_blob
{
	// Core
	fck_type_system type_system;

	// Children
	fck_identifier_api identifier_api;
	fck_type_api type_api;
	fck_member_api member_api;
	fck_serialise_interface_api serialise_api;
	fck_assembly_api assembly_api;
} fck_type_system_api_blob;

static fck_type_system_api_blob fck_type_system_api_blob_private;

void fck_type_size_of(struct fck_serialiser *s, struct fck_serialiser_params *p, void *self, fckc_size_t c)
{
	SDL_assert(s->vt == &fck_size_of_vt);

	fck_type_system *ts = p->type_system;
	fck_serialise_func *serialise = ts->serialise->get(*p->type);
	if (serialise != NULL)
	{
		serialise(s, p, self, c);
		return;
	}

	fck_type_info *info = ts->type->resolve(*p->type);
	fck_member member = ts->type->members_of(info);
	if (!ts->member->is_null(member))
	{
		// Find member with the FURTHEST stride...
		// TODO: Maybe cache the furthest member?
		fck_member_info *current = ts->member->resolve(member);
		fck_member_info *member_info = ts->member->resolve(member);
		member = ts->member->next_of(member_info);
		while (!ts->member->is_null(member))
		{
			member_info = ts->member->resolve(member);

			fckc_size_t current_stride = ts->member->stride_of(current);
			fckc_size_t member_stride = ts->member->stride_of(member_info);
			member = ts->member->next_of(member_info);
			SDL_assert(member_stride != current_stride);
			if (member_stride > current_stride)
			{
				current = member_info;
			}
		}

		// Do the counting...
		fck_serialiser_type_size_of *size_of = (fck_serialiser_type_size_of *)s;
		fckc_size_t stride_size = ts->member->stride_of(current);
		if (ts->member->is_stretchy(current))
		{
			size_of->size = stride_size + (sizeof(fckc_size_t) * c);
			return;
		}
		fckc_size_t count = ts->member->count_of(current);
		fck_serialiser_params params = *p;
		fck_type type = ts->member->type_of(current);
		params.type = &type;
		fck_type_size_of(s, &params, NULL, count);
		size_of->size = (stride_size + size_of->size) * c;
	}
}

fckc_size_t fck_type_size_of_api(fck_type type)
{
	fck_serialiser_type_size_of size_of = {.vt = &fck_size_of_vt, 0};
	fck_serialiser_params parameters;
	parameters.name = NULL;
	parameters.type = &type;
	parameters.type_system = &fck_type_system_api_blob_private.type_system;

	fck_type_size_of((fck_serialiser *)&size_of, &parameters, NULL, 1);

	return size_of.size;
}

fck_type fck_types_add_api(fck_assembly *assembly, fck_type_desc desc)
{
	return fck_types_add(&assembly->types, desc);
}

fck_type fck_types_find_from_hash_api(fck_assembly *assembly, fckc_u64 hash)
{
	return fck_types_find_from_hash(&assembly->types, hash);
}

fck_type fck_types_find_from_string_api(fck_assembly *assembly, const char *name)
{
	return fck_types_find_from_string(&assembly->types, name);
}

int fck_types_iterate_api(fck_assembly *assembly, fck_type *type)
{
	return fck_types_iterate(&assembly->types, type);
}

fck_member fck_members_add_api(fck_type owner, fck_member_desc desc)
{
	fck_assembly *assembly = fck_types_assembly(owner.types);
	return fck_members_add(&assembly->members, owner, desc);
}

void fck_serialise_interfaces_add_api(fck_serialise_desc desc)
{
	fck_assembly *assembly = fck_types_assembly(desc.type.types);
	fck_serialise_interfaces_add(&assembly->serialisers, desc);
}

fck_serialise_func *fck_serialise_interfaces_get_api(fck_type type)
{
	fck_assembly *assembly = fck_types_assembly(type.types);
	return fck_serialise_interfaces_get(&assembly->serialisers, type);
}

int fck_members_is_stretchy(struct fck_member_info *info)
{
	return info->extra_count == (fckc_size_t)(~0LLU);
}

fck_type_system *fck_load_type_system(struct fck_apis *apis)
{
	// fck_type_system_api_blob *api = (fck_type_system_api_blob *)SDL_malloc(sizeof(*api));
	fck_type_system_api_blob *blob = &fck_type_system_api_blob_private;
	// Make sure nothing breaks this cast - This is how we arrange our memory!
	blob->type_system.identifier = &blob->identifier_api;
	blob->type_system.type = &blob->type_api;
	blob->type_system.member = &blob->member_api;
	blob->type_system.serialise = &blob->serialise_api;
	blob->type_system.assembly = &blob->assembly_api;
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
	ts->type->size_of = fck_type_size_of_api;

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

	ts->assembly->alloc = fck_assembly_alloc;
	ts->assembly->free = fck_assembly_free;

	apis->add(fck_type_system_api_name, ts);
	return ts;
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