
#include <fck_plugins.h>

#include <fck_apis.h>
#include <fck_hash.h>
#include <fck_os.h>
#include <fckc_apidef.h>
#include <fckc_assert.h>
#include <fckc_inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define fck_plugins_hashmap_capacity 128

// TODO : On UNIX we DO NOT NEED THE TEMP COPY!
typedef struct fck_plugins_hashmap_entry
{
	char path[424];

	fckc_i64 modified;

	fck_shared_object shared_object;
	void             *implementation;
} fck_plugins_hashmap_entry;

typedef struct fck_plugins_hashmap
{
	fck_plugins_hashmap_entry entries[fck_plugins_hashmap_capacity];
} fck_plugins_hashmap;

static fckc_u32         plugin_hotreload_generation;
static char             plugin_root[2048]; // whatever
static fck_file_watcher plugin_watcher;

static fck_plugins_hashmap plugin_map;
static fck_api_registry   *apis;

static char *dashes_to_underscores(char *str, fckc_size_t length)
{
	for (fckc_size_t index = 0; index < length; index++)
	{
		if (str[index] == '-')
		{
			str[index] = '_';
		}
	}
	return str;
}

static fckc_size_t fck_plugins_hashmap_add(fck_plugins_hashmap *map, const char *path)
{
	const fck_hash_int hash     = fck_hash(path, strlen(path));
	const fckc_size_t  capacity = fck_arraysize(map->entries);
	fckc_size_t        slot     = hash % capacity;

	for (fckc_size_t index = 0; index < capacity; index++)
	{
		fck_plugins_hashmap_entry *entry = map->entries + slot;
		if (entry->path[0] == '\0')
		{
			const int len = strlen(path);
			memcpy(entry->path, path, len);
			entry->modified             = 0;
			entry->implementation       = NULL;
			entry->shared_object.handle = NULL;
			return slot + 1;
		}

		if (strcmp(entry->path, path) == 0)
		{
			return slot + 1;
		}

		slot = (slot + 1) % capacity;
	}
	return 0;
}

static fckc_size_t fck_plugins_hashmap_find(fck_plugins_hashmap *map, const char *path)
{
	const fck_hash_int hash     = fck_hash(path, strlen(path));
	const fckc_size_t  capacity = fck_arraysize(map->entries);
	fckc_size_t        slot     = hash % capacity;

	for (fckc_size_t index = 0; index < capacity; index++)
	{
		fck_plugins_hashmap_entry *entry = map->entries + slot;
		if (entry->path[0] == '\0')
		{
			return 0;
		}

		if (strcmp(entry->path, path) == 0)
		{
			return slot + 1;
		}

		slot = (slot + 1) % capacity;
	}
	return 0;
}

static const char *fck_temporary_shared_object_name(const char *path, fckc_i64 salt, char *buffer, fckc_size_t buffer_size)
{
	const int result = snprintf(buffer, buffer_size, "temp-%s-(%lld)", path, salt);
	if (result > 0)
	{
		return buffer;
	}
	return NULL;
}

static const char *fck_plugin_create_temp_dll(const char *path, fckc_i64 salt, char *buffer, fckc_size_t buffer_size)
{
	const fck_file so_file = os->fs->open(path, "rb");
	const fckc_i64 size    = os->fs->size(so_file);
	void          *mem     = malloc(size);
	fckc_size_t    result  = os->fs->read(so_file, mem, size);
	fck_assert(result == size);
	os->fs->close(so_file);

	// We can fail here when we already loaded something... Maybe we can check first?
	const char    *temp_path = fck_temporary_shared_object_name(path, salt, buffer, buffer_size);
	const fck_file temp_file = os->fs->open(temp_path, "wb");
	result                   = os->fs->write(temp_file, mem, size);
	fck_assert(result == size);
	os->fs->close(temp_file);
	free(mem);

	return temp_path;
}

// Ye, this is shit lmao
static fckc_size_t fck_plugin_cache_newest_shared_library(fck_plugins_hashmap *map, const char *target)
{
	char *api = os->glob->match(target, "fck-*" fck_plugin_extension);
	if (api)
	{
		// Hardcoded filter for these two
		if (strcmp("fck-api" fck_plugin_extension, target) == 0 || //
		    strcmp("fck-plugins" fck_plugin_extension, target) == 0)
		{
			return 0;
		}

		const fckc_size_t result = fck_plugins_hashmap_add(map, target);
		if (!result)
		{
			return 0;
		}

		fck_plugins_hashmap_entry *entry = map->entries + result - 1;

		fck_path_info info;
		const int     info_result = os->fs->info(target, &info);
		if (info_result && info.modified > entry->modified || !os->so->is_ok(entry->shared_object))
		{
			entry->modified = info.modified;
			return result;
		}
		return 0;
	}
	return 0;
}

