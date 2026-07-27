
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

#define fck_multidir_child_capacity 255
#define fck_multidir_bitset_capacity 4
#define fck_multidir_bitset_chunk_capacity 64

static fck_api_registry *apis = NULL;

typedef struct fck_db_ext_map_entry
{
	const char *extension;
	fck_db_loader_interface *loader;
} fck_db_ext_map_entry;

typedef struct fck_db_ext_map
{
	fck_db_ext_map_entry *entries;
	fckc_size_t capacity;
} fck_db_ext_map;

typedef struct fck_db_header
{
	kll_allocator *allocator;
	// TODO: better strategy for strings?
} fck_db_header;

struct fck_db_property_instance;
typedef struct fck_db_property_instance
{
	const char *name;
	fck_db_type type;
	fckc_u32 offset;
} fck_db_property_instance;

typedef struct fck_db_object
{
	fck_db_property_instance *properties;
	void *data;

	fckc_size_t at;
	fckc_size_t size;

	fckc_u32 capacity;
	fckc_u32 count;

	fckc_u32 version;
} fck_db_object;

typedef struct fck_db_memory
{
	fckc_size_t count;
	fckc_size_t capacity;
	fckc_u8 data[1];
} fck_db_memory;

typedef struct fck_multidir_entry
{
	const char *name;
	fck_db_object object;
} fck_multidir_entry;

union fck_multidir;
typedef union fck_multidir {
	struct
	{
		fckc_u64 ok[fck_multidir_bitset_capacity];
		union fck_multidir *children;
	} dir;

	fck_multidir_entry entry;
} fck_multidir;

typedef struct fck_database
{
	fck_db_header header;
	fck_multidir root;
} fck_database;

typedef struct fck_db_section
{
	fck_file_watcher watcher;
	char path[420];
	char scope[256];
} fck_db_section;

typedef struct fck_db_private
{
	kll_allocator *allocator;
	kll_arena *strings;

	fck_db_ext_map loaders;

	fck_database database;
	fck_db_section sections[16];
	fckc_size_t sections_count;

	fckc_u8 id_factory[4];
} fck_db_private;

typedef struct fck_db_undo_unit
{
	fck_db_id target;
	fck_db_id copy;
} fck_db_undo_unit;

typedef struct fck_db_undo_scope_private
{
	struct kll_allocator *allocator;
	fck_db_undo_unit units[16];
	fckc_u32 cursor;
	fckc_u32 front;
	fckc_u32 back;
} fck_db_undo_scope_private;

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

static int fck_db_property_is_used(const fck_db_property_instance *property)
{
	return property->name && property->type != ~0;
}

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

static fckc_size_t fck_db_object_add_ng(fck_db_object *instance, fck_db_type type, const char *name, fckc_size_t offset)
{
	const fckc_size_t len = strlen(name);
	const fckc_u64 hash = to_u64(fck_hash(name, len));
	const fckc_size_t capacity = instance->capacity;

	for (fckc_u64 index = 0; index < capacity; index++)
	{
		const fckc_u64 slot = (hash + index) % capacity;
		fck_db_property_instance *property = instance->properties + slot;
		if (property->name == NULL || property->type == ~0)
		{
			// Add
			instance->count = instance->count + 1;
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
		if (property->name == NULL || property->type == ~0)
		{
			continue;
		}

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
		if ((type == fck_db_type_none || type == property->type) && strcmp(property->name, name) == 0)
		{
			property->type = ~0;
			fck_db_object_adjust_offsets(instance, property->offset, size);
			property->offset = 0;
			return slot;
		}
	}
	return 0;
}

static fckc_size_t fck_db_object_add(fck_db_header *header, fck_db_object *instance, fck_db_type type, const char *name)
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
		fck_db_property_instance *props = (fck_db_property_instance *)kll_malloc(header->allocator, total);
		memset(props, 0, total);

		const fckc_u32 old_capacity = instance->capacity;
		fck_db_property_instance *previous = instance->properties;

		instance->properties = props;
		instance->capacity = capacity;
		if (previous)
		{

			for (fckc_u32 index = 0; index < old_capacity; index++)
			{
				fck_db_property_instance *property = previous + index;
				const fckc_size_t result = fck_db_object_add_ng(instance, property->type, property->name, property->offset);
				fck_assert(result);
			}
			kll_free(header->allocator, previous);
		}
	}

	return fck_db_object_add_ng(instance, type, name, instance->size);
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

