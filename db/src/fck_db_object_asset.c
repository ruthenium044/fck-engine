
#include "fck_db.h"
#include "fck_db_core.inl"
#include "fck_db_ext_map.h"
#include "fck_db_object.h"

#include <kll.h>
#include <kll_malloc.h>

#include <fck_apis.h>

#include <fckc_assert.h>
#include <fckc_inttypes.h>

#include <fck_os.h>

#include <stdio.h>
#include <string.h>

typedef struct fck_db_asset_private
{
	fck_db_asset       base;
	fck_db_asset_state state;
} fck_db_asset_private;

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

static const char *fck_db_api_file_extension(const char *path)
{
	const char *dot = strrchr(path, '.');
	if (!dot || dot == path)
	{
		return "";
	}

	return dot + 1;
}

static fck_db_id fck_db_api_id_from_path(fck_db external, const char *path, fck_db_loader_interface **out_loader)
{
	fck_db_private *db  = (fck_db_private *)external.opaque;
	const char     *ext = fck_db_api_file_extension(path);

	char              base_path[1024];
	const char       *dot      = strrchr(path, '.');
	const fckc_size_t path_len = dot ? (fckc_size_t)(dot - path) : strlen(path);
	if (path_len >= sizeof(base_path))
	{
		return fck_db_id_make(255, 255, 255, 255, fck_db_type_object);
	}

	memcpy(base_path, path, path_len);
	base_path[path_len]             = '\0';
	fck_db_loader_interface *loader = db_ext_map->find(db->loaders, ext);
	if (!loader)
	{
		return fck_db_id_from_path(base_path, NULL);
	}
	const fck_db_id id = fck_db_id_from_path(base_path, loader->category);
	*out_loader        = loader;
	return id;
}

