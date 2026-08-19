
#include "fck_db.h"

#include <fckc_inttypes.h>

#include <kll.h>
#include <kll_malloc.h>

#include <fck_apis.h>
#include <fck_hash.h>
#include <fck_os.h>
#include <fckc_apidef.h>
#include <fckc_assert.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <fckc_atomic.h>

#include <fck_serialiser.h>

#include "fck_db_core.inl"
#include "fck_db_object_page_table.h"

#include "fck_db_accessor_edit.h"
#include "fck_db_accessor_ok.h"
#include "fck_db_accessor_read.h"

#include "fck_db_id_set.h"

#define fck_db_id_tombstone_type to_u16(65535)

#define fck_db_private_serialiser_seperator "_"
#define fck_db_private_uuid fck_db_private_serialiser_seperator "uuid"
#define fck_db_private_name fck_db_private_serialiser_seperator "name"
// It actually does not make any fucking sense to safe this one to disk lol
// It is like storing a fucking pointer
// #define fck_db_private_id fck_db_private_serialiser_seperator "id"
#define fck_db_private_signature fck_db_private_serialiser_seperator "signature"

static fck_api_registry *apis = NULL;
static fck_db_api *db_api;

typedef struct fck_db_guid
{
	// Will totally not get utilised!
	fckc_u64 time;
	fckc_u32 rand;
	// Can never be 0, maybe we hardcode 1 to in progress...
	fckc_atomic_u32 signal;
} fck_db_guid;

typedef struct fck_db_guid_key
{
	fckc_u64 value : 62;
	fckc_u64 ok : 1;
	fckc_u64 tomb : 1;
} fck_db_guid_key;

typedef struct fck_db_guid_key_value
{
	fck_db_guid_key state;
	fck_db_guid guid;
	fck_db_id id;
} fck_db_guid_key_value;

typedef struct fck_db_guid_id_map
{
	fck_db_guid_key_value *values;
	fckc_size_t count;
	fckc_size_t capacity;
} fck_db_guid_id_map;

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

int fck_db_property_is_used(const fck_db_property_instance *property)
{
	return property->name && property->type != fck_db_type_none;
}

fckc_size_t fck_db_object_find(fck_db_object *instance, fck_db_type type, const char *name)
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

fckc_size_t fck_db_object_remove(fck_db_object *instance, fck_db_type type, const char *name, fckc_size_t size)
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

fckc_size_t fck_db_object_add(kll_allocator *allocator, fck_db_object *instance, fck_db_type type, const char *name)
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

static inline fckc_u64 fck_db_random_next_rotl(const fckc_u64 x, int k)
{
	return (x << k) | (x >> (64 - k));
}

static inline fckc_u64 fck_db_random_splitmix64(fckc_u64 *state)
{
	*state += 0x9e3779b97f4a7c15; // Golden ratio increment
	fckc_u64 z = *state;
	z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
	z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
	return z ^ (z >> 31);
}

static fck_db_id fck_db_id_make(fckc_u8 e0, fckc_u8 e1, fckc_u8 e2, fckc_u8 e3, fck_db_type type)
{
	fck_db_id id = {0};
	id.index = ((fckc_u32)e0 << 24) | ((fckc_u32)e1 << 16) | ((fckc_u32)e2 << 8) | (fckc_u32)e3;
	id.index = id.index ^ 0xFFFFFFFFU;
	id.type = type;
	return id;
}

static int fck_multidir_id_is_ok(fckc_u8 e0, fckc_u8 e1, fckc_u8 e2, fckc_u8 e3)
{
	if (e0 == 0xFF)
	{
		return 0;
	}
	if (e1 == 0xFF)
	{
		return 0;
	}
	if (e2 == 0xFF)
	{
		return 0;
	}
	if (e3 == 0xFF)
	{
		return 0;
	}
	return 1;
}

static const char *fck_db_api_file_extension(const char *path)
{
	const char *dot = strrchr(path, '.');
	if (!dot || dot == path)
	{
		return "";
	}

	return dot + 1;
}

