
#include <fck_apis.h>
#include <fck_hash.h>
#include <fck_input.h>
#include <fck_os.h>
#include <fck_pkey.h>
#include <fckc_assert.h>

#include <fckc_inttypes.h>

#include <stdio.h>

#include <stdlib.h>
#include <string.h>

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

#define plugins_hashmap_capacity 64

typedef struct plugins_hashmap_entry
{
	char path[420];

	fck_shared_object shared_object;
	void *implementation;

	fckc_i64 modified;
} plugins_hashmap_entry;

typedef struct plugins_hashmap
{
	plugins_hashmap_entry entries[plugins_hashmap_capacity];
} plugins_hashmap;

static fckc_size_t plugins_hashmap_add(plugins_hashmap *map, const char *path)
{
	const fck_hash_int hash = fck_hash(path, strlen(path));
	const fckc_size_t capacity = fck_arraysize(map->entries);
	fckc_size_t slot = hash % capacity;

	for (fckc_size_t index = 0; index < capacity; index++)
	{
		plugins_hashmap_entry *entry = map->entries + slot;
		if (entry->path[0] == '\0')
		{
			const int len = strlen(path);
			memcpy(entry->path, path, len);
			entry->modified = 0;
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

static fckc_size_t plugins_hashmap_clear(plugins_hashmap *map)
{
	for (fckc_size_t index = 0; index < fck_arraysize(map->entries); index++)
	{
		plugins_hashmap_entry *entry = map->entries + index;
		if (os->so->is_valid(entry->shared_object))
		{
			os->io->log("Unloaded Plugin: %.*s", strlen(entry->path), entry->path);
			os->so->unload(entry->shared_object);
		}
	}
	memset(map, 0, sizeof(*map));
	return 0;
}

static void *load_plugin(plugins_hashmap *map, fck_api_registry *registry, const char *target)
{
	char path_buffer[1024];

	char *api = os->glob->match(target, "fck-*.dll");
	if (api)
	{
		const fckc_size_t result = plugins_hashmap_add(map, target);
		if (!result)
		{
			return NULL;
		}

		const fckc_i64 modified = os->fs->modified(target);

		plugins_hashmap_entry *entry = map->entries + result - 1;
		if (entry->modified >= modified)
		{
			return NULL;
		}

		// Add it if newer...
		entry->modified = modified;

		const char *path = entry->path;
		const char *so_load_path = entry->path;
		{
			const fck_file so_file = os->fs->open(path, "r");
			const fckc_i64 size = os->fs->size(so_file);
			void *mem = malloc(size);
			fckc_size_t result = os->fs->read(so_file, mem, size);
			fck_assert(result == size);
			os->fs->close(so_file);

			snprintf(path_buffer, sizeof(path_buffer), "temp-(%lld)-%s", modified, path);
			const fck_file temp_file = os->fs->open(path_buffer, "w");
			result = os->fs->write(temp_file, mem, size);
			fck_assert(result == size);
			os->fs->close(temp_file);
			free(mem);
			so_load_path = path_buffer;
		}

		const fck_shared_object so = os->so->load(so_load_path);

		if (os->so->is_valid(so))
		{
			char *extension = os->glob->find(path, ".dll");
			const fckc_size_t length = (fckc_size_t)extension - (fckc_size_t)path;

			char buffer[1024];

			const int result = snprintf(buffer, sizeof(buffer), "%.*s_load", (int)length, path);
			(void)result;

			char *loadable = dashes_to_underscores(buffer, length);
			void *symbol = os->so->symbol(so, loadable);
			if (!symbol)
			{
				memset(entry->path, 0, fck_arraysize(entry->path));
				os->so->unload(so);
				return NULL;
			}

			fck_main_func *load = (fck_main_func *)symbol;
			// If the returned API is a value equal to the old one (which can be NULL)
			// We decide that we unload what we just loaded because we failed hot-reloading
			void *api = load(registry, entry->implementation);
			if (api != entry->implementation)
			{
				if (os->so->is_valid(entry->shared_object))
				{
					// Remove old implementation from registry...
					const char* name = registry->nameof(entry->implementation);
					registry->remove(name, entry->implementation);

					os->io->log("Unloaded old Plugin: %.*s", strlen(entry->path), entry->path);
					os->so->unload(entry->shared_object);
				}
				entry->shared_object = so;
				entry->implementation = api;
				os->io->log("Loaded Plugin: %.*s", strlen(entry->path), entry->path);
				return api;
			}

			memset(entry->path, 0, fck_arraysize(entry->path));
			os->so->unload(so);
			return NULL;
		}
		return NULL;
	}

	return NULL;
}

static void purge_temporary_files(void)
{
	char** paths;
	const fckc_size_t count = os->glob->local("", "temp-*.dll", &paths);
	for (fckc_size_t index = 0; index < count; index++)
	{
		char* path = paths[index];
		os->io->log("Purge Temporary File: %.*s", strlen(path), path);
		os->fs->remove(path);
	}
	os->glob->free(paths);
}

static void load_plugins_all(plugins_hashmap* map, fck_api_registry *registry)
{
	char **paths;
	const fckc_size_t count = os->glob->local("", "*.dll", &paths);
	for (fckc_size_t index = 0; index < count; index++)
	{
		char *path = paths[index];
		load_plugin(map, registry, path);
	}
	os->glob->free(paths);
}

static plugins_hashmap plugin_map;

int main(int argc, char **argv)
{
	purge_temporary_files();

	// Dynamically available for convenience!
	// apis
	// os
	fck_api_registry *registry = (fck_api_registry *)load_plugin(&plugin_map, NULL, "fck-api.dll");
	load_plugins_all(&plugin_map, registry);

	// Other stuff has to get loaded and registered?
	const fck_window window = os->win->create("Test", 1920, 1080);

	fck_input *input = (fck_input *)registry->find(fck_input_api_name);

	int is_running = 1;
	while (is_running)
	{
		// TODO: Make file watcher!
		load_plugins_all(&plugin_map, registry);
		fck_input_event events[32] = {0};
		const fckc_size_t result = input->events(events, fck_arraysize(events));
		for (fckc_size_t index = 0; index < result; index++)
		{
			fck_input_event *e = events + index;
			if (e->source->type == fck_input_source_keyboard)
			{
				if (e->description->id == fck_pkey_escape)
				{
					is_running = 0;
				}
			}

			os->io->log("%s - %llu - %u \t %s - %s: %f %f", e->source->name, e->owner, e->description->id, e->description->name,
			            fck_input_data_type_to_string(e->description->data_type), e->data.floats[0], e->data.floats[1]);
		}
	}

	plugins_hashmap_clear(&plugin_map);
	purge_temporary_files();

	os->win->destroy(window);

	return 0;
}
