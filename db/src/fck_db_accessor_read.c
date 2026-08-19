
#include "fck_db_accessor_read.h"

#include "fck_db.h"
#include "fck_db_core.inl"

#include <fckc_assert.h>
#include <fckc_inttypes.h>

#include <string.h>

static void *fck_db_read_api_untyped(fck_db_accessor accessor, fck_db_type type, const char *property)
{
	fck_db_object *obj = accessor.obj;

	const fckc_size_t result = fck_db_object_find(obj, type, property);
	if (result == 0)
	{
		return NULL;
	}
	fck_db_property_instance *prop = obj->properties + result - 1;
	fckc_u8 *src = (fckc_u8 *)fckc_pointer_add(obj->data, prop->offset);
	return src;
}

static fckc_i32 fck_db_read_api_i32(fck_db_accessor accessor, const char *property)
{
	const void *src = fck_db_read_api_untyped(accessor, fck_db_type_i32, property);
	fck_assert(src);
	fckc_i32 value;
	memcpy(&value, src, sizeof(value));
	return value;
}

static fckc_f32 fck_db_read_api_f32(fck_db_accessor accessor, const char *property)
{
	const void *src = fck_db_read_api_untyped(accessor, fck_db_type_f32, property);
	fck_assert(src);
	fckc_f32 value;
	memcpy(&value, src, sizeof(value));
	return value;
}

static fck_db_asset *fck_db_read_api_asset(fck_db_accessor accessor, const char *property)
{
	const void *src = fck_db_read_api_untyped(accessor, fck_db_type_asset, property);
	fck_assert(src);
	fck_db_asset *value;
	memcpy(&value, src, sizeof(fck_db_asset *));
	return value;
}

static fck_db_id fck_db_read_api_reference(fck_db_accessor accessor, const char *property)
{
	const void *src = fck_db_read_api_untyped(accessor, fck_db_type_reference, property);
	fck_assert(src);
	fck_db_id value;
	memcpy(&value, src, sizeof(value));
	return value;
}

static fck_db_id fck_db_read_api_object(fck_db_accessor accessor, const char *property)
{
	const void *src = fck_db_read_api_untyped(accessor, fck_db_type_object, property);
	fck_assert(src);
	fck_db_id value;
	memcpy(&value, src, sizeof(value));
	return value;
}

static fckc_size_t fck_db_read_api_memory(fck_db_accessor accessor, const char *property, const void **data)
{
	const void *src = fck_db_read_api_untyped(accessor, fck_db_type_memory, property);
	fck_assert(src);
	fck_db_memory *memory = (fck_db_memory *)src;
	*data = (const void *)memory->data;
	return memory->count;
}

static const char *fck_db_read_api_string(fck_db_accessor accessor, const char *property)
{
	const void *src = fck_db_read_api_untyped(accessor, fck_db_type_string, property);
	fck_assert(src);
	// Like in edit, let's pray this works!
	fck_db_memory *memory = (fck_db_memory *)src;
	const char *str = (const char *)memory->data;

	// Some mismatch happened
	fck_assert(strlen(str) == memory->count - 1);
	return str;
}

static const fck_db_id_set *fck_db_read_api_object_set(fck_db_accessor accessor, const char *property)
{
	const void *src = fck_db_read_api_untyped(accessor, fck_db_type_object_set, property);
	fck_assert(src);
	const fck_db_id_set *set = (const fck_db_id_set *)src;
	return set;
}

static fck_db_property fck_db_make_variant(fck_db_type type, const void *data, fckc_size_t offset)
{
	fck_db_property result;
	const fckc_u8 *src = (fckc_u8 *)fckc_pointer_add(data, offset);
	result.type = type;
	switch (type)
	{
	case fck_db_type_none:
		break;
	case fck_db_type_i32:
		memcpy(&result.i32, src, sizeof(result.i32));
		break;
	case fck_db_type_f32:
		memcpy(&result.f32, src, sizeof(result.f32));
		break;
	case fck_db_type_memory: {
		fck_db_memory *memory = (fck_db_memory *)src;
		result.memory.data = (const void *)memory->data;
		result.memory.size = memory->count;
		break;
	}
	case fck_db_type_object:
	case fck_db_type_reference:
		memcpy(&result.object, src, sizeof(result.object));
		break;
	case fck_db_type_asset:
		memcpy(&result.asset, src, sizeof(fck_db_asset *));
		break;
	case fck_db_type_object_set:
		result.set = (fck_db_id_set *)src;
		break;
	case fck_db_type_string: {
		fck_db_memory *memory = (fck_db_memory *)src;
		result.string = (const char *)memory->data;
		fck_assert(strlen(result.string) == memory->count - 1);
		break;
	}
	}
	return result;
}

static fck_db_property fck_db_read_api_variant(fck_db_accessor accessor, const char *property)
{
	fck_db_property result = {fck_db_type_none};

	fck_db_private *db = accessor.db.opaque;
	fck_db_object *obj = accessor.obj;

	const fckc_size_t at = fck_db_object_find(obj, fck_db_type_none, property);
	if (at)
	{
		const fck_db_property_instance *prop = obj->properties + at - 1;
		result = fck_db_make_variant(prop->type, obj->data, prop->offset);
	}
	return result;
}

static fckc_u32 fck_db_object_api_iterate(fck_db_accessor accessor, fckc_u32 *offset, fck_db_named_property *property)
{
	fck_db_object *obj = accessor.obj;

	for (*offset = *offset + 1; *offset <= obj->capacity; *offset = *offset + 1)
	{
		const fck_db_property_instance *prop = obj->properties + (*offset - 1);
		if (fck_db_property_is_used(prop))
		{
			property->value = fck_db_make_variant(prop->type, obj->data, prop->offset);
			property->name = prop->name;
			return *offset;
		}
	}

	*offset = 0;
	return 0;
}

static fckc_u32 fck_db_object_api_version(fck_db_accessor accessor)
{
	fck_db_object *obj = accessor.obj;
	return obj->version;
}

static fck_db_read_api db_read_api = {
	.untyped = fck_db_read_api_untyped,
	.iterate = fck_db_object_api_iterate,
	.version = fck_db_object_api_version,
	.variant = fck_db_read_api_variant,
	.i32 = fck_db_read_api_i32,
	.f32 = fck_db_read_api_f32,
	.asset = fck_db_read_api_asset,
	.object = fck_db_read_api_object,
	.reference = fck_db_read_api_reference,
	.memory = fck_db_read_api_memory,
	.set = fck_db_read_api_object_set,
	.string = fck_db_read_api_string,
};

fck_db_read_api *db_read = &db_read_api;