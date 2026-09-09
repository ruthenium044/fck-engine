
#include "fck_serialiser_json.h"
#include <fck_serialiser.h>

#include <ctype.h>
#include <fck_apis.h>
#include <fck_hash.h>
#include <fck_input.h>
#include <fck_mouse.h>
#include <fck_os.h>
#include <fck_pkey.h>
#include <fck_plugins.h>
#include <fck_shader.h>
#include <fckc_assert.h>
#include <fckc_inttypes.h>
#include <kll.h>
#include <kll_format.h>
#include <kll_malloc.h>
#include <sht_render.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fck_texture.h>

#include <fck_gfx.h>
#include <fck_nuklear.h>

#include <fck_ec.h>
#include <fck_gameloop.h>
#include <fck_sprite.h>

#include <fck_db.h>

static void purge_files(const char *pattern)
{
	char            **paths;
	const fckc_size_t count = os->glob->executable(pattern, &paths);
	for (fckc_size_t index = 0; index < count; index++)
	{
		char *path = paths[index];
		os->fs->remove(path);
	}
	os->glob->free(paths);
}

static fck_api_registry *fck_api_registry_load(const char *path)
{
	// This badboy needs to get released
	const fck_shared_object so       = os->so->load(path);
	fck_load_func          *loader   = (fck_load_func *)os->so->symbol(so, "fck_api_load");
	fck_api_registry       *registry = (fck_api_registry *)loader(NULL, NULL);
	return registry;
}

static fck_plugins_api *fck_plugins_load(fck_api_registry *registry, const char *path)
{
	// This badboy needs to get released
	const fck_shared_object so      = os->so->load(path);
	fck_load_func          *loader  = (fck_load_func *)os->so->symbol(so, "fck_plugins_load");
	fck_plugins_api        *plugins = (fck_plugins_api *)loader(registry, NULL);
	return plugins;
}

static void load_config(int argc, char **argv)
{
	for (int index = 0; index < argc; index++)
	{
		char *value = argv[index];
		os->io->log(value);
	}
}

typedef struct app_screen
{
	float width;
	float height;
	float sprite_width;
	float sprite_height;
} app_screen;

typedef struct app_sprite_pie_items
{
	fck_nk_pie_item remove;

	fck_nk_pie_item duplicate;
	fck_nk_pie_item duplicate_left;
	fck_nk_pie_item duplicate_right;
	fck_nk_pie_item duplicate_up;
	fck_nk_pie_item duplicate_down;

	fck_nk_pie_item add;
	fck_nk_pie_item add_bird;
	fck_nk_pie_item add_item;
} app_sprite_pie_items;

// typedef struct app_sprite_component
//{
//	fck_sprite_id id;
// } app_sprite_component;

static void app_sprite_pie_items_init(fck_nuklear_api *nk, app_sprite_pie_items *items, fck_nk_pie_item *root)
{
	memset(items, 0, sizeof(*items));

	const app_sprite_pie_items initial = {
		.remove.name          = "Delete",
		.duplicate.name       = "Duplicate",
		.duplicate_left.name  = "<",
		.duplicate_right.name = ">",
		.duplicate_up.name    = "^",
		.duplicate_down.name  = "V",
		.add.name             = "Add",
		.add_bird.name        = "Bird",
		.add_item.name        = "Item",
	};
	*items = initial;

	nk->pie->push(root, &items->duplicate);
	nk->pie->push(root, &items->remove);
	nk->pie->push(root, &items->add);

	nk->pie->push(&items->duplicate, &items->duplicate_left);
	nk->pie->push(&items->duplicate, &items->duplicate_right);
	nk->pie->push(&items->duplicate, &items->duplicate_up);
	nk->pie->push(&items->duplicate, &items->duplicate_down);

	nk->pie->push(&items->add, &items->add_bird);
	nk->pie->push(&items->add, &items->add_item);
}

static void fck_sprite_transform_property(fck_nuklear_api *nk, fck_nk view, fck_sprite_transform *transform)
{
	transform->x                = nk->element->f32(view, "x", -1280.0f, transform->x, 1280.0f, 1.0f);
	transform->y                = nk->element->f32(view, "y", -720.0f, transform->y, 720.0f, 1.0f);
	transform->z                = nk->element->f32(view, "z", 0.0f, transform->z, 1.0f, 0.1f);
	transform->rotation         = nk->element->f32(view, "rotation", 0.0f, transform->rotation, 360.0f, 1.0f);
	transform->scale            = nk->element->f32(view, "scale", 1.0f, transform->scale, 100.0f, 1.0f);
	transform->horizontal_index = nk->element->i32(view, "horizontal index", 0, transform->horizontal_index, 10, 1);
	transform->vertical_index   = nk->element->i32(view, "vertical index", 0, transform->vertical_index, 10, 1);
}

