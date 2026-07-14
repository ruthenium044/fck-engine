#ifndef FCK_DB_H_INCLUDED
#define FCK_DB_H_INCLUDED

#include <fckc_inttypes.h>

#define fck_db_api_name "fck-db"
#define fck_db_loader_interface_name "fck-db-loader"

#define fck_db_item_meta_extension "fck"
#define fck_db_path_extension "db.fck"

struct kll_allocator;
struct fck_api_registry;

typedef enum fck_db_type
{
	fck_db_asset,
} fck_db_type;

typedef struct fck_db_element
{
	fck_db_type type;
	fckc_i64 timestamp;
} fck_db_element;

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

	fck_db_element *(*import)(struct fck_api_registry *registry, const char *file);
	fckc_size_t (*supports)(const char ***extensions);
} fck_db_loader_interface;

typedef struct fck_db_api
{
	fck_db (*create)(struct kll_allocator *allocator, const char *path);
	void (*hotreload)(fck_db db);
	fck_db_element *(*get)(fck_db db, const char *path);
	void (*close)(fck_db db);
} fck_db_api;

// TODO: Remove all the .db.fck junk for now, let's keep it simple and path-based
// ITERATION 1!! !11!
extern fck_db_api *fck_db_load(struct fck_api_registry *apis, void *old);

// Utility for convenience

inline static const char *fck_db_extension(const char *path)
{
	if (!path)
	{
		return "";
	}

	const char *dot = 0;
	const char *current = path;
	while (*current)
	{
		if (*current == '.')
		{
			dot = current;
		}
		current++;
	}
	if (!dot || dot == path)
	{
		return "";
	}

	return dot + 1;
}

#endif // !FCK_DB_H_INCLUDED
