#ifndef FCK_DB_H_INCLUDED
#define FCK_DB_H_INCLUDED

#include <fckc_inttypes.h>

#define fck_db_api_name "fck-db"

#define fck_db_loader_interface_name "fck-db-loader"

#define fck_category_object "fck-db-object"

// TODO: re-structure to have objects as the core and assets as the "mixin"
struct kll_allocator;
struct fck_api_registry;

// Secret stuff ;)
struct fck_db_object;
struct fck_db_section;

struct fck_serialiser;

typedef enum fck_db_type
{
	fck_db_type_none,
	fck_db_type_i32,
	fck_db_type_f32,
	fck_db_type_memory,
	fck_db_type_reference,
	fck_db_type_object,
	fck_db_type_asset,
	fck_db_type_object_set,
	fck_db_type_string,
	// fck_db_type_reference_set,
} fck_db_type;

typedef union fck_db_id {
	fckc_u64 value;
	struct
	{
		// Unsure if type and generation are useful for the time being
		// TODO: I think type is heavily misused and I need to look into it
		fckc_u64 type       : 16;
		fckc_u64 generation : 16;
		fckc_u64 index      : 32;
	};
} fck_db_id;

typedef enum fck_db_asset_state
{
	fck_db_asset_state_imported,
	fck_db_asset_state_requested,
	fck_db_asset_state_resolved,
} fck_db_asset_state;

typedef struct fck_db_asset
{
	// TODO: Compress this one
	fck_db_id           id;
	const char         *path;
	fck_db_asset_state *state;
	const char         *category;
	fckc_i64            timestamp;
	void               *userdata;
} fck_db_asset;

typedef struct fck_db_asset_reference
{
	fck_db_id   id;
	const char *path;
} fck_db_asset_reference;

typedef struct fck_db_id_set fck_db_id_set;

typedef struct fck_db_property
{
	fck_db_type type;
	union {
		fckc_i32 i32;
		fckc_f32 f32;

		struct
		{
			const void *data;
			fckc_size_t size;
		} memory;

		const char *string;

		fck_db_id_set *set;

		fck_db_id     object;
		fck_db_asset *asset;
	};
} fck_db_property;

// TODO: Rename this one to fck_db_propertey_iterator and add offset to it! :)
typedef struct fck_db_named_property
{
	const char     *name;
	fck_db_property value;
} fck_db_named_property;

// Wait, is this even a scope... A very big scope so it seems :-(
struct fck_db_undo_scope_private;
typedef struct fck_db_undo_scope
{
	struct fck_db_undo_scope_private *opaque;
} fck_db_undo_scope;

#define fck_db_no_undo NULL

struct fck_db_private;
typedef struct fck_db
{
	struct fck_db_private *opaque;
} fck_db;

struct fck_db_api;

typedef struct fck_db_loader_args
{
	struct fck_db_api       *api;
	struct fck_api_registry *registry;
	struct fck_db            db;
	fck_db_id                target;
} fck_db_loader_args;

typedef struct fck_db_loader_interface
{
	// For now we set this one manually for the loaders!
	const char *category; // And this is a category

	// void for now!!
	void       *(*import)(const fck_db_loader_args *args, const char *file);
	fckc_size_t (*supports)(const char ***extensions);
} fck_db_loader_interface;

struct fck_db_edit_api;
struct fck_db_read_api;
struct fck_db_ok_api;
typedef struct fck_db_accessor
{
	fck_db_id original;
	fck_db_id inflight;
	fck_db    db;

	struct fck_db_object   *obj;
	struct fck_db_read_api *read;
	struct fck_db_ok_api   *ok;
	struct fck_db_edit_api *edit;
} fck_db_accessor;

typedef struct fck_db_edit_api
{
	void (*variant)(fck_db_accessor accessor, const char *property, const fck_db_property *value);
	void (*i32)(fck_db_accessor accessor, const char *property, fckc_i32 value);
	void (*f32)(fck_db_accessor accessor, const char *property, fckc_f32 value);
	void (*asset)(fck_db_accessor accessor, const char *property, const fck_db_asset *asset);
	void (*reference)(fck_db_accessor accessor, const char *property, fck_db_id id);
	void (*object)(fck_db_accessor accessor, const char *property, fck_db_id id);
	void (*memory)(fck_db_accessor accessor, const char *property, const void *data, fckc_size_t size);
	void (*string)(fck_db_accessor accessor, const char *property, const char *value);

	void (*set)(fck_db_accessor accessor, const char *property, const fck_db_id_set *set);

	void *(*userdata)(fck_db_accessor accessor, const char *property, const void *data, fckc_size_t size);

	void (*commit)(fck_db_accessor accessor, fck_db_undo_scope *undo);
} fck_db_edit_api;