static fck_db_loader_interface *fck_db_ext_map_find(fck_db_ext_map *map, const char *ext)
{
	if (map->capacity == 0)
	{
		return NULL;
	}

	const fckc_size_t len = strlen(ext);
	const fck_hash_int hash = fck_hash(ext, len);
	const fckc_size_t capacity = map->capacity;

	fckc_size_t slot = hash % capacity;
	for (fckc_size_t index = 0; index < map->capacity; index++)
	{
		fck_db_ext_map_entry *entry = map->entries + slot;
		if (entry->extension == NULL)
		{
			return NULL;
		}

		if (strcmp(ext, entry->extension) == 0)
		{
			return entry->loader;
		}

		slot = (slot + 1) % capacity;
	}
	return NULL;
}

static int fck_db_ext_map_add(fck_db_ext_map *map, const char *ext, fck_db_loader_interface *loader)
{
	if (map->capacity == 0)
	{
		return 0;
	}

	const fckc_size_t len = strlen(ext);
	const fck_hash_int hash = fck_hash(ext, len);
	const fckc_size_t capacity = map->capacity;

	fckc_size_t slot = hash % capacity;
	for (fckc_size_t index = 0; index < map->capacity; index++)
	{
		fck_db_ext_map_entry *entry = map->entries + slot;
		if (entry->extension == NULL)
		{
			entry->extension = ext;
			entry->loader = loader;
			return 1;
		}

		if (strcmp(ext, entry->extension) == 0)
		{
			if (entry->loader == loader)
			{
				return 1;
			}
			return 0;
		}

		slot = (slot + 1) % capacity;
	}
	return 0;
}

static fck_db_ext_map fck_db_ext_map_create(kll_allocator *allocator)
{
	fck_db_ext_map map = {0};

	void **asset_loaders;
	const fckc_size_t asset_loaders_count = apis->implementations(fck_db_loader_interface_name, &asset_loaders);

	map.capacity = 0;
	for (fckc_size_t index = 0; index < asset_loaders_count; index++)
	{
		fck_db_loader_interface *loader = (fck_db_loader_interface *)asset_loaders[index];
		const char **extensions;
		const fckc_size_t count = loader->supports(&extensions);
		map.capacity = map.capacity + count;
	}

	map.capacity = map.capacity * 2;
	const fckc_size_t total = map.capacity * sizeof(*map.entries);
	map.entries = (fck_db_ext_map_entry *)kll_malloc(allocator, total);
	memset(map.entries, 0, total);

	for (fckc_size_t index = 0; index < asset_loaders_count; index++)
	{
		fck_db_loader_interface *loader = (fck_db_loader_interface *)asset_loaders[index];
		const char **extensions;
		const fckc_size_t count = loader->supports(&extensions);
		for (fckc_size_t ext_index = 0; ext_index < count; ext_index++)
		{
			const char *ext = extensions[ext_index];
			const int result = fck_db_ext_map_add(&map, ext, loader);
			fck_assert(result && "Trying to apply duplicated loader");
		}
	}

	return map;
}

static void fck_db_ext_map_destroy(kll_allocator *allocator, fck_db_ext_map *map)
{
	kll_free(allocator, map->entries);
	map->capacity = 0;
}

static fck_db_id fck_db_id_from_path(const char *path, const char *type_name)
{
	fckc_u32 hash = 2166136261U;

	while (*path)
	{
		char c = *path++;
		if (c == '\\')
			c = '/';
		if (c >= 'A' && c <= 'Z')
			c += 32;

		hash ^= (fckc_u8)c;
		hash *= 16777619U;
	}

	if (type_name)
	{
		while (*type_name)
		{
			char c = *type_name++;
			if (c >= 'A' && c <= 'Z')
				c += 32;

			hash ^= (fckc_u8)c;
			hash *= 16777619U;
		}
	}
	fckc_u8 e[4];
	for (int i = 0; i < 4; i++)
	{
		e[i] = (fckc_u8)((hash >> (i * 8)) & 0xFF);
		if (e[i] == 0xFF)
		{
			e[i] = 0xFE;
		}
	}
	fck_db_id id = fck_db_id_make(e[0], e[1], e[2], e[3], fck_db_type_object);
	return id;
}

