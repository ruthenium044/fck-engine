
#include "fck_db_object.h"
#include "fck_db.h"
#include "fck_db_core.inl"

#include <fck_serialiser.h>
#include <kll.h>
#include <kll_malloc.h>

#include <fck_os.h>
#include <fckc_inttypes.h>

#include "fck_db_accessor_edit.h"
#include "fck_db_accessor_ok.h"
#include "fck_db_accessor_read.h"
#include "fck_db_id_set.h"

#include "fck_db_object_asset.h"

#include "fckc_assert.h"

#include "yyjson.h"

#include <stdio.h>
#include <string.h>

static fck_db_id fck_db_id_create_and_next(fck_db_private *db)
{
	fckc_u8          *e          = db->id_factory;
	const fckc_size_t iterations = fck_arraysize(db->id_factory);

	for (fckc_size_t i = iterations; i > 0; i--)
	{
		const fckc_size_t index = i - 1;
		e[index]                = e[index] + 1;

		if (e[index] == 0xFF)
		{
			e[index] = 0;
			continue;
		}
		break;
	}

	os->io->log("Id: %d \t- %d \t- %d \t- %d", to_int(e[0]), to_int(e[1]), to_int(e[2]), to_int(e[3]));
	fck_db_id id = fck_db_id_make(e[0], e[1], e[2], e[3], fck_db_type_object);
	return id;
}

static fck_db_object fck_db_object_clone(kll_allocator *allocator, const fck_db_object *obj)
{
	const fckc_size_t         total = obj->capacity * sizeof(*obj->properties);
	fck_db_property_instance *props = (fck_db_property_instance *)kll_malloc(allocator, total);
	memcpy(props, obj->properties, total);

	void *data = kll_malloc(allocator, obj->size);
	memcpy(data, obj->data, obj->size);

	fck_db_object result = {0};
	result.version       = obj->version + 1 % to_u32(0xFFFFFFFF);

	result.properties = props;
	result.count      = obj->count;
	result.capacity   = obj->capacity;

	result.data = data;
	result.at   = obj->at;
	result.size = obj->size;

	return result;
}

static fck_db_id fck_db_object_api_create(fck_db external)
{
	fck_db_private *db    = external.opaque;
	const fck_db_id id    = fck_db_id_create_and_next(db);
	// We may have to try again if this one already exists... No check for that yet... Oh boy
	fck_db_object  *entry = fck_db_add_object(db->page_table, id);
	fck_assert(entry);
	return id;
}

static fck_db_accessor fck_db_object_api_edit(fck_db external, fck_db_id id)
{
	fck_db_private *db    = external.opaque;
	fck_db_object  *entry = fck_db_resolve_object(db->page_table, id);
	fck_assert(entry);

	// fck_db_object_api_create(id, entry->name);
	const fck_db_id temp = fck_db_object_api_create(external); // fck_db_id_advance(id);

	fck_db_object *copy = fck_db_resolve_object(db->page_table, temp);
	*copy               = fck_db_object_clone(db->allocator, entry);

	fck_db_accessor accessor = {
		.original = id,
		.inflight = temp,
		.edit     = db_edit,
		.read     = db_read,
		.ok       = db_ok,
		.db       = external,
		.obj      = copy,
	};

	return accessor;
}

static fck_db_accessor fck_db_object_api_read(fck_db external, fck_db_id id)
{
	fck_db_private *db    = external.opaque;
	fck_db_object  *entry = fck_db_resolve_object(db->page_table, id);
	fck_assert(entry);

	fck_db_accessor accessor = {
		.original = id,
		.inflight = id,
		.db       = external,
		.edit     = NULL,
		.ok       = db_ok,
		.read     = db_read,
		.obj      = entry,
	};

	return accessor;
}

static void fck_db_object_api_destroy(fck_db external, fck_db_id id)
{
	fck_db_private *db = external.opaque;
	fck_db_remove_object(db->page_table, id);
}

