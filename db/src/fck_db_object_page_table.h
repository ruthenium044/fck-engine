
#ifndef FCK_DB_OBJECT_PAGE_TABLE_H_INCLUDED
#define FCK_DB_OBJECT_PAGE_TABLE_H_INCLUDED

#include <fckc_inttypes.h>

struct fck_db_object;
struct kll_allocator;

typedef struct fck_db_object_page_table fck_db_object_page_table;

void fck_db_object_page_id_extract(fckc_u32 id, fckc_u8 *e0, fckc_u8 *e1, fckc_u8 *e2, fckc_u8 *e3);

fck_db_object_page_table *fck_db_object_page_table_alloc(struct kll_allocator *allocator);
void fck_db_object_page_table_free(fck_db_object_page_table *table);

struct fck_db_object *fck_db_object_page_table_resolve(fck_db_object_page_table *table, fckc_u32 id);
struct fck_db_object *fck_db_object_page_table_ensure(fck_db_object_page_table *table, fckc_u32 id);
struct fck_db_object *fck_db_object_page_table_add(fck_db_object_page_table *table, fckc_u32 id);
int fck_db_object_page_table_remove(fck_db_object_page_table *table, fckc_u32 id);

#endif // !FCK_OBJECT_PAGE_TABLE_H_INCLUDED
