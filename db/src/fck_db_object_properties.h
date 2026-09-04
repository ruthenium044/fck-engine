#ifndef FCK_DB_OBJECT_PROPERTIES_H_INCLUDED
#define FCK_DB_OBJECT_PROPERTIES_H_INCLUDED

#include <fckc_inttypes.h>

struct kll_allocator;
struct fck_db_object;
struct fck_db_property_instance;

enum fck_db_type;

typedef struct fck_db_object_properties_api
{
	// Maybe release is better?
	void (*free)(struct kll_allocator *allocator, struct fck_db_object *obj);
	fckc_size_t (*find)(struct fck_db_object *instance, enum fck_db_type type, const char *name);
	fckc_size_t (*remove)(struct fck_db_object *instance, enum fck_db_type type, const char *name, fckc_size_t size);
	fckc_size_t (*add)(struct kll_allocator *allocator, struct fck_db_object *instance, enum fck_db_type type, const char *name);
	// Crap name :-(
	int (*is_property_used)(const struct fck_db_property_instance *property);

	// The parameters are absolutely fucking tricky
	void (*adjust_offsets)(struct fck_db_object *instance, fckc_size_t offset, fckc_size_t size);
} fck_db_object_properties_api;

extern fck_db_object_properties_api *db_properties;

#endif //! FCK_DB_OBJECT_PROPERTIES_H_INCLUDED