static void fck_db_object_api_save_object(fck_db db, fck_db_id id, yyjson_mut_doc *doc, yyjson_mut_val *root)
{
	const fck_db_accessor reader = fck_db_object_api_read(db, id);

	fckc_u32              offset = 0;
	fck_db_named_property property;
	while (reader.read->iterate(reader, &offset, &property))
	{
		yyjson_mut_val *value = yyjson_mut_obj_add_obj(doc, root, property.name);
		yyjson_mut_obj_add_int(doc, value, "fck-type", (int)property.value.type);
		switch (property.value.type)
		{
		case fck_db_type_none:
			break;
		case fck_db_type_i32:
			yyjson_mut_obj_add_int(doc, value, "value", property.value.i32);
			break;
		case fck_db_type_f32:
			yyjson_mut_obj_add_float(doc, value, "value", property.value.f32);
			break;
		case fck_db_type_memory: {
			yyjson_mut_obj_add_null(doc, value, "value");
			break;
		}
		case fck_db_type_object: {
			yyjson_mut_val *obj = yyjson_mut_obj_add_obj(doc, value, "value");
			fck_db_object_api_save_object(db, property.value.object, doc, obj);
			break;
		}
		case fck_db_type_reference:
			yyjson_mut_obj_add_uint(doc, value, "value", property.value.object.value);
			break;
		case fck_db_type_asset: {
			yyjson_mut_val *obj = yyjson_mut_obj_add_obj(doc, value, "value");
			yyjson_mut_obj_add_str(doc, obj, "category", property.value.asset->category);
			yyjson_mut_obj_add_str(doc, obj, "path", property.value.asset->path);
			break;
		}
		case fck_db_type_object_set: {
			yyjson_mut_val *arr = yyjson_mut_obj_add_arr(doc, value, "value");
			fck_db_id      *id  = NULL;
			while (db_id_set->iterate(property.value.set, &id))
			{
				yyjson_mut_val *obj = yyjson_mut_arr_add_obj(doc, arr);
				fck_db_object_api_save_object(db, *id, doc, obj);
			}
			break;
		}
		case fck_db_type_string: {
			yyjson_mut_obj_add_str(doc, value, "value", property.value.string);
			break;
		}
		}
	}
}

static const fck_db_asset *fck_db_object_api_save(fck_db external, fck_db_id id, const char *scope, const char *path)
{
	fck_db_private *db = external.opaque;

	yyjson_mut_doc *doc  = yyjson_mut_doc_new(NULL);
	yyjson_mut_val *root = yyjson_mut_obj(doc);
	yyjson_mut_doc_set_root(doc, root);

	fck_db_object_api_save_object(external, id, doc, root);

	size_t      size;
	const char *json = yyjson_mut_write(doc, YYJSON_WRITE_PRETTY, &size);

	if (json)
	{
		fckc_size_t result = 0;
		for (fckc_size_t index = 0; index < db->sections_count; index++)
		{
			fck_db_section *section = db->sections + index;
			if (strcmp(scope, section->scope) == 0)
			{
				result = index + 1;
				break;
			}
		}

		if (result == 0)
		{
			return NULL;
		}

		char                  buffer[420] = {0};
		const fck_db_section *target      = db->sections + result - 1;
		(void)snprintf(buffer, sizeof(buffer), "%s%s.json", target->path, path);

		const fck_file file = os->fs->open(buffer, "w");
		os->fs->write(file, (const void *)json, size);
		os->fs->close(file);

		os->io->log(json);
		yyjson_mut_doc_free(doc);
	}
	return NULL;
}

static const void fck_db_object_api_load(fck_db db, fck_db_accessor editor, yyjson_mut_doc *doc, yyjson_mut_val *current);

