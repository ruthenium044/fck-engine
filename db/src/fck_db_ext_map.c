#include "fck_db_ext_map.h"

#include "fck_db.h"
#include "fck_db_core.inl"

#include <fck_apis.h>

#include <fck_hash.h>

#include <kll.h>
#include <kll_malloc.h>

#include <fckc_assert.h>

#include <string.h>

typedef struct fck_db_loader_entry_cache
{
	fck_db_asset_reference *references;
	fckc_size_t count;
	fckc_size_t capacity;
} fck_db_loader_entry_cache;

typedef struct fck_db_ext_map_entry
{
	const char *extension;
	fck_db_loader_interface *loader;
	fck_db_loader_entry_cache cache;
} fck_db_ext_map_entry;

typedef struct fck_db_ext_map
{
	fckc_size_t capacity;

	// Might be better out of here in a sperate object! :)
	fck_db_loader_interface **loaders;
	fckc_size_t loaders_count;

	fck_db_ext_map_entry entries[1];
} fck_db_ext_map;

static fck_db_asset_reference *fck_db_loader_entry_cache_exists(fck_db_loader_entry_cache *cache, const char *entry)
{
	for (fckc_size_t index = 0; index < cache->count; index++)
	{
		if (strcmp(entry, cache->references[index].path) == 0)
		{
			return cache->references + index;
		}
	}
	return NULL;
}

static fck_db_asset_reference *fck_db_loader_entry_cache_add(kll_allocator *allocator, fck_db_loader_entry_cache *cache, const char *entry)
{
	if (cache->count >= cache->capacity)
	{
		const fckc_size_t capacity = cache->count ? cache->capacity * 2 : 16;
		const fckc_size_t total = capacity * sizeof(*cache->references);
		fck_db_asset_reference *entries = (fck_db_asset_reference *)kll_malloc(allocator, total);
		if (cache->references)
		{
			memcpy(entries, cache->references, cache->count);
			kll_free(allocator, cache->references);
		}
		cache->capacity = capacity;
		cache->references = entries;
	}

	fck_db_asset_reference *at = cache->references + cache->count;
	memset(at, 0, sizeof(*at));
	at->path = entry;
	cache->count = cache->count + 1;
	return at;
}

static void fck_db_loader_entry_cache_free(kll_allocator *allocator, fck_db_loader_entry_cache *cache)
{
	kll_free(allocator, cache->references);
	cache->count = 0;
	cache->capacity = 0;
}

// ext_map
static fck_db_ext_map_entry *fck_db_ext_map_find_entry(fck_db_ext_map *map, const char *ext)
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
			return entry;
		}

		slot = (slot + 1) % capacity;
	}
	return NULL;
}

static fck_db_loader_interface *fck_db_ext_map_find(fck_db_ext_map *map, const char *ext)
{
	fck_db_ext_map_entry *entry = fck_db_ext_map_find_entry(map, ext);
	if (entry)
	{
		return entry->loader;
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
	// Banger cast
	map->loaders = (fck_db_loader_interface **)asset_loaders;
	map->loaders_count = asset_loaders_count;

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

static fck_db_asset_reference *fck_db_ext_map_cache_entry(kll_allocator *allocator, fck_db_ext_map *map, const char *ext, const char *path)
{
	fck_db_ext_map_entry *entry = fck_db_ext_map_find_entry(map, ext);
	fck_db_asset_reference *ref = fck_db_loader_entry_cache_exists(&entry->cache, path);
	if (ref == NULL)
	{
		return fck_db_loader_entry_cache_add(allocator, &entry->cache, path);
	}
	return ref;
}

static fckc_size_t fck_db_ext_map_cache_entries(fck_db_ext_map *map, const char *ext, const fck_db_asset_reference **references)
{
	fck_db_ext_map_entry *entry = fck_db_ext_map_find_entry(map, ext);
	*references = entry->cache.references;
	return entry->cache.count;
}

static fckc_size_t fck_db_ext_map_loaders(fck_db_ext_map *map, fck_db_loader_interface ***loaders)
{
	*loaders = map->loaders;
	return map->loaders_count;
}

// ext_map
static void fck_db_ext_map_free(kll_allocator *allocator, fck_db_ext_map *map)
{
	kll_free(allocator, map);
}

static fck_db_ext_map_api db_ext_map_api = {
	.alloc = fck_db_ext_map_alloc,
	.free = fck_db_ext_map_free,
	.find = fck_db_ext_map_find,
	.add = fck_db_ext_map_add,
	.cache = fck_db_ext_map_cache_entry,
	.listof = fck_db_ext_map_cache_entries,
	.loaders = fck_db_ext_map_loaders,
};

fck_db_ext_map_api *db_ext_map = &db_ext_map_api;
