#ifndef FCK_DB_CORE_INL_INCLUDED
#define FCK_DB_CORE_INL_INCLUDED

#include "fck_db.h"

#include <fck_os.h>
#include <fckc_atomic.h>
#include <fckc_inttypes.h>

struct kll_allocator;
struct kll_arena;
struct fck_db_object_page_table;
struct fck_db_ext_map;
struct fck_api_registry;

typedef struct fck_db_id_set
{
	fckc_size_t count;
	fckc_size_t capacity;
	fck_db_id values[1];
} fck_db_id_set;

struct fck_db_property_instance;
typedef struct fck_db_property_instance
{
	const char *name;
	fck_db_type type;
	fckc_u32 offset;
} fck_db_property_instance;

typedef struct fck_db_object
{
	//fck_db_uuid uuid;

	fckc_u32 version;
	fckc_u32 count;

	fckc_u32 capacity;
	fckc_size_t at;
	fckc_size_t size;

	fck_db_property_instance *properties;
	void *data;
} fck_db_object;

typedef struct fck_db_memory
{
	fckc_size_t count;
	fckc_size_t capacity;
	fckc_u8 data[1];
} fck_db_memory;

typedef struct fck_db_section
{
	fck_file_watcher watcher;
	char path[420];
	char scope[256];
} fck_db_section;

typedef struct fck_db_private
{
	struct kll_allocator *allocator;
	struct kll_arena *strings;
	struct fck_api_registry *registry;
	struct fck_db_ext_map *loaders;

	struct fck_db_object_page_table *page_table;
	fck_db_section sections[16];
	fckc_size_t sections_count;

	fckc_u8 id_factory[4];
} fck_db_private;

typedef struct fck_db_undo_unit
{
	fck_db_id target;
	fck_db_id copy;
} fck_db_undo_unit;

typedef struct fck_db_undo_scope_private
{
	struct kll_allocator *allocator;
	fck_db_undo_unit units[16];
	fckc_u32 cursor;
	fckc_u32 front;
	fckc_u32 back;
} fck_db_undo_scope_private;

// some core functionality
fck_db_object *fck_db_resolve_object(struct fck_db_object_page_table *table, fck_db_id id);
fck_db_object *fck_db_ensure_object(struct fck_db_object_page_table *table, fck_db_id id);
fck_db_object *fck_db_add_object(struct fck_db_object_page_table *table, fck_db_id id);
int fck_db_remove_object(struct fck_db_object_page_table *table, fck_db_id id);

fck_db_id fck_db_id_make(fckc_u8 e0, fckc_u8 e1, fckc_u8 e2, fckc_u8 e3, fck_db_type type);
int fck_db_id_ok(fck_db_id id);

#endif // !FCK_DB_CORE_INL_INCLUDED