const char *fck_get_theme_name(fck_nuklear_theme theme_name)
{
	switch (theme_name)
	{
	case fck_nk_theme_white:
		return "White";
	case fck_nk_theme_ruta:
		return "Perfect";
	case fck_nk_theme_red:
		return "Red";
	case fck_nk_theme_blue:
		return "Blue";
	case fck_nk_theme_dark:
		return "Dark";
	case fck_nk_theme_dracula:
		return "Dracula";
	case fck_nk_theme_latte:
		return "Latte";
	case fck_nk_theme_frappe:
		return "Frappe";
	case fck_nk_theme_macchiato:
		return "Macchiato";
	case fck_nk_theme_mocha:
		return "Mocha";
	case fck_nk_theme_count:
		return "";
	}
	return "";
	// todo add something to scream here
}

static void fck_settings_editor(fck_sprite_api *sprite, fck_nuklear_api *nk, fck_nk view, fck_sprites *sprites)
{
	return;
	const fck_sprite_batch_id batch_id = sprite->batches->find_by_name(sprites, "Items");
	const fck_nk_rect         source   = {16.0f, 16.0f, 16.0f, 16.0f};
	if (nk->panel->begin_icon(view, "Settings", sprite->batches->image_view(sprites, batch_id), &source, 800.0f))
	{
		if (nk->panel->push(view, "Appearance"))
		{
			if (nk->panel->push(view, "Theme"))
			{
				const char *component_names[fck_nk_theme_count];
				for (int i = 0; i < fck_nk_theme_count; ++i)
				{
					component_names[i] = fck_get_theme_name((fck_nuklear_theme)i);
				}

				fck_nuklear_theme current_theme = nk->get_theme(view);
				const int         new_index     = nk->element->dropdown(view, current_theme, component_names, fck_nk_theme_count);
				nk->set_theme(view, (fck_nuklear_theme)new_index);
				nk->panel->pop(view);
			}
			nk->panel->pop(view);
		}
		nk->panel->end(view);
	}
}

static void fck_nk_db_object_configure(fck_db_api *db, fck_db assets, fck_nuklear_api *nk, fck_nk view, fck_texture_api *texture,
                                       sht_driver *driver, const char *name, fck_db_id id)
{
	const fck_db_accessor reader = db->object->read(assets, id);
	fckc_u32              offset = 0;
	fck_db_named_property named_property;

	if (nk->panel->push(view, "%s (%u)", name, id.index))
	{
		while (reader.read->iterate(reader, &offset, &named_property))
		{
			const char            *property_name = named_property.name;
			const fck_db_property *property      = &named_property.value;
			switch (property->type)
			{
			case fck_db_type_none:
				break;
			case fck_db_type_i32:
				nk->element->label(view, "%s: %f", property_name, property->i32);
				break;
			case fck_db_type_f32:
				nk->element->label(view, "%s: %f", property_name, property->f32);
				break;
			case fck_db_type_memory:
				// Maybe display bytes like a madman for debugging reasons
				nk->element->label(view, "%s: <memory>");
				break;
			case fck_db_type_asset:
				nk->element->preview(view, property->asset);
				break;
			case fck_db_type_reference:
			case fck_db_type_object:
				fck_nk_db_object_configure(db, assets, nk, view, texture, driver, property_name, property->object);
				break;
			case fck_db_type_string:
				nk->element->label(view, "%s: %s", property_name, property->string);
				break;
			case fck_db_type_object_set:
				if (nk->panel->push(view, "%s (%llu)", property_name, db->set->count(property->set)))
				{
					fckc_size_t count = 0;
					fck_db_id  *child = NULL;
					char        buffer[512];
					while (db->set->iterate(property->set, &child))
					{
						(void)snprintf(buffer, sizeof(buffer), "%s[%lu]", property_name, to_u32(count));
						fck_nk_db_object_configure(db, assets, nk, view, texture, driver, buffer, *child);
						count++;
					}
					nk->panel->pop(view);
				}
				break;
			}
		}

		nk->panel->pop(view);
	}
}

fck_entity fck_create_entity_sprite(const char *item_name, fck_ec_api *ec, fck_ec world, fck_sprite_api *sprite, fck_sprites *sprites,
                                    fck_nuklear_api *nk, fck_nk view, fck_component_id sprite_component_id)
{
	const fck_sprite_transform baseline = {
		.scale = 10.0f,
	};

	const fck_entity          entity    = ec->entity->create(world);
	const fck_sprite_batch_id batch_id  = sprite->batches->find_by_name(sprites, item_name);
	fck_sprite_transform     *transform = sprite->add(sprites, batch_id);
	*transform                          = baseline;
	nk->pie->apply_position(view, &transform->x, &transform->y);

	const fck_sprite_id sprite_id = sprite->indexof(sprites, batch_id, transform);
	ec->component->set(world, entity, sprite_component_id, &sprite_id);

	nk->set_selection(view, transform);
	return entity;
}

