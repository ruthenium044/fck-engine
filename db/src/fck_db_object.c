
#include "fck_db_object.h"
#include "fck_db.h"
#include "fck_db_core.inl"

#include <fck_serialiser.h>
#include <kll.h>
#include <kll_malloc.h>

#include <fck_os.h>
#include <fckc_inttypes.h>

#include "fck_db_accessor_edit.h"
#include "fck_db_accessor_ok.h"
#include "fck_db_accessor_read.h"
#include "fck_db_id_set.h"

#include "fckc_assert.h"

#include <stdio.h>
#include <string.h>

#define fck_db_private_serialiser_seperator "_"
#define fck_db_private_uuid fck_db_private_serialiser_seperator "uuid"
#define fck_db_private_name fck_db_private_serialiser_seperator "name"
// It actually does not make any fucking sense to safe this one to disk lol
// It is like storing a fucking pointer
// #define fck_db_private_id fck_db_private_serialiser_seperator "id"
#define fck_db_private_signature fck_db_private_serialiser_seperator "signature"

static fck_db_id fck_db_id_create_and_next(fck_db_private *db)
{
	fckc_u8 *e = db->id_factory;
	const fckc_size_t iterations = fck_arraysize(db->id_factory);

	for (fckc_size_t i = iterations; i > 0; i--)
	{
		const fckc_size_t index = i - 1;
		e[index] = e[index] + 1;

		if (e[index] == 0xFF)
		{
			e[index] = 0;
			continue;
		}
		break;
	}

	os->io->log("Id: %d \t- %d \t- %d \t- %d", to_int(e[0]), to_int(e[1]), to_int(e[2]), to_int(e[3]));
	fck_db_id id = fck_db_id_make(e[0], e[1], e[2], e[3], fck_db_type_object);
	return id;
}

static fck_db_object fck_db_object_clone(kll_allocator *allocator, const fck_db_object *obj)
{
	const fckc_size_t total = obj->capacity * sizeof(*obj->properties);
	fck_db_property_instance *props = (fck_db_property_instance *)kll_malloc(allocator, total);
	memcpy(props, obj->properties, total);

	void *data = kll_malloc(allocator, obj->at);
	memcpy(data, obj->data, obj->at);

	fck_db_object result = {0};
	result.version = obj->version + 1 % to_u32(0xFFFFFFFF);

	result.properties = props;
	result.count = obj->count;
	result.capacity = obj->capacity;

	result.data = data;
	result.at = obj->at;
	result.size = obj->size;

	return result;
}

static fck_db_id fck_db_object_api_create(fck_db external, const char *name)
{
	fck_db_private *db = external.opaque;
	const fck_db_id id = fck_db_id_create_and_next(db);
	// We may have to try again if this one already exists... No check for that yet... Oh boy
	fck_db_object *entry = fck_db_add_object(db->page_table, id);
	fck_assert(entry);
	entry->name = name;
	return id;
}

static fck_db_accessor fck_db_object_api_edit(fck_db external, fck_db_id id)
{
	fck_db_private *db = external.opaque;
	fck_db_object *entry = fck_db_resolve_object(db->page_table, id);
	fck_assert(entry);

	// fck_db_object_api_create(id, entry->name);
	const fck_db_id temp = fck_db_object_api_create(external, entry->name); // fck_db_id_advance(id);

	fck_db_object *copy = fck_db_resolve_object(db->page_table, temp);
	*copy = fck_db_object_clone(db->allocator, entry);

	fck_db_accessor accessor = {
		.original = id,
		.inflight = temp,
		.edit = db_edit,
		.read = db_read,
		.ok = db_ok,
		.db = external,
		.obj = copy,
	};

	return accessor;
}

static fck_db_accessor fck_db_object_api_read(fck_db external, fck_db_id id)
{
	fck_db_private *db = external.opaque;
	fck_db_object *entry = fck_db_resolve_object(db->page_table, id);
	fck_assert(entry);

	fck_db_accessor accessor = {
		.original = id,
		.inflight = id,
		.db = external,
		.edit = NULL,
		.ok = db_ok,
		.read = db_read,
		.obj = entry,
	};

	return accessor;
}

static void fck_db_object_api_destroy(fck_db external, fck_db_id id)
{
	fck_db_private *db = external.opaque;
	fck_db_remove_object(db->page_table, id);
}

static fckc_u32 fck_xorshift32(fckc_u32 *state)
{
	fckc_u32 x = *state;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	*state = x;
	fck_assert(x);
	return x;
}