static const fck_db_id fck_db_object_api_load_json_obj(fck_db db, yyjson_mut_doc *doc, yyjson_mut_val *current)
{
	const fck_db_id       child  = db_object->create(db);
	const fck_db_accessor editor = db_object->edit(db, child);

	size_t          idx, max;
	yyjson_mut_val *key, *val;
	yyjson_mut_obj_foreach(current, idx, max, key, val)
	{
		fck_db_object_api_load(db, editor, doc, key);
	}

	editor.edit->commit(editor, fck_db_no_undo);
	return child;
}

static const void fck_db_object_api_load(fck_db db, fck_db_accessor editor, yyjson_mut_doc *doc, yyjson_mut_val *key
                                         /* yyjson_mut_val *current*/)
{
	yyjson_mut_val *current = key->next;
	if (current == NULL)
	{
		return;
	}
	// yyjson_mut_val *key = ((yyjson_mut_val*)(current)->uni.ptr)->next->next;
	const char *name = yyjson_mut_get_str(key);

	fck_assert(yyjson_mut_is_obj(current));

	yyjson_mut_val *type = yyjson_mut_obj_get(current, "fck-type");
	fck_assert(type && yyjson_mut_is_int(type));

	const fck_db_type proeprty_type = (fck_db_type)yyjson_mut_get_int(type);

	yyjson_mut_val *value = yyjson_mut_obj_get(current, "value");
	fck_assert(value);

	switch (proeprty_type)
	{
	case fck_db_type_none: // :(
		break;
	case fck_db_type_i32: {
		fck_assert(yyjson_mut_is_int(value));
		const fckc_i64 v = yyjson_mut_get_sint(value);
		editor.edit->i32(editor, name, to_u32(v));
		break;
	}
	case fck_db_type_f32: {
		fck_assert(yyjson_mut_is_real(value));
		const double v = yyjson_mut_get_real(value);
		editor.edit->f32(editor, name, to_f32(v));
		break;
	}
	case fck_db_type_memory: {
		fck_assert(yyjson_mut_is_null(value));
		editor.edit->memory(editor, name, NULL, 0);
		// ...
		break;
	}
	case fck_db_type_object: {
		const fck_db_id id = fck_db_object_api_load_json_obj(db, doc, value);
		editor.edit->object(editor, name, id);
		break;
	}
	case fck_db_type_reference: {
		fck_assert(yyjson_mut_is_uint(value));
		const fckc_u64 v = yyjson_mut_get_uint(value);
		// TODO!!
		break;
	}
	case fck_db_type_asset: {
		fck_assert(yyjson_mut_is_obj(value));
		yyjson_mut_val *category = yyjson_mut_obj_get(value, "category");
		yyjson_mut_val *path     = yyjson_mut_obj_get(value, "path");
		fck_assert(yyjson_mut_is_str(category) && yyjson_mut_is_str(path));

		const fck_db_asset *asset = db_asset->lazy(db, yyjson_mut_get_str(path), yyjson_mut_get_str(category));
		editor.edit->asset(editor, name, asset);
		break;
	}
	case fck_db_type_object_set: {
		fck_assert(yyjson_mut_is_arr(value));

		fck_db_id_set *set = db_id_set->create(kll->system, yyjson_mut_arr_size(value) * 2);

		size_t          idx, max;
		yyjson_mut_val *item;
		yyjson_mut_arr_foreach(value, idx, max, item)
		{
			const fck_db_id id = fck_db_object_api_load_json_obj(db, doc, item);
			db_id_set->add(NULL, &set, id);
		}
		editor.edit->set(editor, name, set);
		db_id_set->destroy(kll->system, set);
		break;
	}
	case fck_db_type_string: {
		// yyjson_mut_obj_foreach(
		fck_assert(yyjson_mut_is_str(value));
		const char *v = yyjson_mut_get_str(value);
		editor.edit->string(editor, name, v);
		break;
	}
	}
}

typedef struct fck_db_object_asset
{
	yyjson_mut_doc *doc;
	fck_db_id       object;
} fck_db_object_asset;

