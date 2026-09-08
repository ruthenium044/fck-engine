
#include "fck_db_object_properties.h"

#include <kll.h>
#include <kll_malloc.h>

#include <fckc_assert.h>

#include <fck_hash.h>

#include "fck_db_core.inl"

#include <string.h>

// Properties
static void fck_db_object_free(kll_allocator *allocator, fck_db_object *obj)
{
	if (obj->properties)
	{
		kll_free(allocator, obj->properties);
	}
	obj->properties = NULL;
	if (obj->data)
	{
		kll_free(allocator, obj->data);
	}
	obj->data = NULL;

	obj->at = obj->size = 0;
	obj->version = 0xFFFFFFFF;
	obj->count = obj->capacity = 0;
}
// Properties
static int fck_db_property_is_used(const fck_db_property_instance *property)
{
	return property->name && property->type != fck_db_type_none;
}
// Properties
static fckc_size_t fck_db_object_find(fck_db_object *instance, fck_db_type type, const char *name)
{
	const fckc_size_t len = strlen(name);
	const fckc_u64 hash = to_u64(fck_hash(name, len));

	const fckc_size_t capacity = instance->capacity;

	for (fckc_u64 index = 0; index < capacity; index++)
	{
		const fckc_u64 slot = (hash + index) % capacity;
		const fck_db_property_instance *property = instance->properties + slot;
		if (property->name == NULL)
		{
			return to_size_t(0);
		}
		// TODO: Having this weird check to begin with will 100% lash back on me.
		if ((type == fck_db_type_none || type == property->type) && strcmp(property->name, name) == 0)
		{
			return slot + 1;
		}
	}
	return to_size_t(0);
}
// Properties
static fckc_size_t fck_db_object_add_ng(fck_db_object *instance, fck_db_type type, const char *name, fckc_size_t offset)
{
	const fckc_size_t len = strlen(name);
	const fckc_u64 hash = to_u64(fck_hash(name, len));
	const fckc_size_t capacity = instance->capacity;

	for (fckc_u64 index = 0; index < capacity; index++)
	{
		const fckc_u64 slot = (hash + index) % capacity;
		fck_db_property_instance *property = instance->properties + slot;
		if (property->name == NULL || property->type == fck_db_type_none)
		{
			property->type = type;
			property->name = name;
			property->offset = offset;
			return slot + 1;
		}
	}
	return 0;
}
// Properties
static void fck_db_object_adjust_offsets(fck_db_object *instance, fckc_size_t offset, fckc_size_t size)
{
	const fckc_size_t capacity = instance->capacity;
	for (fckc_u64 index = 0; index < capacity; index++)
	{
		fck_db_property_instance *property = instance->properties + index;
		if (property->name == NULL || property->type == fck_db_type_none)
		{
			continue;
		}
		// This will break completely since we completely IGNORE alignment
		// We could fix that easily by having a union and work with that
		// Most of our types (and it is internally controlled) work on 8 byte alignment!
		if (property->offset > offset)
		{
			property->offset = property->offset - size;
		}
	}
	fck_assert(offset + size <= instance->at);
	const void *src = fckc_pointer_add(instance->data, offset + size);
	void *dst = fckc_pointer_add(instance->data, offset);
	const fckc_size_t movsize = instance->at - (offset + size);
	memmove(dst, src, movsize);
	instance->at = instance->at - size;
}
// Properties
static fckc_size_t fck_db_object_remove(fck_db_object *instance, fck_db_type type, const char *name, fckc_size_t size)
{
	const fckc_size_t len = strlen(name);
	const fckc_u64 hash = to_u64(fck_hash(name, len));
	const fckc_size_t capacity = instance->capacity;

	for (fckc_u64 index = 0; index < capacity; index++)
	{
		const fckc_u64 slot = (hash + index) % capacity;
		fck_db_property_instance *property = instance->properties + slot;
		if (property->name == NULL)
		{
			return to_size_t(0);
		}
		if (type == property->type && strcmp(property->name, name) == 0)
		{
			fck_db_object_adjust_offsets(instance, property->offset, size);
			property->type = fck_db_type_none;
			property->offset = 0;
			instance->count = instance->count - 1;
			return slot;
		}
	}
	return 0;
}
// Properties
static fckc_size_t fck_db_object_add(kll_allocator *allocator, fck_db_object *instance, fck_db_type type, const char *name)
{
	const fckc_size_t result = fck_db_object_find(instance, type, name);
	if (result)
	{
		return result;
	}

	if (instance->count >= instance->capacity / 2)
	{
		const fckc_u32 capacity = instance->capacity ? instance->capacity * 2 : 8;
		const fckc_size_t total = capacity * sizeof(*instance->properties);
		fck_db_property_instance *props = (fck_db_property_instance *)kll_malloc(allocator, total);
		memset(props, 0, total);

		const fckc_u32 old_capacity = instance->capacity;
		fck_db_property_instance *previous = instance->properties;

		instance->properties = props;
		instance->capacity = capacity;
		instance->count = 0;
		if (previous)
		{

			for (fckc_u32 index = 0; index < old_capacity; index++)
			{
				fck_db_property_instance *property = previous + index;
				if (property->type != fck_db_type_none)
				{
					const fckc_size_t result = fck_db_object_add_ng(instance, property->type, property->name, property->offset);
					fck_assert(result);
					if (result)
					{
						instance->count = instance->count + 1;
					}
				}
			}
			kll_free(allocator, previous);
		}
	}

	{
		const fckc_size_t result = fck_db_object_add_ng(instance, type, name, instance->size);
		if (result)
		{
			instance->count = instance->count + 1;
		}
		return result;
	}
}

static fck_db_object_properties_api db_object_properties_api = {
	.free = fck_db_object_free,
	.add = fck_db_object_add,
	.remove = fck_db_object_remove,
	.adjust_offsets = fck_db_object_adjust_offsets,
	.find = fck_db_object_find,
	.is_property_used = fck_db_property_is_used,
};

fck_db_object_properties_api *db_properties = &db_object_properties_api;