static fck_db_uuid fck_generate_uuid(void)
{
	fck_db_uuid uuid;

	fckc_u32 rng_state = (fckc_u32)os->chrono->now() ^ (fckc_u32)(fckc_uintptr)&uuid;
	const fckc_u32 part1 = fck_xorshift32(&rng_state);
	const fckc_u32 part2 = fck_xorshift32(&rng_state);
	uuid.values[0] = (fckc_i32)part1;
	uuid.values[1] = (fckc_i32)part2;

	return uuid;
}

static void fck_db_guid_generate(fckc_u64 *time, fckc_u32 *rand, fckc_u32 *signal)
{
	fckc_u32 rng_state = (fckc_u32)os->chrono->now() ^ (fckc_u32)(fckc_uintptr)time;
	*time = (fckc_u64)os->chrono->now();
	*rand = ((fckc_u64)fck_xorshift32(&rng_state) << 32) | (fckc_u64)fck_xorshift32(&rng_state);
	*signal = fck_xorshift32(&rng_state);
}

static fck_db_uuid fck_db_object_u64_to_uuid(fckc_u64 val)
{
	fck_db_uuid uuid;

	uuid.values[0] = (fckc_u32)(val & fck_bitmask(32));
	uuid.values[1] = (fckc_u32)((val >> 32) & fck_bitmask(32));
	return uuid;
}

static fckc_u64 fck_db_object_uuid_to_u64(fck_db_uuid uuid)
{
	return ((fckc_u64)uuid.values[1] << 32) | ((fckc_u64)uuid.values[0] & 0xFFFFFFFFULL);
}

static void fck_db_object_uuid_mapping(fck_db external, fck_db_id id, fck_db_object *obj)
{
	fckc_u64 time;
	fckc_u32 rand;
	fckc_u32 signal;
	fck_db_guid_generate(&time, &rand, &signal);

	fck_db_private *db = external.opaque;
	//...
}

static fck_db_uuid fck_db_object_uuid_maybe_generate_and_get(fck_db external, fck_db_id id)
{
	fck_db_private *db = external.opaque;
	fck_db_object *entry = fck_db_resolve_object(db->page_table, id);
	if (entry->uuid.values[0] == 0 && entry->uuid.values[1] == 0)
	{
		entry->uuid = fck_generate_uuid();
		fck_db_object_uuid_mapping(external, id, entry);
	}
	return entry->uuid;
}

static void fck_db_object_save_recursively(kll_arena *arena, fck_serialiser *serialiser, fck_db external, const char *scope, fck_db_id id);

static void fck_db_object_serialise(kll_arena *arena, fck_serialiser *serialiser, fck_db external, fck_db_id id)
{
	fck_db_private *db = external.opaque;
	fck_db_object *entry = fck_db_resolve_object(db->page_table, id);
	if (entry == NULL)
	{
		return;
	}
	const fck_db_uuid uuid = fck_db_object_uuid_maybe_generate_and_get(external, id);
	fckc_u64 uuid_u64 = fck_db_object_uuid_to_u64(entry->uuid);

	serialiser->string(serialiser, fck_db_private_name, (void **)&entry->name, 1);
	serialiser->u64(serialiser, fck_db_private_uuid, &uuid_u64, 1);

	const fck_db_accessor reader = fck_db_object_api_read(external, id);

	const char **names = (const char **)kll_malloc(arena, entry->count * sizeof(const char *));
	fckc_u64 *types = (fckc_u64 *)kll_malloc(arena, entry->count * sizeof(fckc_u64));
	fckc_size_t index = 0;

	fckc_u32 it = 0;
	fck_db_named_property named_property = {0};
	while (reader.read->iterate(reader, &it, &named_property))
	{
		const fckc_u64 type = to_u64(named_property.value.type);
		names[index] = named_property.name;
		types[index] = type;
		index = index + 1;
	}

	serialiser->push(serialiser, fck_db_private_signature);
	serialiser->string(serialiser, "_names", (void **)names, index);
	serialiser->u64(serialiser, "_types", types, index);
	serialiser->pop(serialiser);

	it = 0;
	while (reader.read->iterate(reader, &it, &named_property))
	{
		const char *key = named_property.name;
		char buffer[128];
		fck_db_property *property = &named_property.value;
		switch (property->type)
		{
		case fck_db_type_none:
			break;
		case fck_db_type_i32:
			serialiser->i32(serialiser, key, &property->i32, 1);
			break;
		case fck_db_type_f32:
			// Easy
			serialiser->f32(serialiser, key, &property->f32, 1);
			break;
		case fck_db_type_memory:
			// Binary array?
			property->memory;
			break;
		case fck_db_type_object:
			fck_db_object_save_recursively(arena, serialiser, external, key, property->object);
			break;
		case fck_db_type_reference: {
			const fck_db_uuid uuid = fck_db_object_uuid_maybe_generate_and_get(external, property->object);
			fckc_u64 value = fck_db_object_uuid_to_u64(uuid);
			serialiser->u64(serialiser, key, &value, 1);
			break;
		}
		case fck_db_type_asset:
			// TODO: Making asset a referencable from disk format
			property->asset;
			break;
		case fck_db_type_object_set: {
			fck_db_id *id = NULL;
			serialiser->push(serialiser, key);
			// Add count
			// Make a name key[Index] for the children
			fckc_u32 index = 0;
			while (db_id_set->iterate(property->set, &id))
			{
				snprintf(buffer, sizeof(buffer), "[%u]", index);
				fck_db_object_save_recursively(arena, serialiser, external, buffer, *id);
				index++;
			}
			serialiser->pop(serialiser);
			break;
		}
		}
	}
}

