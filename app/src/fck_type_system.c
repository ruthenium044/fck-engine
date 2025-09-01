
#include "fck_type_system.h"
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
} fck_type_system_api_blob;

void fck_load_type_system(struct fck_apis *apis)
{
	fck_type_system_api_blob *api = (fck_type_system_api_blob *)SDL_malloc(sizeof(*api));
	api->type_system.identifier = &api->identifier_api;
	api->type_system.type = &api->type_api;
	api->type_system.member = &api->member_api;

	// Make sure nothing breaks this cast - This is how we arrange our memory!
	fck_type_system *ts = (fck_type_system *)api;

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

	// Members public API

	apis->add(fck_type_system_api_name, ts);
}

void fck_unload_type_system(struct fck_apis *apis)
{
	apis->remove(fck_type_system_api_name);
}

fck_type_system *fck_get_type_system(struct fck_apis *apis)
{
	apis->find_from_string(fck_type_system_api_name);
}