static void fck_db_id_extract(fck_db_id id, fckc_u8 *e0, fckc_u8 *e1, fckc_u8 *e2, fckc_u8 *e3)
{
	id.index = id.index ^ 0xFFFFFFFFU;
	if (e0)
	{
		*e0 = (fckc_u8)((id.index >> 24) & 0xFF);
	}
	if (e1)
	{
		*e1 = (fckc_u8)((id.index >> 16) & 0xFF);
	}
	if (e2)
	{
		*e2 = (fckc_u8)((id.index >> 8) & 0xFF);
	}
	if (e3)
	{
		*e3 = (fckc_u8)(id.index & 0xFF);
	}
}

static fck_db_id fck_db_id_make(fckc_u8 e0, fckc_u8 e1, fckc_u8 e2, fckc_u8 e3)
{
	fck_db_id id = {0};
	id.index = ((fckc_u32)e0 << 24) | ((fckc_u32)e1 << 16) | ((fckc_u32)e2 << 8) | (fckc_u32)e3;
	id.index = id.index ^ 0xFFFFFFFFU;
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

static fck_multidir_entry *fck_multidir_resolve(fck_multidir *root, fck_db_id id)
{
	fckc_u8 e[4];
	fck_db_id_extract(id, &e[0], &e[1], &e[2], &e[3]);
	const int is_ok = fck_multidir_id_is_ok(e[0], e[1], e[2], e[3]);
	if (!is_ok)
	{
		return NULL;
	}

	fck_multidir *current = root;
	for (fckc_size_t index = 0; index < fck_arraysize(e); index++)
	{
		if (current->dir.children == NULL)
		{
			return NULL;
		}
		const fckc_u8 subid = e[index];
		fck_multidir *next = current->dir.children + subid;
		current = next;
	}

	return &current->entry;
}

static void fck_multidir_bit_set_ok(fck_multidir *dir, const fckc_u8 subid)
{
	const fckc_u8 chunk = subid / fck_multidir_bitset_chunk_capacity;
	const fckc_u8 local = subid % fck_multidir_bitset_chunk_capacity;
	dir->dir.ok[chunk] = dir->dir.ok[chunk] | (1LLU << local);
}

static int fck_multidir_bit_is_ok(const fck_multidir *dir, const fckc_u8 subid)
{
	const fckc_u8 chunk = subid / fck_multidir_bitset_chunk_capacity;
	const fckc_u8 local = subid % fck_multidir_bitset_chunk_capacity;
	return (dir->dir.ok[chunk] & (1LLU << local)) == (1LLU << local);
}

static void fck_multidir_bit_clear_ok(fck_multidir *dir, const fckc_u8 subid)
{
	const fckc_u8 chunk = subid / fck_multidir_bitset_chunk_capacity;
	const fckc_u8 local = subid % fck_multidir_bitset_chunk_capacity;
	dir->dir.ok[chunk] = dir->dir.ok[chunk] & ~(1LLU << local);
}

static int fck_multidir_empty(fck_multidir *dir)
{
	for (fckc_size_t chunk = 0; chunk < fck_arraysize(dir->dir.ok); chunk++)
	{
		if (dir->dir.ok[chunk])
		{
			return 0;
		}
	}
	return 1;
}

static fck_multidir_entry *fck_multidir_add_entry(fck_db_header *header, fck_multidir *root, fck_db_id id, const char *name)
{
	fckc_u8 e[4];
	const fckc_size_t indirections = fck_arraysize(e);
	fck_db_id_extract(id, &e[0], &e[1], &e[2], &e[3]);
	const int is_ok = fck_multidir_id_is_ok(e[0], e[1], e[2], e[3]);
	if (!is_ok)
	{
		return NULL;
	}

	kll_allocator *allocator = header->allocator;

	fck_multidir *parents[fck_arraysize(e)];

	fck_multidir *current = root;
	for (fckc_size_t index = 0; index < indirections; index++)
	{
		const fckc_u8 subid = e[index];
		if (subid == 0xFF)
		{
			return NULL;
		}

		if (current->dir.children == NULL)
		{
			const fckc_size_t total = fck_multidir_child_capacity * sizeof(*current->dir.children);
			current->dir.children = (fck_multidir *)kll_malloc(allocator, total);
			memset(current->dir.children, 0, total);
		}

		parents[index] = current;
		fck_multidir *next = current->dir.children + subid;
		current = next;
	}

	current->entry.name = name;

	for (fckc_size_t index = 0; index < indirections; index++)
	{
		const fckc_u8 subid = e[index];
		fck_multidir *dir = parents[index];
		fck_multidir_bit_set_ok(dir, subid);
	}

	return &current->entry;
}

static fck_multidir_entry *fck_multidir_remove_entry(fck_db_header *header, fck_multidir *root, fck_db_id id)
{
	fckc_u8 e[4];
	const fckc_size_t indirections = fck_arraysize(e);
	fck_db_id_extract(id, &e[0], &e[1], &e[2], &e[3]);
	const int is_ok = fck_multidir_id_is_ok(e[0], e[1], e[2], e[3]);
	if (!is_ok)
	{
		return NULL;
	}

	kll_allocator *allocator = header->allocator;

	fck_multidir *parents[fck_arraysize(e)];
	fck_multidir *current = root;
	for (fckc_size_t index = 0; index < indirections; index++)
	{
		const fckc_u8 subid = e[index];
		if (subid == 0xFF)
		{
			return NULL;
		}

		if (!fck_multidir_bit_is_ok(current, subid))
		{
			return NULL;
		}
		fck_assert(current->dir.children);

		parents[index] = current;
		fck_multidir *next = current->dir.children + subid;
		current = next;
	}

	for (fckc_size_t index = 0; index < indirections; index++)
	{
		const fckc_size_t inverse = indirections - index - 1;
		const fckc_u8 subid = e[inverse];
		fck_multidir *dir = parents[inverse];
		fck_multidir_bit_clear_ok(dir, subid);
		if (fck_multidir_empty(dir))
		{
			kll_free(allocator, dir->dir.children);
			dir->dir.children = NULL;
			continue;
		}
		break;
	}

	return &current->entry;
}

static int fck_multidir_ctz64(fckc_u64 v)
{
	static const int bit_positions[64] = {
		0,  1, 2,  7,  3,  13, 8,  19, 4,  25, 14, 28, 9,  34, 20, 40, 5,  17, 26, 38, 15, 46, 29, 48, 10, 31, 35, 54, 21, 50, 41, 57,
		63, 6, 12, 18, 24, 27, 33, 39, 16, 37, 45, 47, 30, 53, 49, 56, 62, 11, 23, 32, 36, 44, 52, 55, 61, 22, 43, 51, 60, 42, 59, 58,
	};
	return bit_positions[(to_u64((v & (0ULL - v)) * 0x0218A392CD3D5DBFULL)) >> 58];
}

static void fck_multidir_iterate_fast(fck_multidir *root, int level)
{
	for (fckc_size_t chunk_index = 0; chunk_index < fck_arraysize(root->dir.ok); chunk_index++)
	{
		const int chunk_offset = chunk_index * fck_multidir_bitset_chunk_capacity;
		fckc_u64 current = root->dir.ok[chunk_index];
		while (current)
		{
			const int index = fck_multidir_ctz64(current);
			const int child_index = index + chunk_offset;
			fck_multidir *child = root->dir.children + child_index;
			if (level != 3)
			{
				fck_multidir_iterate_fast(child, level + 1);
			}
			else
			{
				os->io->log(child->entry.name);
			}
			current = current & (current - 1ULL);
		}
	}
}

static void fck_multidir_iterate(fck_multidir *root)
{
	fck_multidir_iterate_fast(root, 0);
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

static fck_db_id fck_db_id_from_path(const char *path, fckc_u16 type)
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

	hash ^= (fckc_u32)type << 16;

	fckc_u8 e[4];
	for (int i = 0; i < 4; i++)
	{
		e[i] = (fckc_u8)((hash >> (i * 8)) & 0xFF);
		if (e[i] == 0xFF)
		{
			e[i] = 0xFE;
		}
	}
	fck_db_id id = fck_db_id_make(e[0], e[1], e[2], e[3]);
	id.type = type;
	id.generation = 0;

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

static fck_db_api db_api;

static void *fck_db_edit_api_lazy_find(fck_db external, fck_db_object *obj, fck_db_type type, const char *property, fckc_size_t s,
                                       fckc_size_t a)
{
	fck_db_private *db = external.opaque;
	fckc_size_t result = fck_db_object_add(&db->database.header, obj, type, property);
	fck_assert(result);

	fck_db_property_instance *prop = obj->properties + result - 1;
	if (prop->offset == obj->size)
	{
		prop->offset = fckc_align(obj->at, a);
		obj->at = prop->offset + s;

		if (obj->at >= obj->size)
		{
			const fckc_size_t capacity = obj->size ? obj->size * 2 : 64;
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

	fck_db_loader_interface null_loader = {.name = "none", .type = to_u16(~0)};
	fck_db_loader_interface *loader = fck_db_ext_map_find(&db->loaders, ext);
	fck_db_asset *payload;
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
			.api = &db_api,
			.registry = apis,
			.db = external,
			.target = id,
		};
		payload = loader->import(&args, absolute);
	}

	fck_multidir_entry *entry = fck_multidir_add_entry(&db->database.header, &db->database.root, id, relative_path);
	void *dst = fck_db_edit_api_lazy_find(external, &entry->object, fck_db_type_asset, "_", sizeof(fck_db_asset *), sizeof(fck_db_asset *));
	memcpy(dst, &payload, sizeof(fck_db_asset *));

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
		return fck_db_id_make(255, 255, 255, 255);
	}

	memcpy(base_path, path, path_len);
	base_path[path_len] = '\0';
	fck_db_loader_interface *loader = fck_db_ext_map_find(&db->loaders, ext);
	if (!loader)
	{
		return fck_db_id_from_path(base_path, to_u16(~0));
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
	fck_multidir_remove_entry(&db->database.header, &db->database.root, id);
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

	fck_multidir_iterate(&db->database.root);

	os->glob->free(paths);
}

static fck_db_asset *fck_db_asset_api_get(fck_db external, fck_db_id id)
{
	fck_db_private *db = (fck_db_private *)external.opaque;
	fck_multidir_entry *entry = fck_multidir_resolve(&db->database.root, id);
	if (entry)
	{
		// TODO: Internally, in db, we should use the same code-paths as accessor!!!
		fck_db_object *obj = &entry->object;
		const fckc_size_t result = fck_db_object_find(&entry->object, fck_db_type_asset, "_");
		if (result == 0)
		{
			return NULL;
		}
		fck_db_property_instance *prop = obj->properties + result - 1;
		fckc_u8 *src = (fckc_u8 *)fckc_pointer_add(obj->data, prop->offset);
		fck_db_asset *value;
		memcpy(&value, src, sizeof(fck_db_asset *));
		return value;
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
	db->database.header.allocator = allocator;
	db->loaders = fck_db_ext_map_create(allocator);

	// fck_db_api_import_directory(result, "app", path);

	return result;
}

static void fck_db_api_close(fck_db db)
{
	fck_db_ext_map_destroy(db.opaque->allocator, &db.opaque->loaders);
	kll_free(db.opaque->allocator, db.opaque);
}

static fck_db_id fck_db_id_advance(fck_db_id id)
{
	fckc_u8 e[4];

	fck_db_id_extract(id, &e[0], &e[1], &e[2], &e[3]);

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

	return fck_db_id_make(e[0], e[1], e[2], e[3]);
}

static fck_db_id fck_db_id_create_and_next(fck_db_private *db)
{
	fckc_u8 *e = db->id_factory;
	const fckc_size_t iterations = fck_arraysize(db->id_factory);

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

	const fck_db_id id = fck_db_id_make(e[0], e[1], e[2], e[3]);
	return id;
}

static void *fck_db_read_api_opaque(fck_db_accessor accessor, fck_db_type type, const char *property)
{
	fck_db_private *db = accessor.db.opaque;
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

static void fck_db_edit_api_asset(fck_db_accessor accessor, const char *property, fck_db_asset *value)
{
	const fckc_size_t s = sizeof(fck_db_asset *);
	const fckc_size_t a = alignof(fck_db_asset *);
	void *dst = fck_db_edit_api_lazy_find(accessor.db, accessor.obj, fck_db_type_asset, property, s, a);
	memcpy(dst, &value, sizeof(fck_db_asset *));
}

static void fck_db_edit_api_reference(fck_db_accessor accessor, const char *property, fck_db_id value)
{
	const fckc_size_t s = sizeof(fck_db_asset *);
	const fckc_size_t a = alignof(fck_db_asset *);
	void *dst = fck_db_edit_api_lazy_find(accessor.db, accessor.obj, fck_db_type_reference, property, s, a);
	memcpy(dst, &value, sizeof(fck_db_asset *));
}

static void fck_db_edit_api_memory(fck_db_accessor accessor, const char *property, const void *data, fckc_size_t size)
{
	void *current = fck_db_read_api_opaque(accessor, fck_db_type_f32, property);
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
			const fckc_size_t result = fck_db_object_remove(accessor.obj, fck_db_type_memory, property, total);
			fck_assert(result);
		}
	}

	{
		void *dst = fck_db_edit_api_lazy_find(accessor.db, accessor.obj, fck_db_type_memory, property, total, alignof(fck_db_memory));
		fck_db_memory *memory = (fck_db_memory *)dst;
		memcpy((void *)memory->data, data, size);
		memory->capacity = size;
		memory->count = size;
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
		fck_db_edit_api_reference(accessor, property, value->reference);
		break;
	case fck_db_type_asset:
		fck_db_edit_api_asset(accessor, property, value->asset);
		break;
	}
	return;
}

static void fck_db_edit_api_commit(fck_db_accessor accessor, fck_db_undo_scope external)
{
	// TODO: How do we invalidate the accessor???
	fck_db_private *db = accessor.db.opaque;
	fck_db_object *obj = accessor.obj;

	fck_multidir_entry *original = fck_multidir_resolve(&db->database.root, accessor.original);
	fck_multidir_entry *inflight = fck_multidir_resolve(&db->database.root, accessor.inflight);

	// Swap out the objects - This needs to be done way more elegantly in the future
	// But for now I need to port assets away
	fck_db_object temp = original->object;
	original->object = inflight->object;
	inflight->object = temp;

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

struct fck_db_edit_api db_edit_api = {
	.variant = fck_db_edit_api_variant,
	.i32 = fck_db_edit_api_i32,
	.f32 = fck_db_edit_api_f32,
	.asset = fck_db_edit_api_asset,
	.memory = fck_db_edit_api_memory,
	.reference = fck_db_edit_api_reference,
	.commit = fck_db_edit_api_commit,
};

static fckc_i32 fck_db_read_api_i32(fck_db_accessor accessor, const char *property)
{
	const void *src = fck_db_read_api_opaque(accessor, fck_db_type_i32, property);
	fck_assert(src);
	fckc_i32 value;
	memcpy(&value, src, sizeof(value));
	return value;
}

static fckc_f32 fck_db_read_api_f32(fck_db_accessor accessor, const char *property)
{
	const void *src = fck_db_read_api_opaque(accessor, fck_db_type_f32, property);
	fck_assert(src);
	fckc_f32 value;
	memcpy(&value, src, sizeof(value));
	return value;
}

static fck_db_asset *fck_db_read_api_asset(fck_db_accessor accessor, const char *property)
{
	const void *src = fck_db_read_api_opaque(accessor, fck_db_type_asset, property);
	fck_assert(src);
	fck_db_asset *value;
	memcpy(&value, src, sizeof(fck_db_asset *));
	return value;
}

static fck_db_id fck_db_read_api_reference(fck_db_accessor accessor, const char *property)
{
	const void *src = fck_db_read_api_opaque(accessor, fck_db_type_reference, property);
	fck_assert(src);
	fck_db_id value;
	memcpy(&value, src, sizeof(value));
	return value;
}

static fckc_size_t fck_db_read_api_memory(fck_db_accessor accessor, const char *property, const void **data)
{
	const void *src = fck_db_read_api_opaque(accessor, fck_db_type_memory, property);
	fck_assert(src);
	fck_db_memory *memory = (fck_db_memory *)src;
	*data = (const void *)memory->data;
	return memory->count;
}

static fck_db_property fck_db_make_variant(fck_db_type type, const fckc_u8 *data, fckc_size_t offset)
{
	fck_db_property result = {0};
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
	}
	break;
	case fck_db_type_reference:
		memcpy(&result.reference, src, sizeof(result.reference));
		break;
	case fck_db_type_asset:
		memcpy(&result.asset, src, sizeof(fck_db_asset *));
		break;
	}
	return result;
}

static fck_db_property fck_db_read_api_variant(fck_db_accessor accessor, const char *property)
{
	fck_db_property result = {0};

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
	fck_db_private *db = accessor.db.opaque;
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

struct fck_db_read_api db_read_api = {
	.iterate = fck_db_object_api_iterate,
	.version = fck_db_object_api_version,
	.variant = fck_db_read_api_variant,
	.i32 = fck_db_read_api_i32,
	.f32 = fck_db_read_api_f32,
	.asset = fck_db_read_api_asset,
	.reference = fck_db_read_api_reference,
	.memory = fck_db_read_api_memory,
};

static fck_db_id fck_db_object_api_create(fck_db external, const char *name)
{
	fck_db_private *db = external.opaque;
	const fck_db_id id = fck_db_id_create_and_next(db);
	fck_multidir_entry *entry = fck_multidir_add_entry(&db->database.header, &db->database.root, id, name);
	fck_assert(entry);
	return id;
}

static fck_db_accessor fck_db_object_api_edit(fck_db external, fck_db_id id)
{
	fck_db_private *db = external.opaque;
	fck_multidir_entry *entry = fck_multidir_resolve(&db->database.root, id);
	fck_assert(entry);

	// fck_db_object_api_create(id, entry->name);
	const fck_db_id temp = fck_db_object_api_create(external, entry->name); // fck_db_id_advance(id);
	fck_multidir_entry *copy = fck_multidir_add_entry(&db->database.header, &db->database.root, temp, entry->name);
	copy->object = fck_db_object_clone(db->allocator, &entry->object);

	fck_db_accessor accessor = {
		.original = id,
		.inflight = temp,
		.edit = &db_edit_api,
		.db = db,
		.read = &db_read_api,
		.obj = &copy->object,
	};

	return accessor;
}

static fck_db_accessor fck_db_object_api_read(fck_db external, fck_db_id id)
{
	fck_db_private *db = external.opaque;
	fck_multidir_entry *entry = fck_multidir_resolve(&db->database.root, id);
	fck_assert(entry);

	fck_db_accessor accessor = {
		.original = id,
		.inflight = id,
		.edit = NULL,
		.db = db,
		.read = &db_read_api,
		.obj = &entry->object,
	};

	return accessor;
}

static void fck_db_object_api_destroy(fck_db external, fck_db_id id)
{
	fck_db_private *db = external.opaque;
	fck_multidir_remove_entry(&db->database.header, &db->database.root, id);
}

static fck_db_undo_scope fck_db_undo_api_create(struct kll_allocator *allocator)
{
	fck_db_undo_scope result = {0};
	result.opaque = kll_malloc(allocator, sizeof(*result.opaque));
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

	fck_multidir_entry *target = fck_multidir_resolve(&db->database.root, unit->target);
	fck_multidir_entry *copy = fck_multidir_resolve(&db->database.root, unit->copy);

	fck_db_object temp = target->object;
	target->object = copy->object;
	copy->object = temp;

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

	fck_multidir_entry *target = fck_multidir_resolve(&db->database.root, unit->target);
	fck_multidir_entry *copy = fck_multidir_resolve(&db->database.root, unit->copy);

	fck_db_object temp = target->object;
	target->object = copy->object;
	copy->object = temp;

	undo_scope->cursor = (undo_scope->cursor + 1) % fck_arraysize(undo_scope->units);

	return 1;
}

static fck_db_object_api db_object_api = {
	.create = fck_db_object_api_create,
	.destroy = fck_db_object_api_destroy,
	.read = fck_db_object_api_read,
	.edit = fck_db_object_api_edit,
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

static fck_db_api db_api = {
	.object = &db_object_api,
	.asset = &db_asset_api,
	.undo = &db_undo_api,

	.create = fck_db_api_create,
	.setup = fck_db_api_setup,
	.hotreload = fck_db_api_hotreload,
	.close = fck_db_api_close,
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
	.type = 1,
	.name = "directory",
	.import = fck_directory_import,
	.supports = fck_directory_supports,
};

FCK_EXPORT_API fck_db_api *fck_db_load(fck_api_registry *registry, void *old)
{
	(void)old;
	apis = registry;
	apis->add(fck_db_api_name, &db_api);
	apis->add(fck_db_loader_interface_name, &directory_loader);

	return &db_api;
}