static void fck_db_object_save_recursively(kll_arena *arena, fck_serialiser *serialiser, fck_db external, const char *scope, fck_db_id id)
{
	serialiser->push(serialiser, scope);
	fck_db_object_serialise(arena, serialiser, external, id);
	serialiser->pop(serialiser);
}

static void fck_db_object_api_save(fck_serialiser *serialiser, fck_db external, fck_db_id id)
{
	kll_arena *arena = kll->arena->create(kll->system, 4096);
	fck_db_object_serialise(arena, serialiser, external, id);
	kll->arena->destroy(arena);
}

static fck_serialiser_element *fck_db_serialiser_query(fck_serialiser *serialiser, const char *path, const char *variable)
{
	char buffer[1024];
	int result;
	if (path == NULL || path[0] == '\0')
	{
		result = snprintf(buffer, sizeof(buffer), "/%s", variable);
	}
	else
	{
		result = snprintf(buffer, sizeof(buffer), "/%s/%s", path, variable);
	}
	fck_assert(result >= 0 && result < sizeof(buffer));
	fck_serialiser_element *element = serialiser->query(serialiser, buffer);
	return element;
}

static void fck_db_deserialise_skip_scope(fck_serialiser_iterator *it, fck_serialiser_element *element)
{
	int indent = 0;
	while (it->next(it, element))
	{
		if (element->type == fck_serialiser_push)
		{
			indent = indent + 1;
			continue;
		}

		if (element->type == fck_serialiser_pop)
		{
			if (indent == 0)
			{
				break;
			}
			indent = indent - 1;
		}
	}
}

static void fck_db_object_uuid_set(fck_db external, fck_db_id id, fck_db_uuid uuid)
{
	// TODO: UUID to ID mapping? we know id to uuid since we store it in the object
	// But maybe back and forth mapping would be better...
	fck_db_private *db = external.opaque;
	fck_db_object *entry = fck_db_resolve_object(db->page_table, id);
	if (entry->uuid.values[0] == 0 && entry->uuid.values[1] == 0)
	{
		entry->uuid = uuid;
		fck_db_object_uuid_mapping(external, id, entry);
	}
	else
	{
		fck_assert(entry->uuid.values[0] == uuid.values[0] && entry->uuid.values[1] == uuid.values[1]);
	}
}

