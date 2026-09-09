
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
#include <string.h>

#include <fckc_atomic.h>

#include "fck_db_core.inl"
#include "fck_db_object_page_table.h"

#include "fck_db_undo.h"

#include "fck_db_object.h"

#include "fck_db_id_set.h"

#include "fck_db_ext_map.h"

#include "fck_db_object_asset.h"

static fck_api_registry *apis = NULL;

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
	z          = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
	z          = (z ^ (z >> 27)) * 0x94d049bb133111eb;
	return z ^ (z >> 31);
}

// CORE
fck_db_id fck_db_id_make(fckc_u8 e0, fckc_u8 e1, fckc_u8 e2, fckc_u8 e3, fck_db_type type)
{
	fck_db_id id = {0};
	id.index     = ((fckc_u32)e0 << 24) | ((fckc_u32)e1 << 16) | ((fckc_u32)e2 << 8) | (fckc_u32)e3;
	id.index     = id.index ^ fck_bitmask(32); // 32 bit, remember this
	id.type      = type;
	return id;
}

// CORE
int fck_db_id_ok(fck_db_id id)
{
	return id.index != 0LU;
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

static fck_db fck_db_api_create(kll_allocator *allocator)
{
	fck_db_private *db = (fck_db_private *)kll_malloc(allocator, sizeof(*db));
	memset(db, 0, sizeof(*db));

	db->allocator  = allocator;
	db->strings    = kll->arena->create(allocator, 512);
	db->page_table = fck_db_object_page_table_alloc(allocator);
	db->loaders    = db_ext_map->alloc(allocator, apis);
	db->registry   = apis;

	const fck_db result = {.opaque = db};
	return result;
}

static void fck_db_api_close(fck_db db)
{
	db_ext_map->free(db.opaque->allocator, db.opaque->loaders);
	fck_db_object_page_table_free(db.opaque->page_table);
	kll_free(db.opaque->allocator, db.opaque);
}

static void *fck_directory_import(const fck_db_loader_args *args, const char *path)
{
	(void)args;
	fck_path_info info;
	if (os->fs->info(path, &info))
	{
		os->io->log("Load Directory: %s", path);
		if (info.type == fck_path_directory)
		{
		}
	}
	return NULL;
}

static fckc_size_t fck_directory_supports(const char ***extensions)
{
	static const char *supported[] = {""};
	*extensions                    = supported;
	return fck_arraysize(supported);
}

static fck_db_loader_interface directory_loader = {
	.category = "directory",
	.import   = fck_directory_import,
	.supports = fck_directory_supports,
};

FCK_EXPORT_API fck_db_api *fck_db_load(fck_api_registry *registry, void *old)
{
	static fck_db_api api = {
		.create = fck_db_api_create,
		.close  = fck_db_api_close,
	};

	// "Runtime" resolved addresses...
	api.set    = db_id_set;
	api.object = db_object;
	api.undo   = db_undo;
	api.asset  = db_asset;

	(void)old;
	apis = registry;
	apis->add(fck_db_api_name, &api);
	apis->add(fck_db_loader_interface_name, &directory_loader);
	apis->add(fck_db_loader_interface_name, db_object_import);

	return &api;
}