static void fck_sprite_transform_editor(fck_ec_api *ec, fck_ec world, fck_plugins_api *plugins, fck_sprite_api *sprite, fck_nuklear_api *nk,
                                        fck_nk view, app_sprite_pie_items *pie, fck_sprites *sprites, fck_entity *selected_entity,
                                        fck_db assets, fck_db_api *db)
{
	const int is_selected_entity_ok = ec->entity->is_ok(world, *selected_entity);
	if (!is_selected_entity_ok)
	{
		*selected_entity = ec->entity->invalid(world);
	}

	const char   **sprite_batch_names;
	const fckc_u32 sprite_batch_names_count = sprite->batches->names(sprites, &sprite_batch_names);

	const fck_component_id sprite_component_id = ec->registry->id(world, "sprite");

	if (nk->panel->begin_label(view, "Core Panel", 300.0f))
	{
		if (nk->panel->push(view, "Plugins"))
		{
			if (nk->panel->push(view, "Loaded Plugins"))
			{
				const char *current = NULL;
				while ((current = plugins->loaded(current)))
				{
					if (nk->element->button(view, current))
					{
						plugins->unload(current);
						break;
					}
				}
				nk->panel->pop(view);
			}

			if (nk->panel->push(view, "Unloaded Plugins"))
			{
				const char *current = NULL;
				while ((current = plugins->unloaded(current)))
				{
					if (nk->element->button(view, current))
					{
						plugins->load(current);
						break;
					}
				}
				nk->panel->pop(view);
			}
			nk->panel->pop(view);
		}

		if (nk->panel->push(view, "Entities"))
		{
			const fck_entity *entities;
			const fckc_u32    count = ec->entity->all(world, &entities);
			for (fckc_u32 index = 0; index < count; index++)
			{
				const fck_entity entity = entities[index];
				if (nk->panel->push(view, "Entity[%u]", entity.index))
				{
					{
						fck_archetype_iterator it = ec->archetype->iterator(world, entity);
						fck_component_id       component_id;
						while (ec->archetype->get(&it, &component_id, 1))
						{
							if (nk->panel->push(view, "%s [%u]", ec->registry->nameof(world, component_id), entity.index))
							{
								if (sprite_component_id.value == component_id.value)
								{
									void                 *opaque_component = ec->component->get(world, entity, component_id);
									fck_sprite_id        *sprite_component = (fck_sprite_id *)opaque_component;
									fck_sprite_transform *transform        = sprite->get(sprites, *sprite_component);
									fck_assert(transform);
									const fck_sprite_id sprite_id = *sprite_component;

									nk->element->label(view, "Batch: %lu - Sprite: %lu", sprite_id.batch.value, sprite_id.entry.value);

									nk->element->button_image(view, sprite->batches->image_view(sprites, sprite_id.batch), 128.0f);

									const int as_int    = to_int(sprite_id.batch.value);
									const int new_index = nk->element->dropdown(view, as_int, sprite_batch_names, sprite_batch_names_count);

									if (as_int != new_index)
									{
										const fck_sprite_transform copy = *transform;
										if (sprite->is_ok(sprites, sprite_id))
										{
											if (sprite->remove(sprites, sprite_id))
											{
												const fck_sprite_batch_id new_id = sprite->batches->index(sprites, new_index);
												fck_assert(sprite->batches->is_ok(sprites, new_id));
												fck_sprite_transform *transform = sprite->add(sprites, new_id);
												*transform                      = copy;
												transform->horizontal_index = transform->vertical_index = 0;
												*sprite_component = sprite->indexof(sprites, new_id, transform);
											}
										}
									}

									fck_sprite_transform_property(nk, view, transform);
								}

								if (nk->element->button(view, "Remove Component"))
								{
									os->io->log("Remove Component");
									ec->component->remove(world, entity, component_id);
								}

								nk->panel->pop(view);
							}
						}
					}

					fck_component_names_iterator name_it = ec->registry->iterator(world);
					const char                  *component_names[16];
					const fckc_u32 names_result = ec->registry->names(&name_it, &component_names[0], fck_arraysize(component_names));

					for (int i = 0; i < names_result; i++)
					{
						const char            *component_name = component_names[i];
						const fck_component_id component_id   = ec->registry->id(world, component_name);
						char                   title[256];
						if (!ec->component->get(world, entity, component_id))
						{
							snprintf(title, sizeof(title), "Add %s component", component_name);
							if (nk->element->button(view, title))
							{
								os->io->log("Add Component");
								ec->component->add(world, entity, component_id);
							}
						}
					}

					if (nk->element->button(view, "Remove Entity"))
					{
						os->io->log("Remove Entity");
						ec->entity->destroy(world, entity);
					}

					nk->panel->pop(view);
				}
			}
			if (nk->element->button(view, "Add Entity"))
			{
				os->io->log("Add Entity");

				ec->entity->create(world);
			}
			nk->panel->pop(view);

			if (nk->element->button(view, "Save to Disk"))
			{
				fck_serialiser *writer = serialiser_json->writer(kll->system);
				for (fckc_u32 index = 0; index < count; index++)
				{
					const fck_entity entity = entities[index];
					writer->push(writer, "entity");
					writer->u32(writer, "index", &index, 1);

					fck_archetype_iterator it = ec->archetype->iterator(world, entity);
					fck_component_id       component_id;
					while (ec->archetype->get(&it, &component_id, 1))
					{
						if (sprite_component_id.value == component_id.value)
						{
							void                 *opaque_component = ec->component->get(world, entity, component_id);
							fck_sprite_id        *sprite_component = (fck_sprite_id *)opaque_component;
							fck_sprite_transform *transform        = sprite->get(sprites, *sprite_component);
							fck_assert(transform);

							writer->push(writer, "sprite");
							writer->f32(writer, "x", &transform->x, 1);
							writer->f32(writer, "y", &transform->y, 1);
							writer->pop(writer);
						}
					}

					writer->pop(writer);
				}

				{
					char *buffer = (char *)writer->buffer(writer);

					{
						fck_file file = os->fs->open("fck_some_data.json", "w");
						os->fs->write(file, buffer, strlen(buffer));
						os->fs->close(file);
					}

					os->io->log("%s", buffer);
				}
			}
		}

		if (nk->panel->push(view, "Sprite Transforms"))
		{
			const fckc_u32 batch_count = sprite->batches->count(sprites);
			for (fckc_u32 batch_index = 0; batch_index < batch_count; batch_index++)
			{
				const fck_sprite_batch_id id = sprite->batches->index(sprites, batch_index);
				fck_assert(sprite->batches->is_ok(sprites, id));

				fck_sprite_transform *transforms = NULL;
				const fckc_u32        count      = sprite->transforms(sprites, id, &transforms);
				const char           *batch_name = sprite->batches->nameof(sprites, id);
				if (nk->panel->push(view, batch_name, count))
				{
					for (fckc_u32 index = 0; index < count; index++)
					{
						if (nk->panel->push(view, "%s[%d]", batch_name, index))
						{
							fck_sprite_transform *transform = transforms + index;
							fck_sprite_transform_property(nk, view, transform);
							nk->panel->pop(view);
						}
					}
					nk->panel->pop(view);
				}
			}
			nk->panel->pop(view);
		}
		nk->panel->end(view);
	}

	{
		const fck_nk_colour on  = {0, 255, 0, 255};
		const fck_nk_colour off = {255, 0, 0, 255};

		const fck_entity *entities;
		const fckc_u32    count = ec->entity->all(world, &entities);
		for (fckc_u32 index = 0; index < count; index++)
		{
			const fck_entity entity = entities[index];
			void            *opaque = ec->component->get(world, entity, sprite_component_id);
			if (opaque)
			{
				fck_sprite_id        *sprite_component = (fck_sprite_id *)opaque;
				fck_sprite_transform *transform        = sprite->get(sprites, *sprite_component);
				fck_assert(transform);

				float sprite_width  = 0;
				float sprite_height = 0;
				sprite->batches->dimensions(sprites, sprite_component->batch, &sprite_width, &sprite_height);
				if (nk->select(view, transform, transform->x, transform->y, sprite_width * transform->scale,
				               sprite_height * transform->scale, on))
				{
					*selected_entity = entity;
				}
				if (nk->control_point(view, transform, &transform->x, &transform->y, 16.0f, on, off))
				{
					// break;
				}
			}
		}
	}

	{
		if (nk->pie->happened(&pie->add_bird))
		{
			*selected_entity = fck_create_entity_sprite("Birds", ec, world, sprite, sprites, nk, view, sprite_component_id);
		}

		if (nk->pie->happened(&pie->add_item))
		{
			*selected_entity = fck_create_entity_sprite("Items", ec, world, sprite, sprites, nk, view, sprite_component_id);
		}

		{
			if (nk->pie->happened(&pie->remove) && is_selected_entity_ok)
			{
				ec->entity->destroy(world, *selected_entity);
				nk->set_selection(view, NULL);
			}

			if (nk->pie->happened(&pie->duplicate) && is_selected_entity_ok)
			{
				const fck_entity copy      = ec->entity->copy(world, *selected_entity);
				fck_sprite_id   *component = (fck_sprite_id *)ec->component->get(world, copy, sprite_component_id);
				if (component)
				{
					fck_sprite_transform *transform = sprite->get(sprites, *component);
					nk->pie->apply_position(view, &transform->x, &transform->y);
					nk->set_selection(view, transform);
				}
			}
			if (nk->pie->happened(&pie->duplicate_left) && is_selected_entity_ok)
			{
				const fck_entity copy      = ec->entity->copy(world, *selected_entity);
				fck_sprite_id   *component = (fck_sprite_id *)ec->component->get(world, copy, sprite_component_id);
				if (component)
				{
					float sprite_width  = 0;
					float sprite_height = 0;
					sprite->batches->dimensions(sprites, component->batch, &sprite_width, &sprite_height);
					fck_sprite_transform *transform = sprite->get(sprites, *component);
					transform->x                    = transform->x - (sprite_width * transform->scale);
					nk->set_selection(view, transform);
				}
			}

			if (nk->pie->happened(&pie->duplicate_right) && is_selected_entity_ok)
			{
				const fck_entity copy      = ec->entity->copy(world, *selected_entity);
				fck_sprite_id   *component = (fck_sprite_id *)ec->component->get(world, copy, sprite_component_id);
				if (component)
				{
					float sprite_width  = 0;
					float sprite_height = 0;
					sprite->batches->dimensions(sprites, component->batch, &sprite_width, &sprite_height);
					fck_sprite_transform *transform = sprite->get(sprites, *component);
					transform->x                    = transform->x + (sprite_width * transform->scale);
					nk->set_selection(view, transform);
				}
			}
			if (nk->pie->happened(&pie->duplicate_up) && is_selected_entity_ok)
			{
				const fck_entity copy      = ec->entity->copy(world, *selected_entity);
				fck_sprite_id   *component = (fck_sprite_id *)ec->component->get(world, copy, sprite_component_id);
				if (component)
				{
					float sprite_width  = 0;
					float sprite_height = 0;
					sprite->batches->dimensions(sprites, component->batch, &sprite_width, &sprite_height);
					fck_sprite_transform *transform = sprite->get(sprites, *component);
					transform->y                    = transform->y - (sprite_height * transform->scale);
					nk->set_selection(view, transform);
				}
			}
			if (nk->pie->happened(&pie->duplicate_down) && is_selected_entity_ok)
			{
				const fck_entity copy      = ec->entity->copy(world, *selected_entity);
				fck_sprite_id   *component = (fck_sprite_id *)ec->component->get(world, copy, sprite_component_id);
				if (component)
				{
					float sprite_width  = 0;
					float sprite_height = 0;
					sprite->batches->dimensions(sprites, component->batch, &sprite_width, &sprite_height);
					fck_sprite_transform *transform = sprite->get(sprites, *component);
					transform->y                    = transform->y + (sprite_height * transform->scale);
					nk->set_selection(view, transform);
				}
			}
		}
	}
}

