
#include "fck_db_accessor_edit.h"

#include "fck_db_accessor_ok.h"
#include "fck_db_accessor_read.h"
#include "fck_db_object_properties.h"

#include "fck_db.h"
#include "fck_db_core.inl"

#include <fckc_assert.h>
#include <fckc_inttypes.h>

#include <kll.h>
#include <kll_malloc.h>

#include <string.h>

static fckc_size_t fck_db_edit_next_power_2(fckc_size_t n)
{
	if (n <= 1)
		return 1;

	n--;
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;

#if SIZE_MAX > 0xFFFFFFFFU
	n |= n >> 32;
#endif

	n++;
	return n;
}

static void *fck_db_edit_api_lazy_find(fck_db external, fck_db_object *obj, fck_db_type type, const char *property, fckc_size_t s,
                                       fckc_size_t a)
{
	fck_db_private *db = external.opaque;
	const fckc_size_t result = db_properties->add(db->allocator, obj, type, property);
	fck_assert(result);

	fck_db_property_instance *prop = obj->properties + result - 1;
	if (prop->offset == obj->size)
	{
		prop->offset = fckc_align(obj->at, a);
		obj->at = prop->offset + s;

		if (obj->at >= obj->size)
		{
			const fckc_size_t capacity = fck_db_edit_next_power_2(obj->at);
			void *data = kll_malloc(db->allocator, capacity);
			if (obj->data)
			{
				memcpy(data, obj->data, obj->size);
				kll_free(db->allocator, obj->data);
			}
			obj->size = capacity;
			obj->data = data;
		}
	}
	return (void *)fckc_pointer_add(obj->data, prop->offset);
}

static void fck_db_edit_api_i32(fck_db_accessor accessor, const char *property, fckc_i32 value)
{
	void *dst = fck_db_edit_api_lazy_find(accessor.db, accessor.obj, fck_db_type_i32, property, sizeof(fckc_i32), alignof(fckc_i32));
	memcpy(dst, &value, sizeof(value));
}

static void fck_db_edit_api_f32(fck_db_accessor accessor, const char *property, fckc_f32 value)
{
	void *dst = fck_db_edit_api_lazy_find(accessor.db, accessor.obj, fck_db_type_f32, property, sizeof(fckc_f32), alignof(fckc_f32));
	memcpy(dst, &value, sizeof(value));
}

static void fck_db_edit_api_asset(fck_db_accessor accessor, const char *property, const fck_db_asset *value)
{
	const fckc_size_t s = sizeof(fck_db_asset *);
	const fckc_size_t a = alignof(fck_db_asset *);
	void *dst = fck_db_edit_api_lazy_find(accessor.db, accessor.obj, fck_db_type_asset, property, s, a);
	memcpy(dst, &value, sizeof(fck_db_asset *));
}

static void fck_db_edit_api_reference(fck_db_accessor accessor, const char *property, fck_db_id value)
{
	const fckc_size_t s = sizeof(value);
	const fckc_size_t a = alignof(value);
	void *dst = fck_db_edit_api_lazy_find(accessor.db, accessor.obj, fck_db_type_reference, property, s, a);
	memcpy(dst, &value, sizeof(value));
}

static void fck_db_edit_api_object(fck_db_accessor accessor, const char *property, fck_db_id value)
{
	const fckc_size_t s = sizeof(value);
	const fckc_size_t a = alignof(value);
	void *dst = fck_db_edit_api_lazy_find(accessor.db, accessor.obj, fck_db_type_object, property, s, a);
	memcpy(dst, &value, sizeof(value));
}

static void fck_db_edit_api_any_buffer(fck_db_accessor accessor, const char *property, fck_db_type type, const void *data, fckc_size_t size)
{
	void *current = db_read->untyped(accessor, type, property);
	const fckc_size_t total = offsetof(fck_db_memory, data[size]);
	if (current)
	{
		fck_db_memory *memory = (fck_db_memory *)current;
		if (total <= memory->capacity)
		{
			// Very lovely hot-path!
			memcpy(memory->data, data, size);
			memory->count = size;
			return;
		}
		else
		{
			// Remove old entry, we re-add further down
			const fckc_size_t old_total = offsetof(fck_db_memory, data[memory->capacity]);
			const fckc_size_t result = db_properties->remove(accessor.obj, type, property, old_total);
			fck_assert(result);
		}
	}

	{
		void *dst = fck_db_edit_api_lazy_find(accessor.db, accessor.obj, type, property, total, alignof(fck_db_memory));
		fck_db_memory *memory = (fck_db_memory *)dst;
		memcpy((void *)memory->data, data, size);
		memory->capacity = size;
		memory->count = size;
	}
}

static void fck_db_edit_api_memory(fck_db_accessor accessor, const char *property, const void *data, fckc_size_t size)
{
	fck_db_edit_api_any_buffer(accessor, property, fck_db_type_memory, data, size);
}

