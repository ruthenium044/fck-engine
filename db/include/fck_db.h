#ifndef FCK_DB_H_INCLUDED
#define FCK_DB_H_INCLUDED

#include <fckc_inttypes.h>

#define fck_db_api_name "fck-db"

#define fck_db_loader_interface_name "fck-db-loader"

struct kll_allocator;
struct fck_api_registry;

typedef enum fck_db_type
{
	fck_db_type_asset,
} fck_db_type;

typedef struct fck_db_asset
{
	fck_db_type type;
	fckc_i64 timestamp;
	fckc_size_t size;

} fck_db_asset;

typedef union fck_db_id {
	fckc_u64 value;
	struct
	{
		fckc_u64 type : 16;
		fckc_u64 generation : 16;
		fckc_u64 index : 32;
	};
} fck_db_id;

// Secret object
struct fck_db_object;

struct fck_db_private;
typedef struct fck_db
{
	struct fck_db_private *opaque;
} fck_db;

struct fck_db_api;

typedef struct fck_db_loader_args
{
	struct fck_db_api *api;
	struct fck_db db;
	struct fck_api_registry *registry;
	fck_db_id target;
} fck_db_loader_args;

typedef struct fck_db_loader_interface
{
	// For now we set this one manually for the loaders!
	const char *name;
	fckc_u16 type;

	fckc_u16 pad[3];
	fck_db_asset *(*import)(const fck_db_loader_args *args, const char *file);
	fckc_size_t (*supports)(const char ***extensions);
} fck_db_loader_interface;

// TODO: Small type system, object creation, field stuff ,etc...
typedef struct fck_db_object_api
{
	fck_db_id (*create)(fck_db db);

	struct fck_db_object *(*write)(fck_db db, fck_db_id id);

	void (*set_i32)(fck_db db, struct fck_db_object *obj, fckc_i32 value);
	void (*set_f32)(fck_db db, struct fck_db_object *obj, fckc_f32 value);
	void (*set_asset)(fck_db db, struct fck_db_object *obj, fck_db_asset *asset);
	void (*set_memory)(fck_db db, struct fck_db_object *obj, const void *data, fckc_size_t size);

	void (*commit)(fck_db db, struct fck_db_object *obj);

} fck_db_object_api;

typedef struct fck_db_api
{
	fck_db (*create)(struct kll_allocator *allocator, const char *path);
	void (*hotreload)(fck_db db);

	fck_db_asset *(*get_from_id)(fck_db db, fck_db_id id);

	fck_db_asset *(*get)(fck_db db, const char *path);
	void (*close)(fck_db db);
} fck_db_api;

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