static void fck_purge_temporary_file(fck_plugins_hashmap_entry *entry)
{
	char        buffer[1024];
	const char *path = fck_temporary_shared_object_name(entry->path, entry->modified, buffer, sizeof(buffer));

	os->io->log("Purge Temporary File: %.*s", strlen(path), path);
	os->fs->remove(path);
}

static void *fck_plugin_load_shared_library(fck_plugins_hashmap *map, fck_api_registry *registry, const char *target)
{
	char path_buffer[1024];

	const fckc_size_t result = fck_plugin_cache_newest_shared_library(map, target);
	if (!result)
	{
		return NULL;
	}

	fck_plugins_hashmap_entry *entry = map->entries + result - 1;

	const char *path         = entry->path;
	// #if defined(_WIN32) || defined(_WIN64)
	const char *so_load_path = fck_plugin_create_temp_dll(entry->path, entry->modified, path_buffer, sizeof(path_buffer));
	// #else
	//	const char *so_load_path = entry->path;
	// #endif

	const fck_shared_object so = os->so->load(so_load_path);

	if (os->so->is_ok(so))
	{
		char             *extension = os->glob->find(path, fck_plugin_extension);
		const fckc_size_t length    = (fckc_size_t)extension - (fckc_size_t)path;

		char buffer[1024];

		const int result = snprintf(buffer, sizeof(buffer), "%.*s_load", (int)length, path);
		(void)result;

		char *loadable = dashes_to_underscores(buffer, length);
		void *symbol   = os->so->symbol(so, loadable);
		if (!symbol)
		{
			os->io->log("Load function (%.*s) not found", result, buffer);
			fck_purge_temporary_file(entry);
			os->so->unload(so);
			return NULL;
		}
		fck_load_func *load = (fck_load_func *)symbol;
		// If the returned API is a value equal to the old one (which can be NULL)
		// We decide that we unload what we just loaded because we failed hot-reloading
		void          *api  = load(registry, entry->implementation);
		if (api != entry->implementation)
		{
			if (os->so->is_ok(entry->shared_object))
			{
				// If we had an SO lying around, it is now invalid, so we increment the
				// generation counter - Maybe never unloading is way better...
				if (registry)
				{
					// Remove old implementation from registry...
					const char *name = registry->nameof(entry->implementation);
					registry->remove(name, entry->implementation);
				}

				os->io->log("Unloaded old Plugin: %.*s", strlen(entry->path), entry->path);
				os->so->unload(entry->shared_object);
			}
			entry->shared_object  = so;
			entry->implementation = api;
			os->io->log("Loaded Plugin: %.*s as %.*s", strlen(entry->path), entry->path, strlen(so_load_path), so_load_path);
			return api;
		}
		fck_purge_temporary_file(entry);
		os->so->unload(so);
		return NULL;
	}
	return NULL;
}

static void *fck_plugins_api_load(const char *path)
{
	void *plugin = fck_plugin_load_shared_library(&plugin_map, apis, path);
	return plugin;
}

static void fck_plugins_api_unload(const char *path)
{
	const fckc_size_t result = fck_plugins_hashmap_find(&plugin_map, path);
	if (!result)
	{
		return;
	}

	const fckc_size_t          index = result - 1;
	fck_plugins_hashmap_entry *entry = plugin_map.entries + index;
	if (!os->so->is_ok(entry->shared_object))
	{
		return;
	}
	os->so->unload(entry->shared_object);

	fck_purge_temporary_file(entry);

	if (apis)
	{
		entry->shared_object = (fck_shared_object){0};
		const char *name     = apis->nameof(entry->implementation);
		apis->remove(name, entry->implementation);
		entry->implementation = NULL;
	}
}

static fckc_u32 fck_plugins_api_hotreload(void)
{
	if (os->fw->is_valid(plugin_watcher))
	{
		const fckc_size_t iteration_limit = 64;
		for (fckc_size_t iterations = 0; iterations < iteration_limit; iterations++)
		{
			fck_file_watcher_event changes[16];
			const fckc_size_t      result = os->fw->changes(plugin_watcher, changes, fck_arraysize(changes));
			if (result == 0)
			{
				break;
			}

			for (fckc_size_t index = 0; index < result; index++)
			{
				fck_file_watcher_event *change = changes + index;
				switch ((fck_file_watcher_event_type)change->type)
				{
				case fck_file_unknown:
					os->io->log("Unknown: %s", change->path);
					break;
				case fck_file_deleted:
					// plugin_hotreload_generation = plugin_hotreload_generation + 1;
					//  os->io->log("Deleted: %s", change->path);
					break;
				case fck_file_modified:
					// plugin_hotreload_generation = plugin_hotreload_generation + 1;
					//  os->io->log("Modified: %s", change->path);
					break;
				case fck_file_created: {
					// os->io->log("Created: %s", change->path);
					const fckc_size_t result = fck_plugin_cache_newest_shared_library(&plugin_map, change->path);
					if (result)
					{
						plugin_hotreload_generation = plugin_hotreload_generation + 1;

						// We ONLY load plugins that have been loaded explicitly
						os->io->log("Created: %s", change->path);
						fck_plugins_hashmap_entry *entry = plugin_map.entries + result - 1;
						entry->modified                  = entry->modified - 1; // Little hack ;)
						if (os->so->is_ok(entry->shared_object))
						{
							fck_plugins_api_load(change->path);
						}
					}
					break;
				}
				}
			}
		}
	}
	return plugin_hotreload_generation;
}

