
#include <fck_apis.h>
#include <fck_hash.h>
#include <fck_input.h>
#include <fck_os.h>
#include <fck_pkey.h>
#include <fck_plugins.h>
#include <fckc_assert.h>

#include <fckc_inttypes.h>

#include <stdio.h>

#include <stdlib.h>
#include <string.h>

static void purge_files(const char *pattern)
{
	char **paths;
	const fckc_size_t count = os->glob->executable("", pattern, &paths);
	for (fckc_size_t index = 0; index < count; index++)
	{
		char *path = paths[index];
		os->io->log("Purge Temporary File: %.*s", strlen(path), path);
		os->fs->remove(path);
	}
	os->glob->free(paths);
}

static fck_api_registry *fck_api_gegistry_load(const char *path)
{
	// This badboy needs to get released
	const fck_shared_object so = os->so->load(path);
	fck_load_func *loader = (fck_load_func *)os->so->symbol(so, "fck_api_load");
	fck_api_registry *registry = (fck_api_registry *)loader(NULL, NULL);
	return registry;
}

static fck_plugins_api *fck_plugins_load(fck_api_registry *registry, const char *path)
{
	// This badboy needs to get released
	const fck_shared_object so = os->so->load(path);
	fck_load_func *loader = (fck_load_func *)os->so->symbol(so, "fck_plugins_load");
	fck_plugins_api *plugins = (fck_plugins_api *)loader(registry, NULL);
	return plugins;
}

int main(int argc, char **argv)
{
	purge_files("temp-*.dll");

	fck_api_registry *registry = fck_api_gegistry_load("fck-api.dll");
	fck_plugins_api *plugins = fck_plugins_load(registry, "fck-plugins.dll");
	plugins->root(os->fs->executable());

	{
		char **paths;
		const fckc_size_t count = os->glob->executable("", "*.dll", &paths);
		for (fckc_size_t index = 0; index < count; index++)
		{
			char *path = paths[index];
			plugins->load(path);
		}
		os->glob->free(paths);
	}

	fckc_u32 hotreload_counter = plugins->hotreload();
	// const fck_file_watcher fw = os->fw->create(os->fs->executable());

	// Other stuff has to get loaded and registered?
	const fck_window window = os->win->create("Test", 1920, 1080);

	fck_input *input = (fck_input *)registry->find(fck_input_api_name);

	int is_running = 1;
	while (is_running)
	{
		fckc_u32 next_hotreload_counter = plugins->hotreload();
		if (next_hotreload_counter != hotreload_counter)
		{
			os->io->log("Something changed: %lu", next_hotreload_counter);
			hotreload_counter = next_hotreload_counter;
		}
		/*fck_file_watcher_event changes[4];
		fckc_size_t result = os->fw->changes(fw, changes, fck_arraysize(changes));
		for (fckc_size_t index = 0; index < result; index++)
		{
		    fck_file_watcher_event *change = changes + index;
		    switch ((fck_file_watcher_event_type)change->type)
		    {
		    case fck_file_modified:
		        os->io->log("Modified: %s", change->path);
		        break;
		    case fck_file_deleted:
		        os->io->log("Deleted: %s", change->path);
		        break;
		    case fck_file_created:
		        os->io->log("Created: %s", change->path);
		        plugins->load(change->path);
		        break;
		    case fck_file_unknown:
		        os->io->log("Unknown: %s", change->path);
		        break;
		    }
		}*/

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

			/*os->io->log("%s - %llu - %u \t %s - %s: %f %f", e->source->name, e->owner, e->description->id, e->description->name,
			            fck_input_data_type_to_string(e->description->data_type), e->data.floats[0], e->data.floats[1]);*/
		}
	}

	plugins->shutdown();
	purge_files("temp-*.dll");

	os->win->destroy(window);

	return 0;
}