static const char *fck_db_api_make_scope_path(char *buffer, fckc_size_t size, const char *scope, const char *relative)
{
	const fckc_size_t scope_len    = strlen(scope);
	const fckc_size_t relative_len = strlen(relative);
	const fckc_size_t total_len    = scope_len + 1 + relative_len + 1;
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

static const fck_db_asset *fck_db_asset_api_get(fck_db external, fck_db_id id, const char *category)
{
	fck_db_private *db    = (fck_db_private *)external.opaque;
	fck_db_object  *entry = fck_db_resolve_object(db->page_table, id);
	if (entry)
	{
		// TODO: Internally, in db, we should use the same code-paths as accessor!!!
		const fck_db_accessor stable_reader = db_object->read(external, id);
		// fck_db_asset *asset = accessor.read->asset(accessor, "asset");
		if (stable_reader.ok->reference(stable_reader, "runtime"))
		{
			const fck_db_id       runtime        = stable_reader.read->reference(stable_reader, "runtime");
			const fck_db_accessor runtime_reader = db_object->read(external, runtime);

			const fck_db_asset *info = (const fck_db_asset *)runtime_reader.read->userdata(runtime_reader, "asset");
			if (strcmp(info->category, category) == 0)
			{
				return info;
			}
			// Let's pray I am good enough that this path never happens
			const char *fmt = "ERROR: Found asset is in wrong category! (Found: %s - Request: %s)";
			os->io->log(fmt, info->category, category);

			return NULL;
		}
		return NULL;
	}
	return NULL;
}

static const fck_db_asset *fck_db_asset_api_find(fck_db external, const char *path)
{
	fck_db_loader_interface *loader = NULL;
	const fck_db_id          id     = fck_db_api_id_from_path(external, path, &loader);
	if (fck_db_id_ok(id))
	{
		const char *category = loader ? loader->category : "none";
		return fck_db_asset_api_get(external, id, category);
	}
	return NULL;
}

static const fck_db_asset *fck_db_asset_api_lazy(fck_db db, const char *path, const char *category)
{
	const fck_db_id id = fck_db_id_from_path(path, category);
	if (fck_db_id_ok(id))
	{
		const fck_db_asset *asset = fck_db_asset_api_get(db, id, category);
		if (asset)
		{
			return asset;
		}
	}
	return NULL;
}

static const char *fck_db_make_full_path(char *buffer, fckc_size_t size, const char *dir, const char *path)
{
	if (!buffer || size == 0 || !dir || !path)
	{
		return NULL;
	}

	const fckc_size_t dir_len        = strlen(dir);
	const int         need_separator = (dir_len > 0 && dir[dir_len - 1] != '/' && path[0] != '/');
	const int         written        = snprintf(buffer, size, need_separator ? "%s/%s" : "%s%s", dir, path);
	if (written < 0 || (fckc_size_t)written >= size)
	{
		return NULL;
	}
	return buffer;
}

static fck_db_asset *fck_db_api_import_file(fck_db external, fck_db_section *section, const char *relative)
{
	fck_db_private *db = external.opaque;

	kll_arena  *temp = kll->arena->create(db->allocator, 512);
	const char *ext  = fck_db_api_file_extension(relative);
	fck_assert(ext);

	char        absolute_buffer[1024];
	const char *absolute = fck_db_make_full_path(absolute_buffer, fck_arraysize(absolute_buffer), section->path, relative);
	if (absolute == NULL)
	{
		return NULL;
	}
	temp->reset(temp);

	fck_db_loader_interface  null_loader = {.category = "none"};
	fck_db_loader_interface *loader      = db_ext_map->find(db->loaders, ext);
	if (!loader)
	{
		loader = &null_loader;
	}

	const fckc_size_t scope_len = strlen(section->scope) + 1; // for / separator
	fckc_size_t       total_len;
	char             *relative_path;
	if (ext[0] != '\0')
	{
		const fckc_size_t len = to_size_t(ext - relative); // We add a dot somewhere above
		total_len             = len + scope_len;
		relative_path         = (char *)kll_malloc(db->strings, total_len);
		memcpy(relative_path, section->scope, scope_len);
		char *dst    = (char *)memcpy(relative_path + scope_len, relative, len);
		dst[len - 1] = '\0';
	}
	else
	{
		const fckc_size_t len = strlen(relative) + 1;
		total_len             = len + scope_len;
		relative_path         = (char *)kll_malloc(db->strings, total_len);
		memcpy(relative_path, section->scope, scope_len);
		memcpy(relative_path + scope_len, relative, len);
	}
	relative_path[scope_len - 1] = '/';

	for (fckc_size_t index = 0; index < total_len; index++)
	{
		if (relative_path[index] == '\\')
		{
			relative_path[index] = '/';
		}
	}

	if (loader->import)
	{
		const fck_db_id      blob  = fck_db_id_from_path(relative_path, loader->category);
		const fck_db_object *entry = fck_db_ensure_object(db->page_table, blob);

		fck_db_id runtime = {0};

		fck_db_api *db_api = (fck_db_api *)db->registry->find(fck_db_api_name);

		const fck_db_loader_args args = {
			.api      = db_api,
			.registry = db->registry,
			.db       = external,
			.target   = blob,
		};

		void *userdata = loader->import(&args, absolute);

		const fck_db_accessor reader = db_object->read(external, blob);
		if (reader.ok->reference(reader, "runtime"))
		{
			runtime = reader.read->reference(reader, "runtime");
		}
		else
		{
			runtime = db_object->create(external);

			const fck_db_accessor editor = db_object->edit(external, blob);
			editor.edit->reference(editor, "runtime", runtime);
			editor.edit->commit(editor, fck_db_no_undo);
		}

		{
			// Everything is unstable!
			// We never copy properties, instead we create new chunks and then do simple pointer exchanges
			// That means this fck_db_asset address is fucked! :)
			fck_db_asset_private info = {0};
			info.base.id              = blob;
			info.base.timestamp       = os->chrono->now();
			info.base.category        = loader->category;
			info.base.userdata        = userdata;
			info.base.path            = relative_path;
			info.state                = fck_db_asset_state_imported;

			// We construct a proxy object to keep assets stable! :)
			// Like mentioned above, working with userdata is so extremely hacky
			// Functionality to stabalise or change behaviour as soon as userdata is present might work
			// Also, "named" userdata might also be valuable to have - this way we can have traits and
			// identify the kind of userdata.
			const fck_db_accessor reader = db_object->read(external, runtime);
			void                 *result = reader.read->userdata(reader, "asset");
			if (!result)
			{
				const fck_db_accessor editor = db_object->edit(external, runtime);
				result                       = editor.edit->userdata(editor, "asset", &info, sizeof(info));
				editor.edit->commit(editor, fck_db_no_undo);

				fck_db_asset_private *as_asset = (fck_db_asset_private *)result;
				as_asset->base.state           = &as_asset->state;
			}
			else
			{
				fck_db_asset_private *as_asset = (fck_db_asset_private *)result;
				*as_asset                      = info;
				as_asset->base.state           = &as_asset->state;
			}

			fck_db_asset_reference *ref = db_ext_map->cache(db->allocator, db->loaders, ext, relative_path);
			ref->id                     = blob;

			kll->arena->destroy(temp);
			return (fck_db_asset *)result;
		}
	}
	return NULL;
	// TODO: Fix up how assets work in here
	// TODO: OK, now we can iterate on this shit! :)
	// accessor.edit->i32(accessor, "type", loader->type);

	/*void* dst = fck_db_edit_api_lazy_find(external, &entry->object, fck_db_type_asset, "asset", sizeof(fck_db_asset*),
	sizeof(fck_db_asset*)); memcpy(dst, &payload, sizeof(fck_db_asset*));*/
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
		fck_db_section   *section         = db->sections + index;
		const fckc_size_t iteration_limit = 64;
		for (fckc_size_t iterations = 0; iterations < iteration_limit; iterations++)
		{
			fck_file_watcher_event changes[64];
			const fckc_size_t      result = os->fw->changes(section->watcher, changes, fck_arraysize(changes));
			if (result == 0)
			{
				break;
			}

			for (fckc_size_t change_index = 0; change_index < result; change_index++)
			{
				fck_file_watcher_event *change = changes + change_index;
				if (strstr(change->path, ".db.fck"))
				{
					// This branch and condition is very much removed!
					continue;
				}
				char        buffer[1024];
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
					// const fck_db_id id = fck_db_api_id_from_path(external, scoped_path);
					//  fck_multidir_remove_entry(&db->database.header, &db->database.root, id);
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

static fckc_size_t fck_db_api_sections_find(fck_db_private *db, const char *path)
{
	for (fckc_size_t index = 0; index < db->sections_count; index++)
	{
		fck_db_section *section = db->sections + index;
		if (strcmp(path, section->path) == 0)
		{
			return index + 1;
		}
	}
	return 0;
}

static void fck_db_api_setup(fck_db external, const char *scope, const char *path)
{
	fck_assert(scope);
	// TODO: Check duplication
	// Return a section handle
	fck_db_private *db = (fck_db_private *)external.opaque;
	if (fck_db_api_sections_find(db, path))
	{
		// Later on we probably want to ref-count and all that
		return;
	}

	fck_assert(db->sections_count < fck_arraysize(db->sections));
	fck_db_section *section = db->sections + db->sections_count;
	db->sections_count      = db->sections_count + 1;
	fck_db_section_init(section, scope, path);

	char            **paths;
	const fckc_size_t paths_count = os->glob->directory(path, NULL, &paths);
	for (fckc_size_t index = 0; index < paths_count; index++)
	{
		const char *relative = paths[index];
		fck_db_api_import_file(external, section, relative);
	}

	os->glob->free(paths);
}

static const fck_db_category_iterator *fck_db_asset_api_categories(fck_db external, fck_db_category_iterator *it)
{
	fck_db_private *db = (fck_db_private *)external.opaque;

	fck_db_loader_interface **loaders;
	const fckc_size_t         count = db_ext_map->loaders(db->loaders, &loaders);
	fck_db_loader_interface **last  = loaders + count;
	// All of this shit will break as soon as category is not the first member anymore
	// Such a change will be so fucking fun, so I leave it around
	if (it->handle == NULL)
	{
		it->handle = loaders;
	}
	else
	{
		fck_db_loader_interface **loader = (fck_db_loader_interface **)it->handle;
		loader                           = loader + 1;
		it->handle                       = loader;
	}
	if (it->handle == last)
	{
		// Done
		return NULL;
	}
	{
		fck_db_loader_interface **loader = (fck_db_loader_interface **)it->handle;
		it->name                         = (*loader)->category;
	}
	return it;
}

static void *fck_db_asset_api_category(fck_db external, const char *name)
{
	fck_db_private *db = (fck_db_private *)external.opaque;

	fck_db_loader_interface **loaders;
	const fckc_size_t         count = db_ext_map->loaders(db->loaders, &loaders);
	fck_db_loader_interface **last  = loaders + count;

	for (fckc_size_t index = 0; index < count; index++)
	{
		fck_db_loader_interface *loader = loaders[index];
		if (strcmp(loader->category, name) == 0)
		{
			return loaders + index;
		}
	}
	return NULL;
}

static const char *fck_db_asset_api_extensions(fck_db external, void *category, void **current, const char **ext)
{
	(void)external;
	// fck_db_private           *db     = (fck_db_private *)external.opaque;
	fck_db_loader_interface **loader = (fck_db_loader_interface **)category;

	const char      **extensions;
	const fckc_size_t count = (*loader)->supports(&extensions);
	const char      **last  = extensions + count;

	if (*current == NULL)
	{
		*current = extensions;
	}
	else
	{
		const char **extension = (const char **)*current;
		extension              = extension + 1;
		*current               = extension;
	}
	if (*current == last)
	{
		return NULL;
	}
	{
		const char **extension = (const char **)*current;
		*ext                   = *extension;
	}
	return *ext;
}

static fckc_size_t fck_db_asset_api_assetsof(fck_db external, const char *extension, const fck_db_asset_reference **assets)
{
	fck_db_private *db = (fck_db_private *)external.opaque;
	return db_ext_map->listof(db->loaders, extension, assets);
}

static int fck_db_asset_api_is(const fck_db_asset *asset, const char *category)
{
	return strcmp(asset->category, category) == 0;
}

// static fck_db_asset *fck_db_asset_api_create(fck_db external, const char *scope, const char *path, const char *category)
//{
//	fck_db_private *db = (fck_db_private *)external.opaque;
//
//	fckc_size_t result = fck_db_api_sections_find(db, path);
//	fck_assert(result && "Cannot find scope - Does not exist");
//
//	fck_db_section* section = db->sections + result - 1;
//
//	char** paths;
//	const fckc_size_t paths_count = os->glob->directory(path, NULL, &paths);
//	for (fckc_size_t index = 0; index < paths_count; index++)
//	{
//		const char* relative = paths[index];
//		fck_db_api_import_file(external, section, relative);
//	}
// }

static fck_db_asset_api db_asset_api = {
	.categories = fck_db_asset_api_categories,
	.category   = fck_db_asset_api_category,
	.extensions = fck_db_asset_api_extensions,
	.assetsof   = fck_db_asset_api_assetsof,

	.is = fck_db_asset_api_is,

	.find      = fck_db_asset_api_find,
	.lazy      = fck_db_asset_api_lazy,
	.get       = fck_db_asset_api_get,
	.setup     = fck_db_api_setup,
	.hotreload = fck_db_api_hotreload,

};

fck_db_asset_api *db_asset = &db_asset_api;