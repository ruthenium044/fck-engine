
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

#include "fck_db_core.inl"
#include "fck_db_object_page_table.h"

#include "fck_db_undo.h"

#include "fck_db_object.h"

#include "fck_db_id_set.h"

static fck_api_registry *apis = NULL;
static fck_db_api *db_api;

// Should and could be public! :)
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
int fck_db_property_is_used(const fck_db_property_instance *property)
{
	return property->name && property->type != fck_db_type_none;
}
// Properties
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
// Properties
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

// MAYBE
static inline fckc_u64 fck_db_random_next_rotl(const fckc_u64 x, int k)
{
	return (x << k) | (x >> (64 - k));
}
// MAYBE
static inline fckc_u64 fck_db_random_splitmix64(fckc_u64 *state)
{
	*state += 0x9e3779b97f4a7c15; // Golden ratio increment
	fckc_u64 z = *state;
	z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
	z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
	return z ^ (z >> 31);
}

// CORE
fck_db_id fck_db_id_make(fckc_u8 e0, fckc_u8 e1, fckc_u8 e2, fckc_u8 e3, fck_db_type type)
{
	fck_db_id id = {0};
	id.index = ((fckc_u32)e0 << 24) | ((fckc_u32)e1 << 16) | ((fckc_u32)e2 << 8) | (fckc_u32)e3;
	id.index = id.index ^ 0xFFFFFFFFU;
	id.type = type;
	return id;
}

// ext_map
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
// ext_map
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
// ext_map
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
// ext_map
static void fck_db_ext_map_destroy(kll_allocator *allocator, fck_db_ext_map *map)
{
	kll_free(allocator, map->entries);
	map->capacity = 0;
}

// CORE
fck_db_object *fck_db_resolve_object(fck_db_object_page_table *table, fck_db_id id)
{
	return fck_db_object_page_table_resolve(table, ~id.index);
}
// CORE
fck_db_object *fck_db_ensure_object(fck_db_object_page_table *table, fck_db_id id)
{
	return fck_db_object_page_table_ensure(table, ~id.index);
}
// CORE
fck_db_object *fck_db_add_object(fck_db_object_page_table *table, fck_db_id id)
{
	return fck_db_object_page_table_add(table, ~id.index);
}
// CORE
int fck_db_remove_object(fck_db_object_page_table *table, fck_db_id id)
{
	return fck_db_object_page_table_remove(table, ~id.index);
}

// ASSETS
static const char *fck_db_api_file_extension(const char *path)
{
	const char *dot = strrchr(path, '.');
	if (!dot || dot == path)
	{
		return "";
	}

	return dot + 1;
}
// ASSETS
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
// ASSETS
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
// ASSETS
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

	const fck_db_accessor accessor = db_object->edit(external, id);
	accessor.edit->asset(accessor, "asset", payload);
	// accessor.edit->i32(accessor, "type", loader->type);
	accessor.edit->commit(accessor, fck_db_no_undo);

	/*void* dst = fck_db_edit_api_lazy_find(external, &entry->object, fck_db_type_asset, "asset", sizeof(fck_db_asset*),
	sizeof(fck_db_asset*)); memcpy(dst, &payload, sizeof(fck_db_asset*));*/

	kll->arena->destroy(temp);
}
// ASSETS
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
// ASSETS
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
// ASSETS
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

// CORE
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
		const fck_db_accessor accessor = db_object->read(external, id);
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

static fck_db_asset_api db_asset_api = {
	.find = fck_db_asset_api_find,
	.get = fck_db_asset_api_get,
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
		.asset = &db_asset_api,

		.create = fck_db_api_create,
		.setup = fck_db_api_setup,
		.hotreload = fck_db_api_hotreload,
		.close = fck_db_api_close,
	};
	// "Runtime" resolved addresses...
	api.set = db_id_set;
	api.object = db_object;
	api.undo = db_undo;
	db_api = &api;

	(void)old;
	apis = registry;
	apis->add(fck_db_api_name, db_api);
	apis->add(fck_db_loader_interface_name, &directory_loader);

	return db_api;
}