typedef struct fck_db_read_api
{
	fckc_u32 (*iterate)(fck_db_accessor accessor, fckc_u32 *offset, fck_db_named_property *property);
	fckc_u32 (*version)(fck_db_accessor accessor);

	void               *(*untyped)(fck_db_accessor accessor, fck_db_type type, const char *property);
	fck_db_property     (*variant)(fck_db_accessor accessor, const char *property);
	fckc_i32            (*i32)(fck_db_accessor accessor, const char *property);
	fckc_f32            (*f32)(fck_db_accessor accessor, const char *property);
	const fck_db_asset *(*asset)(fck_db_accessor accessor, const char *property);
	fck_db_id           (*reference)(fck_db_accessor accessor, const char *property);
	fck_db_id           (*object)(fck_db_accessor accessor, const char *property);
	fckc_size_t         (*memory)(fck_db_accessor accessor, const char *property, const void **data);
	const char         *(*string)(fck_db_accessor accessor, const char *property);

	void *(*userdata)(fck_db_accessor accessor, const char *property);

	const fck_db_id_set *(*set)(fck_db_accessor accessor, const char *property);
} fck_db_read_api;

typedef struct fck_db_ok_api
{
	int (*variant)(fck_db_accessor accessor, const char *property);
	int (*i32)(fck_db_accessor accessor, const char *property);
	int (*f32)(fck_db_accessor accessor, const char *property);
	int (*asset)(fck_db_accessor accessor, const char *property);
	int (*reference)(fck_db_accessor accessor, const char *property);
	int (*object)(fck_db_accessor accessor, const char *property);
	int (*memory)(fck_db_accessor accessor, const char *property);
	int (*string)(fck_db_accessor accessor, const char *property);

	int (*set)(fck_db_accessor accessor, const char *property);
} fck_db_ok_api;

typedef struct fck_db_id_set_api
{
	fck_db_id_set   *(*create)(struct kll_allocator *allocator, fckc_size_t capacity);
	void             (*destroy)(struct kll_allocator *allocator, fck_db_id_set *set);
	int              (*add)(struct kll_allocator *allocator, fck_db_id_set **set, fck_db_id id);
	int              (*contains)(fck_db_id_set *set, fck_db_id id);
	int              (*remove)(fck_db_id_set *set, fck_db_id id);
	const fck_db_id *(*iterate)(const fck_db_id_set *set, const fck_db_id **it);
	fckc_size_t      (*count)(fck_db_id_set *set);
} fck_db_id_set_api;

typedef struct fck_db_object_api
{
	fck_db_id       (*create)(fck_db db);
	void            (*destroy)(fck_db db, fck_db_id id);
	fck_db_accessor (*read)(fck_db db, fck_db_id id);
	fck_db_accessor (*edit)(fck_db db, fck_db_id id);
	int             (*is_ok)(fck_db db, fck_db_id id);

	const fck_db_asset *(*save)(fck_db db, fck_db_id id, const char *scope, const char *path);
	const fck_db_id    *(*refresh)(fck_db db, const fck_db_asset *asset, fck_db_undo_scope *scope, fck_db_id *id);
	// void                (*load)(fck_db db);
} fck_db_object_api;

typedef struct fck_db_category_iterator
{
	void       *handle;
	const char *name;
} fck_db_category_iterator;

typedef struct fck_db_asset_api
{
	void (*setup)(fck_db db, const char *scope, const char *path);

	// Prefer iterators over void*!
	const fck_db_category_iterator *(*categories)(fck_db db, fck_db_category_iterator *it);
	void                           *(*category)(fck_db db, const char *name);
	int                             (*is)(const fck_db_asset *asset, const char *category);
	const char                     *(*extensions)(fck_db db, void *category, void **current, const char **extension);
	fckc_size_t                     (*assetsof)(fck_db db, const char *extension, const fck_db_asset_reference **assets);
	const fck_db_asset             *(*get)(fck_db db, fck_db_id id, const char *category);
	const fck_db_asset             *(*find)(fck_db db, const char *path);

	// ?
	const fck_db_asset *(*lazy)(fck_db db, const char *path, const char *category);

	// fck_db_asset *(*create)(fck_db db, const char *scope, const char *path, const char *category);

	void (*hotreload)(fck_db external);
} fck_db_asset_api;

typedef struct fck_db_undo_api
{
	fck_db_undo_scope (*create)(struct kll_allocator *allocator);
	void              (*destroy)(fck_db_undo_scope scope);
	// Let's see if this will work out well
	int               (*undo)(fck_db db, fck_db_undo_scope scope);
	int               (*redo)(fck_db db, fck_db_undo_scope scope);
} fck_db_undo_api;

typedef struct fck_db_api
{
	fck_db_id_set_api *set;
	fck_db_object_api *object;
	fck_db_asset_api  *asset;
	fck_db_undo_api   *undo;

	fck_db (*create)(struct kll_allocator *allocator);
	void   (*close)(fck_db db);
} fck_db_api;

// Utility for convenience
inline static const char *fck_db_extension(const char *path)
{
	if (!path)
	{
		return "";
	}

	const char *dot     = 0;
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
