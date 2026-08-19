#include "fck_db_accessor_ok.h"

#include "fck_db.h"
#include "fck_db_accessor_read.h"
#include "fck_db_core.inl"

static int fck_db_ok_api_variant(fck_db_accessor accessor, const char *property)
{
	fck_db_object *obj = accessor.obj;
	const fckc_size_t at = fck_db_object_find(obj, fck_db_type_none, property);
	return at != 0;
}

static int fck_db_ok_api_i32(fck_db_accessor accessor, const char *property)
{
	const void *src = db_read->untyped(accessor, fck_db_type_i32, property);
	return src != NULL;
}

static int fck_db_ok_api_f32(fck_db_accessor accessor, const char *property)
{
	const void *src = db_read->untyped(accessor, fck_db_type_f32, property);
	return src != NULL;
}

static int fck_db_ok_api_asset(fck_db_accessor accessor, const char *property)
{
	const void *src = db_read->untyped(accessor, fck_db_type_asset, property);
	return src != NULL;
}

static int fck_db_ok_api_reference(fck_db_accessor accessor, const char *property)
{
	const void *src = db_read->untyped(accessor, fck_db_type_reference, property);
	return src != NULL;
}

static int fck_db_ok_api_object(fck_db_accessor accessor, const char *property)
{
	const void *src = db_read->untyped(accessor, fck_db_type_object, property);
	return src != NULL;
}

static int fck_db_ok_api_memory(fck_db_accessor accessor, const char *property)
{
	const void *src = db_read->untyped(accessor, fck_db_type_memory, property);
	return src != NULL;
}

static int fck_db_ok_api_string(fck_db_accessor accessor, const char *property)
{
	const void *src = db_read->untyped(accessor, fck_db_type_string, property);
	return src != NULL;
}

static int fck_db_ok_api_object_set(fck_db_accessor accessor, const char *property)
{
	const void *src = db_read->untyped(accessor, fck_db_type_object_set, property);
	return src != NULL;
}

static fck_db_ok_api db_ok_api = {
	.variant = fck_db_ok_api_variant,
	.i32 = fck_db_ok_api_i32,
	.f32 = fck_db_ok_api_f32,
	.asset = fck_db_ok_api_asset,
	.memory = fck_db_ok_api_memory,
	.object = fck_db_ok_api_object,
	.reference = fck_db_ok_api_reference,
	.set = fck_db_ok_api_object_set,
	.string = fck_db_ok_api_string,
};

fck_db_ok_api *db_ok = &db_ok_api;