static const char *fck_db_make_full_path(char *buffer, fckc_size_t size, const char *dir, const char *path)
{
	if (!buffer || size == 0 || !dir || !path)
	{
		return NULL;
	}

	const fckc_size_t dir_len = strlen(dir);
	const int need_separator = (dir_len > 0 && dir[dir_len - 1] != '/' && path[0] != '/');
	const int written = snprintf(buffer, size, need_separator ? "%s/%s" : "%s%s", dir, path);
	if (written < 0 || (fckc_size_t)written >= size)
	{
		return NULL;
	}
	return buffer;
}

static fck_db_id fck_db_id_advance(fck_db_id id)
{
	fckc_u8 e[4];

	fck_db_object_page_id_extract(~id.index, &e[0], &e[1], &e[2], &e[3]);

	const fckc_size_t iterations = fck_arraysize(e);
	e[iterations - 1] = e[iterations - 1] + 1;
	for (fckc_size_t index = 1; index < iterations; index++)
	{
		const fckc_size_t inverse = iterations - index - 1;
		if (e[inverse] == 0xFF)
		{
			e[inverse] = 0;
			e[inverse + 1] = e[inverse + 1] + 1;
		}
	}

	return fck_db_id_make(e[0], e[1], e[2], e[3], fck_db_type_object);
}

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

static fck_db_id fck_db_id_create_from_uuid(fck_db_private *db, fck_db_uuid uuid)
{
	const fckc_u32 hash = (fckc_u32)uuid.values[0] ^ (fckc_u32)uuid.values[1];

	fckc_u8 e[4];
	for (int i = 0; i < 4; i++)
	{
		e[i] = (fckc_u8)((hash >> (i * 8)) & 0xFF);
		if (e[i] == 0xFF)
		{
			e[i] = 0xFE;
		}
	}

	fck_db_id id = fck_db_id_make(e[0], e[1], e[2], e[3], fck_db_type_object);
	return id;
}

fck_db_object *fck_db_resolve_object(fck_db_object_page_table *table, fck_db_id id)
{
	return fck_db_object_page_table_resolve(table, ~id.index);
}

fck_db_object *fck_db_ensure_object(fck_db_object_page_table *table, fck_db_id id)
{
	return fck_db_object_page_table_ensure(table, ~id.index);
}

fck_db_object *fck_db_add_object(fck_db_object_page_table *table, fck_db_id id)
{
	return fck_db_object_page_table_add(table, ~id.index);
}

