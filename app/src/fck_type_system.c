
#include "fck_type_system.h"
#include "fck_type_system.inl"
#include "SDL3/SDL_stdinc.h"
#include "fck_apis.h"

static const char *fck_type_system_api_name = "FCK_TYPE_SYSTEM";

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
static struct fck_serialise_interfaces* get_serialisers(void)
{
	return fck_type_system_api_blob_private.serialisers;
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

fck_member fck_members_add_api(fck_member_desc desc)
{
	return fck_members_add(fck_type_system_api_blob_private.members, desc);
}

void fck_serialise_interfaces_add_api(fck_serialise_desc desc)
{
	fck_serialise_interfaces_add(fck_type_system_api_blob_private.serialisers, desc);
}

fck_serialise_func *fck_serialise_interfaces_get_api(fck_type type)
{
	return fck_serialise_interfaces_get(fck_type_system_api_blob_private.serialisers, type);
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

	ts->get_identifiers = get_identifiers;
	ts->get_types = get_types;
	ts->get_members = get_members;
	ts->get_serialisers = get_serialisers;

	// Types public API
	ts->type->null = fck_type_null;
	ts->type->is_null = fck_type_is_null;
	ts->type->is_same = fck_type_is_same;
	ts->type->is = fck_type_is;
	ts->type->resolve = fck_type_resolve;

	ts->type->identify = fck_type_info_identify;
	ts->type->members_of = fck_type_info_first_member;

	ts->type->add = fck_types_add_api;
	ts->type->find_from_hash = fck_types_find_from_hash_api;
	ts->type->find_from_string = fck_types_find_from_string_api;

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
	ts->member->count = fck_member_info_count;

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
	return apis->find_from_string(fck_type_system_api_name);
}