static void *fck_db_edit_api_userdata(fck_db_accessor accessor, const char *property, const void *data, fckc_size_t size)
{
	// Should work for now...
	fck_db_edit_api_any_buffer(accessor, property, fck_db_type_memory, data, size);
	void *current = db_read->untyped(accessor, fck_db_type_memory, property);
	fck_db_memory *memory = (fck_db_memory *)current;
	return (void *)memory->data;
}

static void fck_db_edit_api_string(fck_db_accessor accessor, const char *property, const char *value)
{
	fck_assert(value);
	const fckc_size_t len = strlen(value) + 1; // null-terminator
	// Let's pray this works
	fck_db_edit_api_any_buffer(accessor, property, fck_db_type_string, value, len);
}

static void fck_db_edit_api_object_set(fck_db_accessor accessor, const char *property, const fck_db_id_set *set)
{
	// TODO: should this not work similiarly to api_string and api_memory using any_buffer?
	void *current = db_read->untyped(accessor, fck_db_type_object_set, property);

	if (db_ok->set(accessor, property))
	{
		const fck_db_id_set *prev = db_read->set(accessor, property);
		const fckc_size_t total = prev ? offsetof(fck_db_id_set, values[prev->capacity]) : sizeof(*prev);

		// Let's always remove... For now!
		const fckc_size_t result = db_properties->remove(accessor.obj, fck_db_type_object_set, property, total);
		fck_assert(result);
	}

	{
		const fckc_size_t total = set ? offsetof(fck_db_id_set, values[set->capacity]) : sizeof(*set);
		// We copy everything incoming in. Count, capacity, set replication!
		void *dst = fck_db_edit_api_lazy_find(accessor.db, accessor.obj, fck_db_type_object_set, property, total, alignof(fck_db_id_set));
		fck_db_id_set *memory = (fck_db_id_set *)dst;
		if (set)
		{
			memcpy((void *)memory, set, total);
		}
		else
		{
			memset((void *)memory, 0, total);
		}
	}
}

static void fck_db_edit_api_variant(fck_db_accessor accessor, const char *property, const fck_db_property *value)
{
	switch (value->type)
	{
	case fck_db_type_none:
		return;
	case fck_db_type_i32:
		fck_db_edit_api_i32(accessor, property, value->i32);
		break;
	case fck_db_type_f32:
		fck_db_edit_api_f32(accessor, property, value->f32);
		break;
	case fck_db_type_memory:
		fck_db_edit_api_memory(accessor, property, value->memory.data, value->memory.size);
		break;
	case fck_db_type_reference:
		fck_db_edit_api_reference(accessor, property, value->object);
		break;
	case fck_db_type_object:
		fck_db_edit_api_object(accessor, property, value->object);
		break;
	case fck_db_type_asset:
		fck_db_edit_api_asset(accessor, property, value->asset);
		break;
	case fck_db_type_object_set:
		fck_db_edit_api_object_set(accessor, property, value->set);
		break;
	case fck_db_type_string:
		fck_db_edit_api_string(accessor, property, value->string);
		break;
	}
	return;
}

static void fck_db_edit_api_commit(fck_db_accessor accessor, fck_db_undo_scope external)
{
	// TODO: How do we invalidate the accessor???
	fck_db_private *db = accessor.db.opaque;
	fck_db_object *obj = accessor.obj;

	fck_db_object *original = fck_db_resolve_object(db->page_table, accessor.original);
	fck_db_object *inflight = fck_db_resolve_object(db->page_table, accessor.inflight);

	// Swap out the objects - This needs to be done way more elegantly in the future
	// But for now I need to port assets away
	const fck_db_object temp = *original;
	*original = *inflight;
	*inflight = temp;

	fck_db_undo_scope_private *undo = external.opaque;
	if (undo)
	{
		// Too lazy to fully implement it for now!
		fck_db_undo_unit *unit = undo->units + undo->cursor;
		unit->target = accessor.original;
		unit->copy = accessor.inflight;

		const fckc_u32 next_cursor = (undo->cursor + 1) % fck_arraysize(undo->units);
		const int push_back = next_cursor == undo->back;
		if (push_back)
		{
			// Since we overwrite the back, we push the back one forward!
			undo->back = (next_cursor + 1) % fck_arraysize(undo->units);
		}
		// Front is always equal to cursor when committing
		undo->cursor = next_cursor;
		undo->front = next_cursor;
	}
}

static fck_db_edit_api db_edit_api = {
	.variant = fck_db_edit_api_variant,
	.i32 = fck_db_edit_api_i32,
	.f32 = fck_db_edit_api_f32,
	.asset = fck_db_edit_api_asset,
	.memory = fck_db_edit_api_memory,
	.object = fck_db_edit_api_object,
	.reference = fck_db_edit_api_reference,
	.set = fck_db_edit_api_object_set,
	.commit = fck_db_edit_api_commit,
	.string = fck_db_edit_api_string,
	.userdata = fck_db_edit_api_userdata,
};

fck_db_edit_api *db_edit = &db_edit_api;