static fck_db_id fck_db_deserialise_object(fck_db external, fck_serialiser *s, fck_serialiser_iterator *it, fck_serialiser_element *element)
{
	fck_assert(element->type == fck_serialiser_push);

	fck_serialiser_element *object_name = fck_db_serialiser_query(s, element->name, fck_db_private_name);
	fck_assert(object_name->count == 1);
	fck_assert(object_name->type == fck_serialiser_string);

	fck_serialiser_element *object_uuid = fck_db_serialiser_query(s, element->name, fck_db_private_uuid);
	fck_assert(object_uuid->count == 1);
	fck_assert(object_uuid->type == fck_serialiser_u64);

	const fck_db_id temp = fck_db_object_api_create(external, object_name->values->as_string);
	fck_db_object_uuid_set(external, temp, fck_db_object_u64_to_uuid(object_uuid->values->as_u64));

	fck_serialiser_element *signature_names = fck_db_serialiser_query(s, element->name, fck_db_private_signature "/_names");
	fck_serialiser_element *signature_values = fck_db_serialiser_query(s, element->name, fck_db_private_signature "/_types");

	fck_assert(signature_names->count == signature_values->count);

	{
		// Reserve relevant data
		const fck_db_accessor editor = fck_db_object_api_edit(external, temp);
		for (fckc_size_t index = 0; index < signature_names->count; index++)
		{
			const char *name = signature_names->values[index].as_string;
			const fckc_u64 type = signature_values->values[index].as_u64;
			const fck_db_property property = {.type = (fck_db_type)type};
			editor.edit->variant(editor, name, &property);
		}
		editor.edit->commit(editor, fck_db_no_undo);
	}
	{
		const fck_db_accessor reader = fck_db_object_api_read(external, temp);
		const fck_db_accessor editor = fck_db_object_api_edit(external, temp);

		while (it->next(it, element))
		{
			if (element->type == fck_serialiser_pop)
			{
				break;
			}

			// Due to paths having '/', we need to account for it
			// TODO: Fix this - Stupid hiccup with paths...
			const char *name = element->name; //+ prefix_len;
			if (strcmp(name, fck_db_private_name) == 0)
			{
				continue;
			}
			if (strcmp(name, fck_db_private_uuid) == 0)
			{
				continue;
			}
			if (strcmp(name, fck_db_private_signature) == 0)
			{
				fck_db_deserialise_skip_scope(it, element);
				continue;
			}

			// Not so simple cases
			const fck_db_property current = reader.read->variant(reader, name);
			switch (current.type)
			{
			case fck_db_type_none:
				// Uuuummmm... Might be string
				// We should handle this
				fck_assert(0 && "Handle this");
				continue;
			case fck_db_type_reference: {
				// fck_db_accessor reader = fck_db_object_api_read(external, current.object);
				// reader.read->reference(reader, "uuid");
			}
				continue;
			case fck_db_type_memory:
			case fck_db_type_asset:
			case fck_db_type_object:
				editor.edit->object(editor, name, fck_db_deserialise_object(external, s, it, element));
				continue;
			case fck_db_type_object_set:
				fck_db_deserialise_skip_scope(it, element);
				continue;

			case fck_db_type_i32:
				editor.edit->i32(editor, name, to_i32(element->values->as_i32));
				break;
			case fck_db_type_f32:
				editor.edit->f32(editor, name, to_f32(element->values->as_f64));
				break;
			}
		}

		editor.edit->commit(editor, fck_db_no_undo);
	}
	return temp;
}

static void fck_db_print(fck_db external, fck_db_id id)
{
	const fck_db_accessor reader = fck_db_object_api_read(external, id);
	fckc_u32 it = 0;
	fck_db_named_property property = {0};
	while (reader.read->iterate(reader, &it, &property))
	{
		switch (property.value.type)
		{
		case fck_db_type_f32:
			os->io->log("Name: %s - %f", property.name, property.value.f32);
			break;
		case fck_db_type_none:
			break;
		case fck_db_type_i32:
			os->io->log("Name: %s - %d", property.name, property.value.i32);
			break;
		case fck_db_type_memory:
		case fck_db_type_object:
			fck_db_print(external, property.value.object);
			break;
		case fck_db_type_asset:
		case fck_db_type_reference:
			break;
		}
	}
}

static void fck_db_deserialise(fck_db external, fck_db_type type, fck_serialiser *serialiser, fck_serialiser_iterator *it)
{
	fck_serialiser_element element;
	it->next(it, &element);
	if (element.type != fck_serialiser_push)
	{
		os->io->log("Weird that we ended up here, we should be in an object scope");
		return;
	}

	const fck_db_id temp = fck_db_deserialise_object(external, serialiser, it, &element);
	fck_db_print(external, temp);
}

static void fck_db_object_api_load(fck_serialiser *serialiser, fck_db external)
{
	fck_serialiser_iterator *it = serialiser->iterator(serialiser);
	fck_db_deserialise(external, fck_db_type_object, serialiser, it);
}

static fck_db_object_api db_object_api = {
	.create = fck_db_object_api_create,
	.destroy = fck_db_object_api_destroy,
	.read = fck_db_object_api_read,
	.edit = fck_db_object_api_edit,
	.save = fck_db_object_api_save,
	.load = fck_db_object_api_load,
};

fck_db_object_api *db_object = &db_object_api;