typedef struct app_sprite_implementation
{
	fck_sprite_api *sprite;
	fck_sprites    *sprites;
} app_sprite_implementation;

static void *app_sprite_implementation_constructor(void *self, void *userdata)
{
	app_sprite_implementation *impl      = (app_sprite_implementation *)userdata;
	fck_sprite_id             *component = (fck_sprite_id *)self;
	const fck_sprite_batch_id  id        = impl->sprite->batches->index(impl->sprites, 0);
	fck_sprite_transform      *transform = impl->sprite->add(impl->sprites, id);
	transform->scale                     = 10.0f;
	*component                           = impl->sprite->indexof(impl->sprites, id, transform);
	return component;
}

static void *app_sprite_implementation_destructor(void *self, void *userdata)
{
	app_sprite_implementation *impl      = (app_sprite_implementation *)userdata;
	fck_sprite_id             *component = (fck_sprite_id *)self;
	const int                  result    = impl->sprite->remove(impl->sprites, *component);
	fck_assert(result);
	return component;
}

static void *app_sprite_implementation_copy(void *dst, const void *src, void *userdata)
{
	app_sprite_implementation *impl                  = (app_sprite_implementation *)userdata;
	const fck_sprite_id       *source_component      = (fck_sprite_id *)src;
	fck_sprite_id             *destination_component = (fck_sprite_id *)dst;

	fck_sprite_transform *destination_transform = impl->sprite->add(impl->sprites, source_component->batch);
	fck_sprite_transform *source_transform      = impl->sprite->get(impl->sprites, *source_component);
	*destination_transform                      = *source_transform;

	*destination_component = impl->sprite->indexof(impl->sprites, source_component->batch, destination_transform);
	return dst;
}

