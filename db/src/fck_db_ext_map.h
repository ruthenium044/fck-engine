#ifndef FCK_DB_EXT_MAP_H_INCLUDED
#define FCK_DB_EXT_MAP_H_INCLUDED

struct fck_db_ext_map;
struct kll_allocator;
struct fck_db_loader_interface;
struct fck_api_registry;

typedef struct fck_db_ext_map_api
{
	struct fck_db_ext_map *(*alloc)(struct kll_allocator *allocator, struct fck_api_registry *apis);
	void (*free)(struct kll_allocator *allocator, struct fck_db_ext_map *map);
	struct fck_db_loader_interface *(*find)(struct fck_db_ext_map *map, const char *ext);
	int (*add)(struct fck_db_ext_map *map, const char *ext, struct fck_db_loader_interface *loader);
} fck_db_ext_map_api;

extern fck_db_ext_map_api *db_ext_map;

#endif //! FCK_DB_EXT_MAP_H_INCLUDED