static void fck_plugins_api_root(const char *path)
{
	const int len = strlen(path);
	memcpy(plugin_root, path, strlen(path));
	plugin_root[len] = '\0';

	if (os->fw->is_valid(plugin_watcher))
	{
		os->fw->destroy(plugin_watcher);
	}

	char            **paths;
	const fckc_size_t results = os->glob->directory(path, "*" fck_plugin_extension, &paths);
	for (fckc_size_t index = 0; index < results; index++)
	{
		const char *path = paths[index];
		(void)fck_plugin_cache_newest_shared_library(&plugin_map, path);
	}
	os->glob->free(paths);

	plugin_watcher = os->fw->create(plugin_root);
}

static const char *fck_plugins_loaded(const char *prev)
{
	fckc_size_t index = 0;
	if (prev != NULL)
	{
		const fckc_size_t root   = to_size_t(&plugin_map.entries[0]);
		const fckc_size_t at     = to_size_t(prev);
		const fckc_size_t offset = (at - offsetof(fck_plugins_hashmap_entry, path)) - root;

		index = (offset / sizeof(plugin_map.entries[0])) + 1;
	}

	for (; index < fck_arraysize(plugin_map.entries); index++)
	{
		fck_plugins_hashmap_entry *entry = plugin_map.entries + index;
		if (os->so->is_ok(entry->shared_object))
		{
			return entry->path;
		}
	}
	return NULL;
}

static const char *fck_plugins_unloaded(const char *prev)
{
	fckc_size_t index = 0;
	if (prev != NULL)
	{
		const fckc_size_t root   = to_size_t(&plugin_map.entries[0]);
		const fckc_size_t at     = to_size_t(prev);
		const fckc_size_t offset = (at - offsetof(fck_plugins_hashmap_entry, path)) - root;

		index = (offset / sizeof(plugin_map.entries[0])) + 1;
	}

	for (; index < fck_arraysize(plugin_map.entries); index++)
	{
		fck_plugins_hashmap_entry *entry = plugin_map.entries + index;
		if (entry->path[0] != '\0' && !os->so->is_ok(entry->shared_object))
		{
			return entry->path;
		}
	}
	return NULL;
}

static const fck_shared_object *fck_plugins_api_so(const char *path)
{
	const fckc_size_t result = fck_plugins_hashmap_find(&plugin_map, path);
	if (result)
	{
		const fck_plugins_hashmap_entry *entry = plugin_map.entries + result - 1;
		return &entry->shared_object;
	}
	return NULL;
};

static void fck_plugins_api_shutdown(void);

static fck_plugins_api plugin_api = {
	.hotreload = fck_plugins_api_hotreload,

	.loaded   = fck_plugins_loaded,
	.unloaded = fck_plugins_unloaded,

	.root     = fck_plugins_api_root,
	.load     = fck_plugins_api_load,
	.unload   = fck_plugins_api_unload,
	.shutdown = fck_plugins_api_shutdown,
	.so       = fck_plugins_api_so,
};

void fck_plugins_api_shutdown(void)
{
	for (fckc_size_t index = 0; index < fck_arraysize(plugin_map.entries); index++)
	{
		fck_plugins_hashmap_entry *entry = plugin_map.entries + index;
		if (os->so->is_ok(entry->shared_object))
		{
			os->io->log("Unloaded Plugin: %.*s", strlen(entry->path), entry->path);
			os->so->unload(entry->shared_object);

			const char *name = apis->nameof(entry->implementation);
			if (name)
			{
				os->io->log("Plugin API removed from API Registry: %.*s", strlen(name), name);
				apis->remove(name, entry->implementation);
			}
			else
			{
				os->io->log("Could not remove Plugin API from API Reigstry: %.*s", strlen(entry->path), entry->path);
			}

			fck_purge_temporary_file(entry);
		}
	}
	memset(&plugin_map, 0, sizeof(plugin_map));

	apis->remove(fck_plugins_api_name, &plugin_api);
}

FCK_EXPORT_API void *fck_plugins_load(fck_api_registry *registry, void *old)
{
	(void)old;
	apis = registry;

	if (registry)
	{
		registry->add(fck_plugins_api_name, &plugin_api);
	}

	return &plugin_api;
}