typedef struct app_gameloop
{
	fck_gameloop            o;
	fck_gameloop_interface *i;
} app_gameloop;

typedef struct app_gameloops
{
	app_gameloop *values;
	fckc_size_t   count;
	fckc_size_t   capacity;
} app_gameloops;

static app_gameloop *app_gameloops_add(kll_allocator *allocator, app_gameloops *loops)
{
	if (loops->count >= loops->capacity)
	{
		const fckc_size_t capacity = loops->capacity ? loops->capacity * 2 : 8;
		const fckc_size_t total    = capacity * sizeof(*loops->values);
		app_gameloop     *values   = (app_gameloop *)kll_malloc(allocator, total);
		if (loops->values)
		{
			memcpy(values, loops->values, loops->count * sizeof(*loops->values));
			kll_free(allocator, loops->values);
		}
		loops->capacity = capacity;
		loops->values   = values;
	}

	const fckc_size_t index = loops->count;
	loops->count            = loops->count + 1;
	app_gameloop *current   = loops->values + index;
	memset(current, 0, sizeof(*current));
	return current;
}

int main(int argc, char **argv)
{
	// TODO: We need to setup stable editor entities, or something like that
	load_config(argc, argv);

	purge_files("temp-fck-*");

	fck_api_registry *registry = fck_api_registry_load("fck-api" fck_plugin_extension);
	fck_plugins_api  *plugins  = fck_plugins_load(registry, "fck-plugins" fck_plugin_extension);

	plugins->root(os->fs->executable());

	const char *current = NULL;
	while ((current = plugins->unloaded(current)))
	{
		plugins->load(current);
	}

	current = NULL;
	while ((current = plugins->loaded(current)))
	{
		plugins->load(current);
		fck_assert(os->so->is_ok(*plugins->so(current)));
	}

	fck_input       *input   = (fck_input *)registry->find(fck_input_api_name);
	sht_render_api  *render  = (sht_render_api *)registry->find(sht_render_api_name);
	fck_texture_api *png     = (fck_texture_api *)registry->find(fck_texture_api_name);
	fck_nuklear_api *nk      = (fck_nuklear_api *)registry->find(fck_nuklear_api_name);
	fck_gfx_api     *gfx     = (fck_gfx_api *)registry->find(fck_gfx_api_name);
	fck_sprite_api  *sprite  = (fck_sprite_api *)registry->find(fck_sprite_api_name);
	fck_ec_api      *ec      = (fck_ec_api *)registry->find(fck_ec_api_name);
	fck_db_api      *db      = (fck_db_api *)registry->find(fck_db_api_name);
	fck_texture_api *texture = (fck_texture_api *)registry->find(fck_texture_api_name);

	fck_assert(input);
	fck_assert(render);
	fck_assert(png);
	fck_assert(nk);
	fck_assert(gfx);
	fck_assert(ec);
	fck_assert(db);

	fck_db assets = db->create(kll->system);

	db->asset->setup(assets, "app", fck_resource_path);

	app_gameloops loops = {0};
	// We can create a new ec
	fck_ec        world = ec->core->create(kll->system, 32);
	{
		fck_gameloop_interface             **gameloops;
		const fckc_size_t                    gameloops_count = registry->implementations(fck_gameloop_interface_name, (void ***)&gameloops);
		const fck_gameloop_create_parameters create_parameters = {.apis = registry, .ec = ec, .state = &world};
		for (fckc_size_t index = 0; index < gameloops_count; index++)
		{
			fck_gameloop_interface *gameloop_interface = gameloops[index];
			app_gameloop           *loop               = app_gameloops_add(kll->system, &loops);

			loop->o = gameloop_interface->create(kll->system, &create_parameters);
			loop->i = gameloop_interface;
		}
	}

	fck_input_source *mouse = NULL;
	{
		fck_input_source **sources;
		const fckc_size_t  count = input->sources(&sources);
		for (fckc_size_t index = 0; index < count; index++)
		{
			fck_input_source *source = sources[index];
			if (source->type == fck_input_source_mouse)
			{
				mouse = source;
				break;
			}
		}
	}
	fck_assert(mouse);

	const char *title  = "FCK Application";
	fck_window  window = os->win->create(title, 1280, 720);

	int window_width, window_height;
	os->win->size(window, &window_width, &window_height);

	// We have to do this a bit smarter... Maybe not now
	// os->win->text_input_start(window);
	os->io->log("General Data Setup");
	const sht_instance instance = render->load(sht_header_version);
	if (!render->is_ok(instance))
	{
		return 0;
	}
	os->io->log("Trying to setup renderer...");

	sht_driver driver = instance.vt->start(instance, &window);
	if (!instance.vt->is_ok(driver))
	{
		return 0;
	}
	sht_memory            *memory    = driver.vt->memory(driver);
	const sht_swapchain    swapchain = driver.vt->swapchain(driver);
	sht_command_buffer_vt *command   = driver.vt->command_buffer;

	const fck_nuklear_create_args nuklear_create_args = {.driver = &driver, .window = &window, .db = &assets};
	fck_nk                        view                = nk->create(kll->system, &nuklear_create_args);
	nk->set_theme(view, fck_nk_theme_ruta);

	fck_nk_hamburger_item help_menu_item = {
		.type = fck_nk_hamburger_item_button,
		.name = "Help",
	};
	fck_nk_hamburger_item about_menu_item = {
		.type = fck_nk_hamburger_item_button,
		.name = "About",
	};
	fck_nk_hamburger_item setting_menu_item = {
		.type = fck_nk_hamburger_item_bool,
		.name = "Setting",
	};

	nk->hamburger->push(view, &help_menu_item);
	nk->hamburger->push(view, &about_menu_item);
	nk->hamburger->push(view, &setting_menu_item);

	fck_nk_pie_item     *root = nk->pie->root(view);
	app_sprite_pie_items sprite_pie;
	app_sprite_pie_items_init(nk, &sprite_pie, root);

	sht_image      depth_image = {0};
	sht_image_view depth_view  = {0};
	{
		const sht_extent              extent = swapchain.vt->extent(swapchain);
		const sht_image_configuration config = (sht_image_configuration){
			.format   = sht_format_d16_unorm,
			.width    = to_u32(extent.width),
			.height   = to_u32(extent.height),
			.transfer = sht_transfer_retained,
			.usage    = sht_image_usage_depth_stencil_attachment,
		};

		depth_image = memory->image->create(memory->bump, &config, sht_memory_gpu);
		depth_view  = memory->image->view(memory->bump, depth_image, sht_format_undefined);
	}

	const fck_sprite_create_args sprite_create_args = {.db = &assets, .driver = &driver};
	fck_sprites                  sprites            = sprite->create(kll->system, &sprite_create_args);

	//{
	//	const fck_db_id       gfx_infra = db->object->create(assets);
	//	const fck_db_accessor editor    = db->object->edit(assets, gfx_infra);
	//	db->asset->get
	//}

	app_sprite_implementation      sprite_implementation = {.sprite = sprite, .sprites = &sprites};
	const fck_component_id         sprite_id             = ec->registry->declare(world, "sprite", sizeof(fck_sprite_id));
	const fck_component_definition sprite_definition     = {
			.constructor = app_sprite_implementation_constructor,
			.destructor  = app_sprite_implementation_destructor,
			.copy        = app_sprite_implementation_copy,
			.userdata    = &sprite_implementation,
    };
	ec->registry->define(world, sprite_id, &sprite_definition);

	fckc_u64 time_point = os->chrono->ms();

	const fck_db_asset *ass = db->asset->find(assets, "app/test.json");
	const fck_db_id sprite_batches_id = db->object->resolve(ass);

	//const fck_db_id sprite_batches_id = db->object->create(assets);
	fck_entity      selected_entity   = ec->entity->invalid(world);
	int             is_running        = 1;
	while (is_running)
	{
		os->chrono->sleep(4);

		const fck_nk_control control = nk->control(view);
		if (control.close)
		{
			is_running = 0;
		}

		if (control.minimise)
		{
			os->win->minimise(window);
		}

		const fckc_u64 now   = os->chrono->ms();
		const fckc_u64 delta = now - time_point;
		time_point           = now;

		// TODO: Make render-vk hotreloadable :)
		// How hard can it be?
		plugins->hotreload();
		db->asset->hotreload(assets);

		nk->input->begin(view);

		fck_input_event   events[128] = {0};
		const fckc_size_t result      = input->events(events, fck_arraysize(events));
		for (fckc_size_t index = 0; index < result; index++)
		{
			fck_input_event *e = events + index;
			if (e->source->type == fck_input_source_keyboard)
			{
				switch (e->description->id)
				{
				case fck_pkey_escape:
					is_running = 0;
					break;
				default:
					break;
				}
				continue;
			}
		}

		nk->input->events(view, events, result);
		nk->input->end(view);

		{
			if (control.body == 0)
			{
				const fck_gameloop_tick_parameters tick_parameters = {.apis = registry, .ec = ec, .state = &world, .sprites = &sprites};
				for (fckc_size_t index = 0; index < loops.count; index++)
				{
					app_gameloop *gameloop = loops.values + index;
					gameloop->i->tick(gameloop->o, &tick_parameters);
				}
			}
		}

		{
			if (nk->begin(view))
			{
				if (nk->panel->begin_label(view, "assets", 400.0f))
				{
					fck_db_category_iterator it = {0};
					if (nk->panel->push(view, "Example"))
					{
						while (db->asset->categories(assets, &it))
						{
							if (nk->panel->push(view, "%s", it.name))
							{
								void       *extension = NULL;
								const char *ext;
								while (db->asset->extensions(assets, it.handle, &extension, &ext))
								{
									if (nk->panel->push(view, ".%s", ext))
									{
										const fck_db_asset_reference *references;
										const fckc_size_t             count = db->asset->assetsof(assets, ext, &references);
										for (fckc_size_t index = 0; index < count; index++)
										{
											const fck_db_asset_reference *ref   = references + index;
											const fck_db_asset           *asset = db->asset->get(assets, ref->id, it.name);
											nk->element->preview(view, asset);
										}
										nk->panel->pop(view);
									}
								}
								nk->panel->pop(view);
							}
						}
						nk->panel->pop(view);
					}
					nk->panel->end(view);
				}

				if (nk->panel->begin_label(view, "Loops", 400.f))
				{
					if (nk->panel->push(view, "Game Loops"))
					{
						for (fckc_size_t index = 0; index < loops.count; index++)
						{
							app_gameloop *gameloop = loops.values + index;
							nk->element->button(view, gameloop->i->name);
						}
						nk->panel->pop(view);
					}
					nk->panel->end(view);
				}

				fck_sprite_transform_editor(ec, world, plugins, sprite, nk, view, &sprite_pie, &sprites, &selected_entity, assets, db);

				const fck_gameloop_edit_parameters edit_parameters = {.apis = registry, .ec = ec, .state = &world, .view = &view, .nk = nk};
				for (fckc_size_t index = 0; index < loops.count; index++)
				{
					app_gameloop *gameloop = loops.values + index;
					gameloop->i->edit(gameloop->o, &edit_parameters);
				}
				fck_settings_editor(sprite, nk, view, &sprites);

				if (nk->panel->begin_label(view, "sprites", 400.0f))
				{
					if (nk->panel->push(view, "editor"))
					{
						static const fck_db_asset *asset       = NULL;
						static fckc_i32            cw          = 16;
						static fckc_i32            ch          = 16;
						static char                buffer[512] = {0};

						asset = nk->element->asset(view, &assets, asset, fck_category_texture);
						if (asset)
						{
							nk->element->preview(view, asset);

							cw = nk->element->i32(view, "sprite width", 0, cw, 256, 1);
							ch = nk->element->i32(view, "sprite height", 0, ch, 256, 1);
							nk->element->string(view, buffer, sizeof(buffer));
							const fck_sprite_batch_id batch = sprite->batches->find_by_name(&sprites, buffer);
							if (!sprite->batches->is_ok(&sprites, batch))
							{
								if (nk->element->button(view, "add"))
								{
									sprite->batches->add(&sprites, buffer, asset, cw, ch);
								}
							}
						}

						if (nk->element->button(view, "save to disk"))
						{
							const fckc_u32 sprite_count = sprite->batches->count(&sprites);
							fck_db_id_set *set          = db->set->create(kll->system, sprite_count);

							for (fckc_u32 index = 0; index < sprite_count; index++)
							{
								const fck_sprite_batch_id batch_id = sprite->batches->index(&sprites, index);
								const char               *name     = sprite->batches->nameof(&sprites, batch_id);
								float                     sprite_width, sprite_height;
								const int       result   = sprite->batches->dimensions(&sprites, batch_id, &sprite_width, &sprite_height);
								const fck_db_id child_id = db->object->create(assets);
								const fck_db_accessor child_editor = db->object->edit(assets, child_id);
								const fck_db_asset   *asset        = sprite->batches->asset(&sprites, batch_id);
								child_editor.edit->string(child_editor, "name", name);
								child_editor.edit->asset(child_editor, "texture", asset);
								child_editor.edit->f32(child_editor, "sprite_width", sprite_width);
								child_editor.edit->f32(child_editor, "sprite_height", sprite_width);
								child_editor.edit->commit(child_editor, fck_db_no_undo);
								db->set->add(NULL, &set, child_id);
							}

							const fck_db_accessor editor = db->object->edit(assets, sprite_batches_id);
							editor.edit->set(editor, "batches", set);
							editor.edit->commit(editor, fck_db_no_undo);
							db->set->destroy(kll->system, set);

							db->object->save(assets, sprite_batches_id, "app", "test");
						}

						if (db->object->is_ok(assets, sprite_batches_id))
						{
							fck_nk_db_object_configure(db, assets, nk, view, texture, &driver, "fck-sprite", sprite_batches_id);
						}

						nk->panel->pop(view);
					}

					nk->panel->end(view);
				}
			}
			nk->end(view);
		}

		memory->reset(memory->temp);

		{
			// Texture resolution
			void *it_category = db->asset->category(assets, fck_category_texture);

			void       *it_extension = NULL;
			const char *extension;
			while (db->asset->extensions(assets, it_category, &it_extension, &extension))
			{
				const fck_db_asset_reference *references;
				const fckc_size_t             count = db->asset->assetsof(assets, extension, &references);
				for (fckc_size_t index = 0; index < count; index++)
				{
					const fck_db_asset_reference *ref   = references + index;
					const fck_db_asset           *asset = db->asset->get(assets, ref->id, fck_category_texture);
					if (*asset->state == fck_db_asset_state_requested)
					{
						texture->asset->resolve(asset, &driver);
					}
				}
			}
		}

		const sht_swapchain_state sc = swapchain.vt->wait_and_acquire(swapchain);
		if (swapchain.vt->is_ok(swapchain, &sc))
		{
			const sht_command_buffer command_buffer = command->acquire(driver, sc.index);
			if (command->is_ok(command_buffer))
			{
				const fck_nk_colour colour = nk->get_style_colour(FCK_NK_COLOR_WINDOW);

				sht_render_desc desc = {
					.colour = {.view        = sc.view,
				               .load_op     = sht_clear,
				               .store_op    = sht_store,
				               .clear_value = {colour.r / 255.0f, colour.g / 255.0f, colour.b / 255.0f, 1.0f}},
					.depth  = {.view = depth_view, .load_op = sht_clear, .store_op = sht_dont_care},
				};

				const fck_gfx_args args = {
					.commands   = &command_buffer,
					.driver     = &driver,
					.swapchain  = &sc,
					.suggestion = &desc,
				};

				sprite->present(&sprites, &args);

				desc.colour.load_op = sht_load;
				desc.depth.load_op  = sht_load;

				nk->present(&view, &args);

				command->submit(command_buffer, sht_queue_graphic);
			}
		}
		else
		{
			if (sc.resize)
			{
				const sht_extent extent = swapchain.vt->extent(swapchain);
				memory->image->recreate(memory->bump, &depth_image, extent, &depth_view, 1);
			}
		}
	}

	plugins->shutdown();
	purge_files("temp-fck-*");

	os->win->destroy(window);

	return 0;
}
