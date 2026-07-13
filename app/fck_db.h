#ifndef FCK_DB_H_INCLUDED
#define FCK_DB_H_INCLUDED

#include <fckc_inttypes.h>

#define fck_db_api_name "fck-db"
#define fck_db_loader_interface_name "fck-db-loader"

#define fck_db_item_meta_extension "fck"
#define fck_db_extension "db.fck"

struct kll_allocator;
struct fck_api_registry;

typedef struct fck_db_uuid
{
	fckc_u64 high;
	fckc_u64 low;
} fck_db_uuid;

typedef union fck_db_id {
	fckc_u64 value;

	struct
	{
		fckc_u64 type : 16;
		fckc_u64 generation : 16;
		fckc_u64 index : 32;
	};
} fck_db_id;

struct fck_db_private;
typedef struct fck_db
{
	struct fck_db_private *opaque;
} fck_db;

typedef struct fck_db_loader_interface
{
	// For now we set this one manually for the loaders!
	const char *name;
	fckc_u16 type;
	
	fckc_u16 pad[3];

	void *(*import)(const char *file);
	fckc_size_t (*supports)(const char ***extensions);
} fck_db_loader_interface;

typedef struct fck_db_api
{
	fck_db (*create)(struct kll_allocator *allocator, const char *path);
	void (*close)(fck_db db);
} fck_db_api;

extern fck_db_api *fck_db_load(struct fck_api_registry *apis, void *old);

#endif // !FCK_DB_H_INCLUDED