int fck_db_remove_object(fck_db_object_page_table *table, fck_db_id id)
{
	return fck_db_object_page_table_remove(table, ~id.index);
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

static fck_db_id fck_db_object_api_create_from_uuid(fck_db external, const char *name, fck_db_uuid uuid)
{
	fck_db_private *db = external.opaque;
	const fck_db_id id = fck_db_id_create_from_uuid(db, uuid);
	// We may have to try again if this one already exists... No check for that yet... Oh boy
	fck_db_object *entry = fck_db_add_object(db->page_table, id);
	fck_assert(entry);
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

static fck_db_undo_scope fck_db_undo_api_create(struct kll_allocator *allocator)
{
	fck_db_undo_scope result = {0};
	result.opaque = (fck_db_undo_scope_private *)kll_malloc(allocator, sizeof(*result.opaque));
	memset(result.opaque, 0, sizeof(*result.opaque));
	result.opaque->allocator = allocator;
	return result;
}

static void fck_db_undo_api_destroy(fck_db_undo_scope scope)
{
	kll_free(scope.opaque->allocator, scope.opaque);
}

static int fck_db_undo_api_undo(fck_db external, fck_db_undo_scope scope)
{
	fck_db_private *db = external.opaque;
	fck_db_undo_scope_private *undo_scope = scope.opaque;

	if (undo_scope->cursor == undo_scope->back)
	{
		return 0;
	}
	undo_scope->cursor = (undo_scope->cursor - 1) % fck_arraysize(undo_scope->units);

	fck_db_undo_unit *unit = undo_scope->units + undo_scope->cursor;

	fck_db_object *target = fck_db_resolve_object(db->page_table, unit->target);
	fck_db_object *copy = fck_db_resolve_object(db->page_table, unit->copy);

	const fck_db_object temp = *target;
	*target = *copy;
	*copy = temp;

	return 1;
}

static int fck_db_undo_api_redo(fck_db external, fck_db_undo_scope scope)
{
	fck_db_private *db = external.opaque;
	fck_db_undo_scope_private *undo_scope = scope.opaque;

	if (undo_scope->cursor == undo_scope->front)
	{
		return 0;
	}
	fck_db_undo_unit *unit = undo_scope->units + undo_scope->cursor;

	fck_db_object *target = fck_db_resolve_object(db->page_table, unit->target);
	fck_db_object *copy = fck_db_resolve_object(db->page_table, unit->copy);

	const fck_db_object temp = *target;
	*target = *copy;
	*copy = temp;

	undo_scope->cursor = (undo_scope->cursor + 1) % fck_arraysize(undo_scope->units);

	return 1;
}

static void fck_db_api_import_file(fck_db external, fck_db_section *section, const char *relative)
{
	fck_db_private *db = external.opaque;

	kll_arena *temp = kll->arena->create(db->allocator, 512);
	const char *ext = fck_db_api_file_extension(relative);
	fck_assert(ext);

	char absolute_buffer[1024];
	const char *absolute = fck_db_make_full_path(absolute_buffer, fck_arraysize(absolute_buffer), section->path, relative);
	if (absolute == NULL)
	{
		return;
	}
	temp->reset(temp);

	// ZERO IS ALWAYS RESERVED TO BE INVALID!
	fck_db_loader_interface null_loader = {.name = "none", .type = NULL};
	fck_db_loader_interface *loader = fck_db_ext_map_find(&db->loaders, ext);
	fck_db_asset *payload = NULL;
	if (!loader)
	{
		loader = &null_loader;
	}

	const fckc_size_t scope_len = strlen(section->scope) + 1; // for / separator
	fckc_size_t total_len;
	char *relative_path;
	if (ext[0] != '\0')
	{
		const fckc_size_t len = to_size_t(ext - relative); // We add a dot dot somewhere above
		total_len = len + scope_len;
		relative_path = (char *)kll_malloc(db->strings, total_len);
		memcpy(relative_path, section->scope, scope_len);
		relative_path[scope_len - 1] = '/';
		char *dst = (char *)memcpy(relative_path + scope_len, relative, len);
		dst[len - 1] = '\0';
	}
	else
	{
		const fckc_size_t len = strlen(relative) + 1;
		total_len = len + scope_len;
		relative_path = (char *)kll_malloc(db->strings, total_len);
		memcpy(relative_path, section->scope, scope_len);
		relative_path[scope_len - 1] = '/';
		memcpy(relative_path + scope_len, relative, len);
	}

	for (fckc_size_t index = 0; index < total_len; index++)
	{
		if (relative_path[index] == '\\')
		{
			relative_path[index] = '/';
		}
	}
	const fck_db_id id = fck_db_id_from_path(relative_path, loader->type);
	if (loader->import)
	{
		const fck_db_loader_args args = {
			.api = db_api,
			.registry = apis,
			.db = external,
			.target = id,
		};
		payload = loader->import(&args, absolute);
	}

	fck_db_object *entry = fck_db_ensure_object(db->page_table, id);
	entry->name = relative_path;

	const fck_db_accessor accessor = fck_db_object_api_edit(external, id);
	accessor.edit->asset(accessor, "asset", payload);
	// accessor.edit->i32(accessor, "type", loader->type);
	accessor.edit->commit(accessor, fck_db_no_undo);

	/*void* dst = fck_db_edit_api_lazy_find(external, &entry->object, fck_db_type_asset, "asset", sizeof(fck_db_asset*),
	sizeof(fck_db_asset*)); memcpy(dst, &payload, sizeof(fck_db_asset*));*/

	kll->arena->destroy(temp);
}

static fck_db_id fck_db_api_id_from_path(fck_db external, const char *path)
{
	fck_db_private *db = (fck_db_private *)external.opaque;
	const char *ext = fck_db_api_file_extension(path);

	char base_path[1024];
	const char *dot = strrchr(path, '.');
	const fckc_size_t path_len = dot ? (fckc_size_t)(dot - path) : strlen(path);
	if (path_len >= sizeof(base_path))
	{
		return fck_db_id_make(255, 255, 255, 255, fck_db_type_object);
	}

	memcpy(base_path, path, path_len);
	base_path[path_len] = '\0';
	fck_db_loader_interface *loader = fck_db_ext_map_find(&db->loaders, ext);
	if (!loader)
	{
		return fck_db_id_from_path(base_path, NULL);
	}
	const fck_db_id id = fck_db_id_from_path(base_path, loader->type);
	return id;
}

static const char *fck_db_api_make_scope_path(char *buffer, fckc_size_t size, const char *scope, const char *relative)
{
	const fckc_size_t scope_len = strlen(scope);
	const fckc_size_t relative_len = strlen(relative);
	const fckc_size_t total_len = scope_len + 1 + relative_len + 1;
	if (total_len > size)
	{
		return NULL;
	}
	memcpy(buffer, scope, scope_len);
	buffer[scope_len] = '/';
	memcpy(buffer + scope_len + 1, relative, relative_len);
	buffer[total_len - 1] = '\0';
	for (fckc_size_t index = 0; index < total_len - 1; index++)
	{
		if (buffer[index] == '\\')
		{
			buffer[index] = '/';
		}
	}
	return buffer;
}

static void fck_db_api_remove_path(fck_db external, fck_db_section *section, fck_db_id id, const char *relative)
{
	(void)section;
	(void)relative;
	fck_db_private *db = (fck_db_private *)external.opaque;
	fck_db_remove_object(db->page_table, id);
	char buffer[1024];

	// const char *full_path = fck_db_make_full_path(buffer, fck_arraysize(buffer), section->path, relative);
	//  TODO
}

static void fck_db_api_hotreload(fck_db external)
{
	fck_db_private *db = (fck_db_private *)external.opaque;

	for (fckc_size_t index = 0; index < db->sections_count; index++)
	{
		fck_db_section *section = db->sections + index;
		const fckc_size_t iteration_limit = 64;
		for (fckc_size_t iterations = 0; iterations < iteration_limit; iterations++)
		{
			fck_file_watcher_event changes[64];
			const fckc_size_t result = os->fw->changes(section->watcher, changes, fck_arraysize(changes));
			if (result == 0)
			{
				break;
			}

			for (fckc_size_t change_index = 0; change_index < result; change_index++)
			{
				fck_file_watcher_event *change = changes + change_index;
				if (strstr(change->path, ".db.fck"))
				{
					continue;
				}
				char buffer[1024];
				const char *scoped_path = fck_db_api_make_scope_path(buffer, fck_arraysize(buffer), section->scope, change->path);

				switch ((fck_file_watcher_event_type)change->type)
				{
				case fck_file_unknown:
					os->io->log("Unknown: %s", change->path);
					break;
				case fck_file_deleted: {
					os->io->log("Deleted: %s", change->path);
					// Not supported - Let's say we load everything always into memory?
					// const fck_db_id id = fck_db_api_id_from_path(external, scoped_path);
					// fck_db_api_remove_path(external, section, id, change->path);
					break;
				}
				case fck_file_modified: {
					os->io->log("Modified: %s", change->path);
					const fck_db_id id = fck_db_api_id_from_path(external, scoped_path);
					// fck_multidir_remove_entry(&db->database.header, &db->database.root, id);
					fck_db_api_import_file(external, section, change->path);
					break;
				}
				case fck_file_created: {
					os->io->log("Created: %s", change->path);
					fck_db_api_import_file(external, section, change->path);
					break;
				}
				}
			}
		}
	}
}

static void fck_db_section_init(fck_db_section *section, const char *scope, const char *path)
{
	section->watcher = os->fw->create(path);
	{
		const fckc_size_t len = strlen(scope) + 1;
		memcpy(section->scope, scope, len);
	}
	{
		const fckc_size_t len = strlen(path) + 1;
		memcpy(section->path, path, len);
	}
}

static void fck_db_api_setup(fck_db external, const char *scope, const char *path)
{
	fck_assert(scope);

	fck_db_private *db = (fck_db_private *)external.opaque;
	fck_assert(db->sections_count < fck_arraysize(db->sections));
	fck_db_section *section = db->sections + db->sections_count;
	db->sections_count = db->sections_count + 1;
	fck_db_section_init(section, scope, path);

	char **paths;
	const fckc_size_t paths_count = os->glob->directory(path, NULL, &paths);
	for (fckc_size_t index = 0; index < paths_count; index++)
	{
		const char *relative = paths[index];
		fck_db_api_import_file(external, section, relative);
	}

	// fck_multidir_iterate(db->page_table.root);

	os->glob->free(paths);
}

static fck_db_asset *fck_db_asset_api_get(fck_db external, fck_db_id id)
{
	fck_db_private *db = (fck_db_private *)external.opaque;
	fck_db_object *entry = fck_db_resolve_object(db->page_table, id);
	if (entry)
	{
		// TODO: Internally, in db, we should use the same code-paths as accessor!!!
		const fck_db_accessor accessor = fck_db_object_api_read(external, id);
		fck_db_asset *asset = accessor.read->asset(accessor, "asset");

		if (asset == NULL)
		{
			return NULL;
		}
		return asset;
	}
	return NULL;
}

static fck_db_asset *fck_db_asset_api_find(fck_db external, const char *path)
{
	fck_db_private *db = (fck_db_private *)external.opaque;
	const fck_db_id id = fck_db_api_id_from_path(external, path);
	return fck_db_asset_api_get(external, id);
}

static fck_db_asset *fck_db_asset_api_upcast(fck_db db, void *value)
{
	(void)db;
	return (fck_db_asset *)value;
}

static fck_db fck_db_api_create(kll_allocator *allocator)
{
	fck_db_private *db = (fck_db_private *)kll_malloc(allocator, sizeof(*db));
	memset(db, 0, sizeof(*db));
	const fck_db result = {.opaque = db};

	db->allocator = allocator;
	db->strings = kll->arena->create(allocator, 512);
	db->page_table = fck_db_object_page_table_alloc(allocator);
	db->loaders = fck_db_ext_map_create(allocator);
	// fck_db_api_import_directory(result, "app", path);

	return result;
}

static void fck_db_api_close(fck_db db)
{
	fck_db_ext_map_destroy(db.opaque->allocator, &db.opaque->loaders);
	fck_db_object_page_table_free(db.opaque->page_table);
	kll_free(db.opaque->allocator, db.opaque);
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

static int fck_db_guid_store(fck_db_guid *guid, fckc_u64 time, fckc_u32 rand, fckc_u32 signal)
{
	if (signal == 0)
	{
		return 0;
	}
	guid->time = time;
	guid->rand = rand;
	const fckc_u32 old = fckc_u32_cas(&guid->signal, 0, signal);

	if (old == 0)
	{
		return 1;
	}
	return 0;
}

static int fck_db_guid_load(fck_db_guid *src, fckc_u64 *time, fckc_u32 *rand, fckc_u32 *signal)
{
	const fckc_u32 sig = fckc_u32_load(&src->signal);
	if (sig == 0)
	{
		return 0;
	}

	*time = src->time;
	*rand = src->rand;
	*signal = sig;
	return 1;
}

static int fck_db_guid_is_ok(fck_db_guid *guid)
{
	return fckc_u32_load(&guid->signal);
}

static void fck_db_guid_generate(fckc_u64 *time, fckc_u32 *rand, fckc_u32 *signal)
{
	fckc_u32 rng_state = (fckc_u32)os->chrono->now() ^ (fckc_u32)(fckc_uintptr)time;
	*time = (fckc_u64)os->chrono->now();
	*rand = ((fckc_u64)fck_xorshift32(&rng_state) << 32) | (fckc_u64)fck_xorshift32(&rng_state);
	*signal = fck_xorshift32(&rng_state);
	// fckc_spin(!fck_db_guid_store(slot, time, rand_val, signal));
}

static fckc_u64 fck_db_guid_raw_hash(fckc_u64 time, fckc_u32 rand, fckc_u32 signal)
{
	fckc_u64 hash = fck_hash_combine(time, (to_u64(rand) << 32) | to_u64(signal));
	// Extract two bits for the state...
	hash = hash & to_u64(~0LLU >> 2);
	return hash;
}

static int fck_db_guid_equals(fck_db_guid *guid, fckc_u64 time, fckc_u32 rand, fckc_u32 signal)
{
	fckc_u64 t;
	fckc_u32 r;
	fckc_u32 s;
	if (fck_db_guid_load(guid, &t, &r, &s))
	{
		return time == t && rand == r && signal == s;
	}
	return 0;
}

static fckc_u64 fck_db_guid_id_map_probe(fck_db_guid_id_map *map, fckc_u64 hash)
{
	for (fckc_size_t index = 0; index < map->capacity; index++)
	{
		const fckc_size_t slot = (hash + index) % map->capacity;
		fck_db_guid_key_value *item = map->values + slot;
		if (!item->state.ok || item->state.tomb)
		{
			return slot + 1;
		}
	}
	fck_assert(0 && "out of capacity - this should NOT happen");
	return 0;
}

static fckc_u64 fck_db_guid_id_map_find(fck_db_guid_id_map *map, fckc_u64 time, fckc_u32 rand, fckc_u32 signal)
{
	const fckc_u64 hash = fck_db_guid_raw_hash(time, rand, signal);

	for (fckc_size_t index = 0; index < map->capacity; index++)
	{
		const fckc_size_t slot = (hash + index) % map->capacity;
		fck_db_guid_key_value *item = map->values + slot;
		if (!item->state.ok)
		{
			break;
		}
		if (!item->state.tomb && item->state.value == hash)
		{
			if (fck_db_guid_equals(&item->guid, time, rand, signal))
			{
				return slot + 1;
			}
		}
	}
	return 0;
}

static fckc_u64 fck_db_guid_id_map_next(fck_db_guid_id_map *map, fckc_u64 *value)
{
	for (fckc_size_t index = *value; index < map->capacity; index++)
	{
		const fck_db_guid_key_value *item = map->values + index;
		if (item->state.ok)
		{
			// Asset that item->state.tomb is 0?
			*value = index + 1;
			return *value;
		}
	}
	return 0;
}

static fckc_u64 fck_db_guid_id_map_add(fck_db_guid_id_map *map, fckc_u64 hash, fckc_u64 time, fckc_u32 rand, fckc_u32 signal, fck_db_id id)
{
	const fckc_u64 result = fck_db_guid_id_map_probe(map, hash);
	if (result)
	{
		fck_db_guid_key_value *item = map->values + result - 1;
		item->state.ok = 1;
		item->state.tomb = 0;
		item->state.value = hash;
		item->id = id;
		// When we lock the map later, we should maintain exclusive access here!
		fck_db_guid_store(&item->guid, time, rand, signal);
		map->count = map->count + 1;
	}
	return result;
}

static fckc_u64 fck_db_guid_id_map_grow_add(fck_db_guid_id_map *map, fckc_u64 time, fckc_u32 rand, fckc_u32 signal, fck_db_id id)
{
	{
		const fckc_u64 result = fck_db_guid_id_map_find(map, time, rand, signal);
		if (result)
		{
			return result;
		}
	}

	// TODO: Locking! :-D
	if (map->count > map->capacity / 2)
	{
		const fckc_size_t capacity = map->capacity ? 32 : map->capacity * 4;
		const fckc_size_t total = sizeof(*map->values) * capacity;
		fck_db_guid_key_value *values = (fck_db_guid_key_value *)kll_malloc(kll->system, total);
		memset(values, 0, total);

		fck_db_guid_id_map next = {.values = values, .capacity = capacity};
		fckc_u64 it = 0;
		while (fck_db_guid_id_map_next(map, &it))
		{
			fck_db_guid_key_value *item = map->values + it - 1;
			if (item->state.ok)
			{
				fckc_u64 t;
				fckc_u32 r;
				fckc_u32 s;
				if (fck_db_guid_load(&item->guid, &t, &r, &s))
				{
					fck_db_guid_id_map_add(&next, item->state.value, t, r, s, item->id);
				}
			}
		}

		kll_free(kll->system, map->values);
		*map = next;
	}

	{
		const fckc_u64 hash = fck_db_guid_raw_hash(time, rand, signal);
		const fckc_u64 result = fck_db_guid_id_map_add(map, hash, time, rand, signal, id);
		return result;
	}
}

static void fck_db_object_uuid_mapping(fck_db external, fck_db_id id, fck_db_object *obj)
{
	fckc_u64 time;
	fckc_u32 rand;
	fckc_u32 signal;
	fck_db_guid_generate(&time, &rand, &signal);

	fck_db_private *db = external.opaque;

	db->page_table;
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

static fckc_u64 fck_db_object_uuid_to_u64(fck_db_uuid uuid)
{
	return ((fckc_u64)uuid.values[1] << 32) | ((fckc_u64)uuid.values[0] & 0xFFFFFFFFULL);
}

static fck_db_uuid fck_db_object_u64_to_uuid(fckc_u64 val)
{
	fck_db_uuid uuid;
	uuid.values[0] = (fckc_u32)(val & 0xFFFFFFFFULL);
	uuid.values[1] = (fckc_u32)(val >> 32);
	return uuid;
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

static fck_db_asset_api db_asset_api = {
	.find = fck_db_asset_api_find,
	.get = fck_db_asset_api_get,
};

static fck_db_undo_api db_undo_api = {
	.create = fck_db_undo_api_create,
	.destroy = fck_db_undo_api_destroy,
	.undo = fck_db_undo_api_undo,
	.redo = fck_db_undo_api_redo,
};

static fck_db_asset *fck_directory_import(const fck_db_loader_args *args, const char *path)
{
	(void)args;
	fck_path_info info;
	if (os->fs->info(path, &info))
	{
		os->io->log("Load Directory: %s", path);
		if (info.type == fck_path_directory)
		{
			return NULL;
		}
		return NULL;
	}

	return NULL;
}
static fckc_size_t fck_directory_supports(const char ***extensions)
{
	static const char *supported[] = {""};
	*extensions = supported;
	return fck_arraysize(supported);
}

static fck_db_loader_interface directory_loader = {
	.type = "direcotry",
	.name = "directory",
	.import = fck_directory_import,
	.supports = fck_directory_supports,
};

FCK_EXPORT_API fck_db_api *fck_db_load(fck_api_registry *registry, void *old)
{
	static fck_db_api api = {
		.object = &db_object_api,
		.asset = &db_asset_api,
		.undo = &db_undo_api,

		.create = fck_db_api_create,
		.setup = fck_db_api_setup,
		.hotreload = fck_db_api_hotreload,
		.close = fck_db_api_close,
	};
	// Runtime resolved addresses...
	api.set = db_id_set;
	db_api = &api;

	(void)old;
	apis = registry;
	apis->add(fck_db_api_name, db_api);
	apis->add(fck_db_loader_interface_name, &directory_loader);

	return db_api;
}