fck_db_id fck_db_object_resolve(const fck_db_asset *asset)
{
	if (strcmp(asset->category, fck_category_object) != 0)
	{
		// Uuhhh... Maybe a return value would
		return (fck_db_id){0};
	}

	const fck_db_object_asset *data = (fck_db_object_asset *)asset->userdata;
	return data->object;
}

static void *fck_db_object_import(const fck_db_loader_args *args, const char *path)
{
	os->io->log("Load db object: %s", path);

	const fck_db_accessor accessor = args->api->object->edit(args->db, args->target);

	fck_db_object_asset *asset = (fck_db_object_asset *)accessor.read->userdata(accessor, fck_category_object);
	if (asset == NULL)
	{
		fck_db_object_asset initial = {0};

		asset = (fck_db_object_asset *)accessor.edit->userdata(accessor, fck_category_object, &initial, sizeof(initial));
	}

	if (asset->doc != NULL)
	{
		yyjson_mut_doc_free(asset->doc);
	}
	yyjson_doc *idoc = yyjson_read_file(path, 0, NULL, NULL);

	asset->doc = yyjson_doc_mut_copy(idoc, NULL);
	// doc = (yyjson_mut_doc *)accessor.edit->userdata(accessor, "fck-db-object", &doc, sizeof(doc));

	// TODO: Read
	// TODO: Read
	if (asset->doc)
	{
		if (fck_db_id_ok(asset->object))
		{
			db_object->destroy(args->db, asset->object);
		}
		asset->object                = db_object->create(args->db);
		const fck_db_accessor editor = db_object->edit(args->db, asset->object);

		yyjson_mut_val *obj = yyjson_mut_doc_get_root(asset->doc);

		size_t          idx, max;
		yyjson_mut_val *key, *val;
		yyjson_mut_obj_foreach(obj, idx, max, key, val)
		{
			fck_db_object_api_load(args->db, editor, asset->doc, key);
		}
		editor.edit->commit(editor, fck_db_no_undo);
	}
	yyjson_doc_free(idoc);

	return asset;
}

static fckc_size_t fck_db_object_supports(const char ***extensions)
{
	static const char *supported[] = {"json"};
	*extensions                    = supported;
	return fck_arraysize(supported);
}

static void fck_db_print(fck_db external, fck_db_id id)
{
	const fck_db_accessor reader   = fck_db_object_api_read(external, id);
	fckc_u32              it       = 0;
	fck_db_named_property property = {0};
	while (reader.read->iterate(reader, &it, &property))
	{
		switch (property.value.type)
		{
		case fck_db_type_f32:
			os->io->log("Name: %s - %f", property.name, property.value.f32);
			break;
		case fck_db_type_none:
			break;
		case fck_db_type_i32:
			os->io->log("Name: %s - %d", property.name, property.value.i32);
			break;
		case fck_db_type_memory:
		case fck_db_type_object:
			fck_db_print(external, property.value.object);
			break;
		case fck_db_type_asset:
		case fck_db_type_reference:
			break;
		}
	}
}

// static void fck_db_object_api_load(fck_db external)
//{
//	fck_db_private *db = external.opaque;
// }

static int fck_db_object_api_is_ok(fck_db external, fck_db_id id)
{
	fck_db_private *db  = external.opaque;
	fck_db_object  *ptr = fck_db_resolve_object(db->page_table, id);
	return ptr != NULL;
}

static fck_db_object_api db_object_api = {
	.create  = fck_db_object_api_create,
	.destroy = fck_db_object_api_destroy,
	.read    = fck_db_object_api_read,
	.edit    = fck_db_object_api_edit,
	.save    = fck_db_object_api_save,
	//.load    = fck_db_object_api_load,
	.is_ok   = fck_db_object_api_is_ok,

	.resolve = fck_db_object_resolve,
};

static fck_db_loader_interface db_object_loader = {
	.category = fck_category_object,
	.import   = fck_db_object_import,
	.supports = fck_db_object_supports,
};

fck_db_loader_interface *db_object_import = &db_object_loader;
fck_db_object_api       *db_object        = &db_object_api;