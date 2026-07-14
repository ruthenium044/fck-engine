
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

#include "fck_serialiser.h"
#include "fck_serialiser_json.h"
#include "fck_serialiser_text.h"

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

typedef struct fck_multidir_entry
{
	const char *name; // ?
	fck_db_element *element;
} fck_multidir_entry;

union fck_multidir;
typedef union fck_multidir {
	struct
	{
		fckc_u64 ok[fck_multidir_bitset_capacity];
	} common;

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
	char database_path[420];
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
} fck_db_private;

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

static fck_db_element *fck_multidir_resolve(fck_multidir *root, fck_db_id id)
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

	return current->entry.element;
}

static void fck_multidir_bit_set_ok(fck_multidir *dir, const fckc_u8 subid)
{
	const fckc_u8 chunk = subid / fck_multidir_bitset_chunk_capacity;
	const fckc_u8 local = subid % fck_multidir_bitset_chunk_capacity;
	dir->common.ok[chunk] = dir->common.ok[chunk] | (1LLU << local);
}

static int fck_multidir_bit_is_ok(const fck_multidir *dir, const fckc_u8 subid)
{
	const fckc_u8 chunk = subid / fck_multidir_bitset_chunk_capacity;
	const fckc_u8 local = subid % fck_multidir_bitset_chunk_capacity;
	return (dir->common.ok[chunk] & (1LLU << local)) == (1LLU << local);
}

static void fck_multidir_bit_clear_ok(fck_multidir *dir, const fckc_u8 subid)
{
	const fckc_u8 chunk = subid / fck_multidir_bitset_chunk_capacity;
	const fckc_u8 local = subid % fck_multidir_bitset_chunk_capacity;
	dir->common.ok[chunk] = dir->common.ok[chunk] & ~(1LLU << local);
}

static int fck_multidir_empty(fck_multidir *dir)
{
	for (fckc_size_t chunk = 0; chunk < fck_multidir_bitset_chunk_capacity; chunk++)
	{
		if (dir->common.ok[chunk])
		{
			return 0;
		}
	}
	return 1;
}

static fck_db_element *fck_multidir_add_entry(fck_db_header *header, fck_multidir *root, fck_db_id id, const char *name,
                                              fck_db_element *element)
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
	// We now know current is a leaf!
	current->entry.element = element;
	current->entry.name = name;

	for (fckc_size_t index = 0; index < indirections; index++)
	{
		const fckc_u8 subid = e[index];
		fck_multidir *dir = parents[index];
		fck_multidir_bit_set_ok(dir, subid);
	}

	return current->entry.element;
}

static fck_db_element *fck_multidir_remove_entry(fck_db_header *header, fck_multidir *root, fck_db_id id)
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
	// We now know current is a leaf!
	fck_db_element *stored = current->entry.element;

	for (fckc_size_t index = 0; index < indirections; index++)
	{
		const fckc_u8 subid = e[index];
		fck_multidir *dir = parents[index];
		fck_multidir_bit_clear_ok(dir, subid);
	}

	for (fckc_size_t index = 0; index < indirections; index++)
	{
		const fckc_size_t inverse = indirections - index - 1;
		fck_multidir *dir = parents[inverse];
		if (fck_multidir_empty(dir))
		{
			kll_free(allocator, dir->dir.children);
		}
	}

	return stored;
}

