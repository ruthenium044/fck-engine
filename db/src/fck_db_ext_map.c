#include "fck_db_ext_map.h"

#include "fck_db.h"
#include "fck_db_core.inl"

#include <fck_apis.h>

#include <fck_hash.h>

#include <kll.h>
#include <kll_malloc.h>

#include <fckc_assert.h>

#include <string.h>

typedef struct fck_db_ext_map
{
	fckc_size_t capacity;
	fck_db_ext_map_entry entries[1];
} fck_db_ext_map;

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
static fck_db_ext_map *fck_db_ext_map_alloc(kll_allocator *allocator, fck_api_registry *apis)
{
	// fck_db_ext_map map = {0};

	void **asset_loaders;
	const fckc_size_t asset_loaders_count = apis->implementations(fck_db_loader_interface_name, &asset_loaders);

	fckc_size_t capacity = 0;
	for (fckc_size_t index = 0; index < asset_loaders_count; index++)
	{
		fck_db_loader_interface *loader = (fck_db_loader_interface *)asset_loaders[index];
		const char **extensions;
		const fckc_size_t count = loader->supports(&extensions);
		capacity = capacity + count;
	}

	fck_db_ext_map *map;
	capacity = capacity * 2;
	const fckc_size_t total = offsetof(fck_db_ext_map, entries[capacity]);
	map = (fck_db_ext_map *)kll_malloc(allocator, total);
	memset(map, 0, total);
	map->capacity = capacity;

	for (fckc_size_t index = 0; index < asset_loaders_count; index++)
	{
		fck_db_loader_interface *loader = (fck_db_loader_interface *)asset_loaders[index];
		const char **extensions;
		const fckc_size_t count = loader->supports(&extensions);
		for (fckc_size_t ext_index = 0; ext_index < count; ext_index++)
		{
			const char *ext = extensions[ext_index];
			const int result = fck_db_ext_map_add(map, ext, loader);
			fck_assert(result && "Trying to apply duplicated loader or out of space!");
		}
	}

	return map;
}
// ext_map
static void fck_db_ext_map_free(kll_allocator *allocator, fck_db_ext_map *map)
{
	kll_free(allocator, map);
}

static fck_db_ext_map_api db_ext_map_api = {
	.find = fck_db_ext_map_find,
	.add = fck_db_ext_map_add,
	.alloc = fck_db_ext_map_alloc,
	.free = fck_db_ext_map_free,
};

fck_db_ext_map_api *db_ext_map = &db_ext_map_api;
