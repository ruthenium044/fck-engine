#ifndef FCK_DB_EXT_MAP_H_INCLUDED
#define FCK_DB_EXT_MAP_H_INCLUDED

#include <fckc_inttypes.h>

struct fck_db_ext_map;
struct kll_allocator;
struct fck_db_loader_interface;
struct fck_api_registry;
struct fck_db_asset_reference;

typedef struct fck_db_ext_map_api
{
	struct fck_db_ext_map *(*alloc)(struct kll_allocator *allocator, struct fck_api_registry *apis);
	void (*free)(struct kll_allocator *allocator, struct fck_db_ext_map *map);
	struct fck_db_loader_interface *(*find)(struct fck_db_ext_map *map, const char *ext);
	int (*add)(struct fck_db_ext_map *map, const char *ext, struct fck_db_loader_interface *loader);

	fckc_size_t (*loaders)(struct fck_db_ext_map *map, struct fck_db_loader_interface ***loaders);

	struct fck_db_asset_reference *(*cache)(struct kll_allocator *allocator, struct fck_db_ext_map *map, const char *ext, const char *path);
	fckc_size_t (*listof)(struct fck_db_ext_map *map, const char *ext, const struct fck_db_asset_reference **entries);
} fck_db_ext_map_api;

extern fck_db_ext_map_api *db_ext_map;

#endif //! FCK_DB_EXT_MAP_H_INCLUDED