static void fck_multidir_iterate(fck_multidir *root)
{
	int iterations = 0;
	fckc_u8 e[4] = {0};
	fck_multidir *l[4] = {0};
	for (e[0] = 0; e[0] < 255; e[0]++)
	{
		if (!fck_multidir_bit_is_ok(root, e[0]))
		{
			continue;
		}
		l[0] = root->dir.children + e[0];
		for (e[1] = 0; e[1] < 255; e[1]++)
		{
			if (!fck_multidir_bit_is_ok(l[0], e[1]))
			{
				continue;
			}
			l[1] = l[0]->dir.children + e[1];
			for (e[2] = 0; e[2] < 255; e[2]++)
			{
				if (!fck_multidir_bit_is_ok(l[1], e[2]))
				{
					continue;
				}
				l[2] = l[1]->dir.children + e[2];
				for (e[3] = 0; e[3] < 255; e[3]++)
				{
					if (!fck_multidir_bit_is_ok(l[2], e[3]))
					{
						continue;
					}
					iterations = iterations + 1;
					l[3] = l[2]->dir.children + e[3];
					os->io->log(l[3]->entry.name);
				}
			}
		}
	}

	os->io->log("Iterations: %d", iterations);
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
			fck_assert(result);
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

static const char *fck_db_make_meta_path(char *buffer, fckc_size_t size, const char *dir, const char *path)
{
	if (!buffer || size == 0 || !dir || !path)
	{
		return NULL;
	}

	const fckc_size_t dir_len = strlen(dir);
	const int need_separator = (dir_len > 0 && dir[dir_len - 1] != '/' && path[0] != '/');
	const int written = snprintf(buffer, size, need_separator ? "%s/%s.%s" : "%s%s.%s", dir, path, fck_db_item_meta_extension);

	if (written < 0 || (fckc_size_t)written >= size)
	{
		return NULL;
	}

	char *path_start = buffer + dir_len + (need_separator ? 1 : 0);
	char *ext_start = strrchr(path_start, '.');

	for (char *p = path_start; p < ext_start; p++)
	{
		if (*p == '/' || *p == '\\')
		{
			*p = '.';
		}
	}

	return buffer;
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

static const char *fck_db_make_database_path(char *buffer, fckc_size_t size, const char *path)
{
	if (!buffer || size == 0 || !path)
	{
		return NULL;
	}

	const int written = snprintf(buffer, size, "%s.%s", path, fck_db_path_extension);
	if (written < 0 || (fckc_size_t)written >= size)
	{
		return NULL;
	}
	return buffer;
}

static void fck_db_api_import_file(fck_db external, fck_db_section *section, const char *relative)
{
	fck_db_private *db = external.opaque;

	kll_arena *temp = kll->arena->create(db->allocator, 512);
	const char *ext = fck_db_api_file_extension(relative);
	fck_assert(ext);
	if (strcmp(ext, fck_db_item_meta_extension) == 0)
	{
		return;
	}

	char absolute_buffer[1024];
	const char *absolute = fck_db_make_full_path(absolute_buffer, fck_arraysize(absolute_buffer), section->path, relative);
	if (absolute == NULL)
	{
		return;
	}
	temp->reset(temp);

	fck_db_loader_interface null_loader = {.name = "none", .type = to_u16(~0)};
	fck_db_loader_interface *loader = fck_db_ext_map_find(&db->loaders, ext);
	fck_db_element *payload;
	if (loader)
	{
		payload = loader->import(apis, absolute);
	}
	else
	{
		const fck_file file = os->fs->open(absolute, "r");
		fck_assert(os->fs->is_valid(file));
		const fckc_size_t size = os->fs->size(file);
		payload = kll_malloc(db->allocator, size);
		os->fs->read(file, payload, size);
		os->fs->close(file);

		loader = &null_loader;
		os->io->log("No loader for: %s (%s)", ext, absolute);
	}

	char meta_buffer[1024];
	const char *meta = fck_db_make_meta_path(meta_buffer, fck_arraysize(meta_buffer), section->database_path, relative);

	// try red
	int loaded = 0;
	fck_file file = os->fs->open(meta, "r");
	if (os->fs->is_valid(file))
	{
		const fckc_i64 size = os->fs->size(file);
		char *buffer = (char *)kll_malloc(temp, size + 1);
		os->fs->read(file, buffer, size);
		buffer[size] = '\0';

		const fckc_size_t read = os->fs->read(file, (void *)buffer, size);
		fck_assert(read == 0);
		fck_serialiser *reader = serialiser_json->reader(db->allocator, buffer, size);
		fck_serialiser_element *query = reader->query(reader, "/path");

		if (query && query->type == fck_serialiser_string && query->count == 1)
		{
			fck_assert(strcmp(query->values->as_string, relative) == 0);

			query = reader->query(reader, "/loader/type");
			if (query && query->type == fck_serialiser_u64 && query->count == 1)
			{
				const fckc_u64 type = query->values->as_u64;
				if (type == loader->type)
				{
					query = reader->query(reader, "/loader/name");
					if (query && query->type == fck_serialiser_string && query->count == 1)
					{
						// I think for this, I should let it fall through and the name should just get updated?
						if (strcmp(query->values->as_string, loader->name) == 0)
						{
							loaded = 1;
						}
					}
				}
			}
		}
		os->fs->close(file);
	}

	if (loaded == 0)
	{
		fck_serialiser *writer = serialiser_json->writer(db->allocator);

		const fckc_u64 time = to_u64(os->chrono->now());
		fck_serialiser_params params;
		params.name = "path";
		writer->string(writer, &params, (void **)&relative, 1);
		params.name = "loader";
		writer->push(writer, &params);
		params.name = "type";
		writer->u16(writer, &params, &loader->type, 1);
		params.name = "name";
		writer->string(writer, &params, (void **)&loader->name, 1);
		writer->pop(writer);

		const void *buffer = writer->buffer(writer);
		const fckc_size_t size = writer->at(writer);

		file = os->fs->open(meta, "w+");
		fck_assert(os->fs->is_valid(file));
		os->fs->write(file, buffer, size);
		os->fs->close(file);

		writer->destroy(writer);
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
	fck_multidir_add_entry(&db->database.header, &db->database.root, id, relative_path, payload);
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
	fck_db_private *db = (fck_db_private *)external.opaque;
	fck_multidir_remove_entry(&db->database.header, &db->database.root, id);
	char buffer[1024];

	const char *full_path = fck_db_make_full_path(buffer, fck_arraysize(buffer), section->path, relative);
	const char *meta = fck_db_make_meta_path(buffer, fck_arraysize(buffer), section->database_path, relative);
	if (meta)
	{
		os->fs->remove(meta);
	}
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

			for (fckc_size_t index = 0; index < result; index++)
			{
				fck_file_watcher_event *change = changes + index;
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
					fck_multidir_remove_entry(&db->database.header, &db->database.root, id);
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
	const char *database = fck_db_make_database_path(section->database_path, fck_arraysize(section->database_path), path);
	fck_assert(database);

	os->fs->create_directory(database);
}

static void fck_db_api_import_directory(fck_db external, const char *scope, const char *path)
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

static fck_db_element *fck_db_api_get(fck_db external, const char *path)
{
	fck_db_private *db = (fck_db_private *)external.opaque;
	const fck_db_id id = fck_db_api_id_from_path(external, path);
	return fck_multidir_resolve(&db->database.root, id);
}

static fck_db fck_db_api_create(kll_allocator *allocator, const char *path)
{
	fck_db_private *db = (fck_db_private *)kll_malloc(allocator, sizeof(*db));
	memset(db, 0, sizeof(*db));
	const fck_db result = {.opaque = db};
	db->allocator = allocator;
	db->strings = kll->arena->create(allocator, 512);
	db->database.header.allocator = allocator;
	db->loaders = fck_db_ext_map_create(allocator);

	fck_db_api_import_directory(result, "app", path);

	return result;
}

static void fck_db_api_close(fck_db db)
{
	fck_db_ext_map_destroy(db.opaque->allocator, &db.opaque->loaders);
	kll_free(db.opaque->allocator, db.opaque);
}

static fck_db_api db_api = {
	.create = fck_db_api_create,
	.hotreload = fck_db_api_hotreload,
	.close = fck_db_api_close,
	.get = fck_db_api_get,
};

static fck_db_element *fck_directory_import(fck_api_registry *registry, const char *path)
{
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

fck_db_api *fck_db_load(fck_api_registry *registry, void *old)
{
	apis = registry;
	apis->add(fck_db_api_name, &db_api);
	apis->add(fck_db_loader_interface_name, &directory_loader);

	return &db_api;
}