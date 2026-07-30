
#include "fck_serialiser.h"
#include "fck_serialiser_json.h"

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
	char **paths;
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
	const fck_shared_object so = os->so->load(path);
	fck_load_func *loader = (fck_load_func *)os->so->symbol(so, "fck_api_load");
	fck_api_registry *registry = (fck_api_registry *)loader(NULL, NULL);
	return registry;
}

static fck_plugins_api *fck_plugins_load(fck_api_registry *registry, const char *path)
{
	// This badboy needs to get released
	const fck_shared_object so = os->so->load(path);
	fck_load_func *loader = (fck_load_func *)os->so->symbol(so, "fck_plugins_load");
	fck_plugins_api *plugins = (fck_plugins_api *)loader(registry, NULL);
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
		.remove.name = "Delete",
		.duplicate.name = "Duplicate",
		.duplicate_left.name = "<",
		.duplicate_right.name = ">",
		.duplicate_up.name = "^",
		.duplicate_down.name = "V",
		.add.name = "Add",
		.add_bird.name = "Bird",
		.add_item.name = "Item",
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
	transform->x = nk->elements->f32(view, "x", -1280.0f, transform->x, 1280.0f, 1.0f);
	transform->y = nk->elements->f32(view, "y", -720.0f, transform->y, 720.0f, 1.0f);
	transform->z = nk->elements->f32(view, "z", 0.0f, transform->z, 1.0f, 0.1f);
	transform->rotation = nk->elements->f32(view, "rotation", 0.0f, transform->rotation, 360.0f, 1.0f);
	transform->scale = nk->elements->f32(view, "scale", 1.0f, transform->scale, 100.0f, 1.0f);
	transform->horizontal_index = nk->elements->i32(view, "horizontal index", 0, transform->horizontal_index, 10, 1);
	transform->vertical_index = nk->elements->i32(view, "vertical index", 0, transform->vertical_index, 10, 1);
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
	const fck_sprite_batch_id batch_id = sprite->batches->find_by_name(sprites, "Items");
	const fck_nk_rect source = {16.0f, 16.0f, 16.0f, 16.0f};
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
				const int new_index = nk->elements->dropdown(view, current_theme, component_names, fck_nk_theme_count);
				nk->set_theme(view, (fck_nuklear_theme)new_index);
				nk->panel->pop(view);
			}
			nk->panel->pop(view);
		}
		nk->panel->end(view);
	}
}

fck_entity fck_create_entity_sprite(const char *item_name, fck_ec_api *ec, fck_ec world, fck_sprite_api *sprite, fck_sprites *sprites,
                                    fck_nuklear_api *nk, fck_nk view, fck_component_id sprite_component_id)
{
	const fck_sprite_transform baseline = {
		.scale = 10.0f,
	};

	const fck_entity entity = ec->entity->create(world);
	const fck_sprite_batch_id batch_id = sprite->batches->find_by_name(sprites, item_name);
	fck_sprite_transform *transform = sprite->add(sprites, batch_id);
	*transform = baseline;
	nk->pie->apply_position(view, &transform->x, &transform->y);

	const fck_sprite_id sprite_id = sprite->indexof(sprites, batch_id, transform);
	ec->component->set(world, entity, sprite_component_id, &sprite_id);

	nk->set_selection(view, transform);
	return entity;
}

static void fck_sprite_transform_editor(fck_ec_api *ec, fck_ec world, fck_plugins_api *plugins, fck_sprite_api *sprite, fck_nuklear_api *nk,
                                        fck_nk view, app_sprite_pie_items *pie, fck_sprites *sprites, fck_entity *selected_entity,
                                        fck_db assets, fck_db_api *db, fck_db_id item)
{
	const int is_selected_entity_ok = ec->entity->is_ok(world, *selected_entity);
	if (!is_selected_entity_ok)
	{
		*selected_entity = ec->entity->invalid(world);
	}

	const char **sprite_batch_names;
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
					if (nk->elements->button(view, current))
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
					if (nk->elements->button(view, current))
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
			const fckc_u32 count = ec->entity->all(world, &entities);
			for (fckc_u32 index = 0; index < count; index++)
			{
				const fck_entity entity = entities[index];
				if (nk->panel->push(view, "Entity[%u]", entity.index))
				{
					{
						fck_archetype_iterator it = ec->archetype->iterator(world, entity);
						fck_component_id component_id;
						while (ec->archetype->get(&it, &component_id, 1))
						{
							if (nk->panel->push(view, "%s [%u]", ec->registry->nameof(world, component_id), entity.index))
							{
								if (sprite_component_id.value == component_id.value)
								{
									void *opaque_component = ec->component->get(world, entity, component_id);
									fck_sprite_id *sprite_component = (fck_sprite_id *)opaque_component;
									fck_sprite_transform *transform = sprite->get(sprites, *sprite_component);
									fck_assert(transform);
									const fck_sprite_id sprite_id = *sprite_component;

									nk->elements->label(view, "Batch: %lu - Sprite: %lu", sprite_id.batch.value, sprite_id.entry.value);

									nk->elements->button_image(view, sprite->batches->image_view(sprites, sprite_id.batch));

									const int as_int = to_int(sprite_id.batch.value);
									const int new_index =
										nk->elements->dropdown(view, as_int, sprite_batch_names, sprite_batch_names_count);
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
												*transform = copy;
												transform->horizontal_index = transform->vertical_index = 0;
												*sprite_component = sprite->indexof(sprites, new_id, transform);
											}
										}
									}

									fck_sprite_transform_property(nk, view, transform);
								}

								if (nk->elements->button(view, "Remove Component"))
								{
									os->io->log("Remove Component");
									ec->component->remove(world, entity, component_id);
								}

								nk->panel->pop(view);
							}
						}
					}

					fck_component_names_iterator name_it = ec->registry->iterator(world);
					const char *component_names[16];
					const fckc_u32 names_result = ec->registry->names(&name_it, &component_names[0], fck_arraysize(component_names));

					for (int i = 0; i < names_result; i++)
					{
						const char *component_name = component_names[i];
						const fck_component_id component_id = ec->registry->id(world, component_name);
						char title[256];
						if (!ec->component->get(world, entity, component_id))
						{
							snprintf(title, sizeof(title), "Add %s component", component_name);
							if (nk->elements->button(view, title))
							{
								os->io->log("Add Component");
								ec->component->add(world, entity, component_id);
							}
						}
					}

					if (nk->elements->button(view, "Remove Entity"))
					{
						os->io->log("Remove Entity");
						ec->entity->destroy(world, entity);
					}

					nk->panel->pop(view);
				}
			}
			if (nk->elements->button(view, "Add Entity"))
			{
				os->io->log("Add Entity");

				ec->entity->create(world);
			}
			nk->panel->pop(view);

			if (nk->elements->button(view, "Save to Disk"))
			{
				fck_serialiser *writer = serialiser_json->writer(kll->system);

				for (fckc_u32 index = 0; index < count; index++)
				{
					const fck_entity entity = entities[index];
					writer->push(writer, "entity");
					writer->u32(writer, "index", &index, 1);

					fck_archetype_iterator it = ec->archetype->iterator(world, entity);
					fck_component_id component_id;
					while (ec->archetype->get(&it, &component_id, 1))
					{
						if (sprite_component_id.value == component_id.value)
						{
							void *opaque_component = ec->component->get(world, entity, component_id);
							fck_sprite_id *sprite_component = (fck_sprite_id *)opaque_component;
							fck_sprite_transform *transform = sprite->get(sprites, *sprite_component);
							fck_assert(transform);

							writer->push(writer, "sprite");
							writer->f32(writer, "x", &transform->x, 1);
							writer->f32(writer, "y", &transform->y, 1);
							writer->pop(writer);
						}
					}

					writer->pop(writer);
				}

				// const fck_db_accessor reader = db->object->read(assets, item);
				// float x = reader.read->f32(reader, "x");
				// float y = reader.read->f32(reader, "y");
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
				const fckc_u32 count = sprite->transforms(sprites, id, &transforms);
				const char *batch_name = sprite->batches->nameof(sprites, id);
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
		const fck_nk_colour on = {0, 255, 0, 255};
		const fck_nk_colour off = {255, 0, 0, 255};

		const fck_entity *entities;
		const fckc_u32 count = ec->entity->all(world, &entities);
		for (fckc_u32 index = 0; index < count; index++)
		{
			const fck_entity entity = entities[index];
			void *opaque = ec->component->get(world, entity, sprite_component_id);
			if (opaque)
			{
				fck_sprite_id *sprite_component = (fck_sprite_id *)opaque;
				fck_sprite_transform *transform = sprite->get(sprites, *sprite_component);
				fck_assert(transform);

				float sprite_width = 0;
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
				const fck_entity copy = ec->entity->copy(world, *selected_entity);
				fck_sprite_id *component = (fck_sprite_id *)ec->component->get(world, copy, sprite_component_id);
				if (component)
				{
					fck_sprite_transform *transform = sprite->get(sprites, *component);
					nk->pie->apply_position(view, &transform->x, &transform->y);
					nk->set_selection(view, transform);
				}
			}
			if (nk->pie->happened(&pie->duplicate_left) && is_selected_entity_ok)
			{
				const fck_entity copy = ec->entity->copy(world, *selected_entity);
				fck_sprite_id *component = (fck_sprite_id *)ec->component->get(world, copy, sprite_component_id);
				if (component)
				{
					float sprite_width = 0;
					float sprite_height = 0;
					sprite->batches->dimensions(sprites, component->batch, &sprite_width, &sprite_height);
					fck_sprite_transform *transform = sprite->get(sprites, *component);
					transform->x = transform->x - (sprite_width * transform->scale);
					nk->set_selection(view, transform);
				}
			}

			if (nk->pie->happened(&pie->duplicate_right) && is_selected_entity_ok)
			{
				const fck_entity copy = ec->entity->copy(world, *selected_entity);
				fck_sprite_id *component = (fck_sprite_id *)ec->component->get(world, copy, sprite_component_id);
				if (component)
				{
					float sprite_width = 0;
					float sprite_height = 0;
					sprite->batches->dimensions(sprites, component->batch, &sprite_width, &sprite_height);
					fck_sprite_transform *transform = sprite->get(sprites, *component);
					transform->x = transform->x + (sprite_width * transform->scale);
					nk->set_selection(view, transform);
				}
			}
			if (nk->pie->happened(&pie->duplicate_up) && is_selected_entity_ok)
			{
				const fck_entity copy = ec->entity->copy(world, *selected_entity);
				fck_sprite_id *component = (fck_sprite_id *)ec->component->get(world, copy, sprite_component_id);
				if (component)
				{
					float sprite_width = 0;
					float sprite_height = 0;
					sprite->batches->dimensions(sprites, component->batch, &sprite_width, &sprite_height);
					fck_sprite_transform *transform = sprite->get(sprites, *component);
					transform->y = transform->y - (sprite_height * transform->scale);
					nk->set_selection(view, transform);
				}
			}
			if (nk->pie->happened(&pie->duplicate_down) && is_selected_entity_ok)
			{
				const fck_entity copy = ec->entity->copy(world, *selected_entity);
				fck_sprite_id *component = (fck_sprite_id *)ec->component->get(world, copy, sprite_component_id);
				if (component)
				{
					float sprite_width = 0;
					float sprite_height = 0;
					sprite->batches->dimensions(sprites, component->batch, &sprite_width, &sprite_height);
					fck_sprite_transform *transform = sprite->get(sprites, *component);
					transform->y = transform->y + (sprite_height * transform->scale);
					nk->set_selection(view, transform);
				}
			}
		}
	}
}

typedef struct app_sprite_implementation
{
	fck_sprite_api *sprite;
	fck_sprites *sprites;
} app_sprite_implementation;

static void *app_sprite_implementation_constructor(void *self, void *userdata)
{
	app_sprite_implementation *impl = (app_sprite_implementation *)userdata;
	fck_sprite_id *component = (fck_sprite_id *)self;
	const fck_sprite_batch_id id = impl->sprite->batches->index(impl->sprites, 0);
	fck_sprite_transform *transform = impl->sprite->add(impl->sprites, id);
	transform->scale = 10.0f;
	*component = impl->sprite->indexof(impl->sprites, id, transform);
	return component;
}

static void *app_sprite_implementation_destructor(void *self, void *userdata)
{
	app_sprite_implementation *impl = (app_sprite_implementation *)userdata;
	fck_sprite_id *component = (fck_sprite_id *)self;
	const int result = impl->sprite->remove(impl->sprites, *component);
	fck_assert(result);
	return component;
}

static void *app_sprite_implementation_copy(void *dst, const void *src, void *userdata)
{
	app_sprite_implementation *impl = (app_sprite_implementation *)userdata;
	const fck_sprite_id *source_component = (fck_sprite_id *)src;
	fck_sprite_id *destination_component = (fck_sprite_id *)dst;

	fck_sprite_transform *destination_transform = impl->sprite->add(impl->sprites, source_component->batch);
	fck_sprite_transform *source_transform = impl->sprite->get(impl->sprites, *source_component);
	*destination_transform = *source_transform;

	*destination_component = impl->sprite->indexof(impl->sprites, source_component->batch, destination_transform);
	return dst;
}

typedef struct app_gameloop
{
	fck_gameloop o;
	fck_gameloop_interface *i;
} app_gameloop;

typedef struct app_gameloops
{
	app_gameloop *values;
	fckc_size_t count;
	fckc_size_t capacity;
} app_gameloops;

static app_gameloop *app_gameloops_add(kll_allocator *allocator, app_gameloops *loops)
{
	if (loops->count >= loops->capacity)
	{
		const fckc_size_t capacity = loops->capacity ? loops->capacity * 2 : 8;
		const fckc_size_t total = capacity * sizeof(*loops->values);
		app_gameloop *values = (app_gameloop *)kll_malloc(allocator, total);
		if (loops->values)
		{
			memcpy(values, loops->values, loops->count * sizeof(*loops->values));
			kll_free(allocator, loops->values);
		}
		loops->capacity = capacity;
		loops->values = values;
	}

	const fckc_size_t index = loops->count;
	loops->count = loops->count + 1;
	app_gameloop *current = loops->values + index;
	memset(current, 0, sizeof(*current));
	return current;
}

int main(int argc, char **argv)
{
	// TODO: We need to setup stable editor entities, or something like that
	load_config(argc, argv);

	purge_files("temp-fck-*");

	fck_api_registry *registry = fck_api_registry_load("fck-api" fck_plugin_extension);
	fck_plugins_api *plugins = fck_plugins_load(registry, "fck-plugins" fck_plugin_extension);

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
	}

	fck_input *input = (fck_input *)registry->find(fck_input_api_name);
	sht_render_api *render = (sht_render_api *)registry->find(sht_render_api_name);
	fck_texture_api *png = (fck_texture_api *)registry->find(fck_texture_api_name);
	fck_nuklear_api *nk = (fck_nuklear_api *)registry->find(fck_nuklear_api_name);
	fck_gfx_api *gfx = (fck_gfx_api *)registry->find(fck_gfx_api_name);
	fck_sprite_api *sprite = (fck_sprite_api *)registry->find(fck_sprite_api_name);
	fck_ec_api *ec = (fck_ec_api *)registry->find(fck_ec_api_name);
	fck_db_api *db = (fck_db_api *)registry->find(fck_db_api_name);
	fck_assert(input);
	fck_assert(render);
	fck_assert(png);
	fck_assert(nk);
	fck_assert(gfx);
	fck_assert(ec);
	fck_assert(db);

	fck_db assets = db->create(kll->system);

	const fck_db_undo_scope undo = db->undo->create(kll->system);

	fck_db_id item = db->object->create(assets, "Test");
	db->setup(assets, "app", fck_resource_path);

	app_gameloops loops = {0};
	// We can create a new ec
	fck_ec world = ec->core->create(kll->system, 32);
	{
		fck_gameloop_interface **gameloops;
		const fckc_size_t gameloops_count = registry->implementations(fck_gameloop_interface_name, (void ***)&gameloops);
		const fck_gameloop_create_parameters create_parameters = {.apis = registry, .ec = ec, .state = &world};
		for (fckc_size_t index = 0; index < gameloops_count; index++)
		{
			fck_gameloop_interface *gameloop_interface = gameloops[index];
			app_gameloop *loop = app_gameloops_add(kll->system, &loops);

			loop->o = gameloop_interface->create(kll->system, &create_parameters);
			loop->i = gameloop_interface;
		}
	}

	fck_input_source *mouse = NULL;
	{
		fck_input_source **sources;
		const fckc_size_t count = input->sources(&sources);
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

	fck_window window = os->win->create("FCK Application", 1280, 720);
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
	sht_memory *memory = driver.vt->memory(driver);
	const sht_swapchain swapchain = driver.vt->swapchain(driver);
	sht_command_buffer_vt *command = driver.vt->command_buffer;

	fck_nk view = nk->create(kll->system, &window, &driver);
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

	fck_nk_pie_item *root = nk->pie->root(view);
	app_sprite_pie_items sprite_pie;
	app_sprite_pie_items_init(nk, &sprite_pie, root);

	// fck_db_element* txt_asset = (fck_db_element*)db->get(assets, "app/configuration/config.txt");
	fck_texture_asset *debug_png_asset = (fck_texture_asset *)db->asset->find(assets, "app/debug.png");
	fck_texture_asset *bg_png_asset = (fck_texture_asset *)db->asset->find(assets, "app/bg-mockup.png");
	fck_texture_asset *bird_png_asset = (fck_texture_asset *)db->asset->find(assets, "app/bird-sheet.png");
	fck_texture_asset *items_png_asset = (fck_texture_asset *)db->asset->find(assets, "app/items-sheet.png");
	fck_shader_asset *sprite_vs = (fck_shader_asset *)db->asset->find(assets, "app/sprite.vs");
	fck_shader_asset *sprite_fs = (fck_shader_asset *)db->asset->find(assets, "app/sprite.fs");

	sht_image depth_image = {0};
	sht_image_view depth_view = {0};
	{
		const sht_extent extent = swapchain.vt->extent(swapchain);
		const sht_image_configuration config = (sht_image_configuration){
			.format = sht_format_d16_unorm,
			.width = to_u32(extent.width),
			.height = to_u32(extent.height),
			.transfer = sht_transfer_retained,
			.usage = sht_image_usage_depth_stencil_attachment,
		};

		depth_image = memory->image->create(memory->bump, &config, sht_memory_gpu);
		depth_view = memory->image->view(memory->bump, depth_image, sht_format_undefined);
	}

	fck_sprites sprites = sprite->create(kll->system, &assets, &driver);

	app_sprite_implementation sprite_implementation = {.sprite = sprite, .sprites = &sprites};
	const fck_component_id sprite_id = ec->registry->declare(world, "sprite", sizeof(fck_sprite_id));
	const fck_component_definition sprite_definition = {
		.constructor = app_sprite_implementation_constructor,
		.destructor = app_sprite_implementation_destructor,
		.copy = app_sprite_implementation_copy,
		.userdata = &sprite_implementation,
	};
	ec->registry->define(world, sprite_id, &sprite_definition);

	// TODO: Add empty inline in sprites. Sprites has access to sht
	// Then we could also move the whole render pass there?
	// const fck_sprite_batch_id empty_batch = sprite->batches->add(&sprites, "Empty", &white_view, 32.0f, 32.0f);
	// fck_assert(empty_batch.value == 0);
	const fck_sprite_batch_id debug_batch = sprite->batches->add(&sprites, "Debug", debug_png_asset, 8.0f, 8.0f);
	const fck_sprite_batch_id background_batch = sprite->batches->add(&sprites, "Background", bg_png_asset, 132.0f, 72.0f);
	const fck_sprite_batch_id birds_batch = sprite->batches->add(&sprites, "Birds", bird_png_asset, 32.0f, 32.0f);
	const fck_sprite_batch_id items_batch = sprite->batches->add(&sprites, "Items", items_png_asset, 16.0f, 16.0f);

	fckc_u64 time_point = os->chrono->ms();

	{
		const fck_db_accessor accessor = db->object->edit(assets, item);

		accessor.edit->asset(accessor, "texture", to_fck_db_asset(debug_png_asset));
		accessor.edit->f32(accessor, "x", 10.0f);
		accessor.edit->f32(accessor, "y", 10.0f);
		accessor.edit->reference(accessor, "self", item);

		accessor.edit->commit(accessor, fck_db_no_undo);

		const fck_db_accessor reader = db->object->read(assets, item);
		os->io->log("Version: %lu", reader.read->version(reader));

		fckc_u32 it = 0;
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
			case fck_db_type_memory:
			case fck_db_type_asset:
			case fck_db_type_reference:
				os->io->log("Name: %s", property.name);
				break;
			}
		}
	}

	fck_entity selected_entity = ec->entity->invalid(world);
	int is_running = 1;
	while (is_running)
	{
		os->chrono->sleep(16);

		const fck_nk_control control = nk->control(view);
		if (control.close)
		{
			is_running = 0;
		}

		if (control.minimise)
		{
			os->win->minimise(window);
		}

		const fckc_u64 now = os->chrono->ms();
		const fckc_u64 delta = now - time_point;
		time_point = now;
		// TODO: Make render-vk hotreloadable :)
		// How hard can it be?
		plugins->hotreload();
		db->hotreload(assets);

		nk->input->begin(view);

		fck_input_event events[128] = {0};
		const fckc_size_t result = input->events(events, fck_arraysize(events));
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
			const fck_gameloop_tick_parameters tick_parameters = {.apis = registry, .ec = ec, .state = &world, .sprites = &sprites};
			for (fckc_size_t index = 0; index < loops.count; index++)
			{
				app_gameloop *gameloop = loops.values + index;
				gameloop->i->tick(gameloop->o, &tick_parameters);
			}
		}

		{
			if (nk->begin(view))
			{
				if (nk->panel->begin_label(view, "db", 400.f))
				{
					if (nk->panel->push(view, "Example"))
					{
						{
							const fck_db_accessor reader = db->object->read(assets, item);

							nk->elements->i32(view, "Version", 0, reader.read->version(reader), 65000, 1);
							fckc_u32 it = 0;
							fck_db_named_property property = {0};
							while (reader.read->iterate(reader, &it, &property))
							{
								fck_db_property previous = {0};
								memcpy(&previous, &property.value, sizeof(previous));
								switch (property.value.type)
								{
								case fck_db_type_none:
									break;
								case fck_db_type_i32:
									property.value.i32 = nk->elements->i32(view, property.name, -1000, property.value.i32, 1000, 1.0f);
									break;
								case fck_db_type_f32:
									property.value.f32 =
										nk->elements->f32(view, property.name, -1000.0f, property.value.f32, 1000.0f, 1.0f);
									break;
								case fck_db_type_memory:
								case fck_db_type_reference:
								case fck_db_type_asset:
									nk->elements->label(view, "<NO DISPLAY>");
									break;
								}

								if (memcmp(&previous, &property.value, sizeof(previous)) != 0)
								{
									os->io->log("Changed");
									const fck_db_accessor accessor = db->object->edit(assets, item);
									accessor.edit->variant(accessor, property.name, &property.value);
									accessor.edit->commit(accessor, undo);
								}
							}
						}

						//if (nk->elements->button(view, "Save to Disk")) //
						//{
						//	const fck_db_accessor reader = db->object->read(assets, item);
						//	float x = reader.read->f32(reader, "x");
						//	float y = reader.read->f32(reader, "y");

						//	fck_serialiser *writer = serialiser_json->writer(kll->system);
						//	const char *name = "position";
						//	writer->push(writer, name);
						//	name = "x";
						//	writer->f32(writer, name, &x, 1);
						//	name = "y";
						//	writer->f32(writer, name, &y, 1);
						//	writer->pop(writer);

						//	char *buffer = (char *)writer->buffer(writer);

						//	{
						//		fck_file file = os->fs->open("fck_some_data.json", "w");
						//		os->fs->write(file, buffer, strlen(buffer));
						//		os->fs->close(file);
						//	}
						//	{
						//		fck_file file = os->fs->open("fck_some_data.json", "r");
						//		const fckc_i64 size = os->fs->size(file);
						//		void *memory = kll_malloc(kll->system, size);
						//		os->fs->read(file, memory, size);

						//		fck_serialiser *jreader = serialiser_json->reader(kll->system, (fckc_char *)memory, size);
						//		x = (float)jreader->query(jreader, "/position/x")->values->as_f64;
						//		y = (float)jreader->query(jreader, "/position/y")->values->as_f64;

						//		kll_free(kll->system, memory);
						//	}

						//	os->io->log("%s", buffer);
						//}

						if (nk->elements->button(view, "Undo"))
						{
							if (db->undo->undo(assets, undo))
							{
								os->io->log("Undid");
							}
						}
						if (nk->elements->button(view, "Redo"))
						{
							if (db->undo->redo(assets, undo))
							{
								os->io->log("Redid");
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
							nk->elements->button(view, gameloop->i->name);
						}
						nk->panel->pop(view);
					}
					nk->panel->end(view);
				}

				fck_sprite_transform_editor(ec, world, plugins, sprite, nk, view, &sprite_pie, &sprites, &selected_entity, assets, db,
				                            item);

				const fck_gameloop_edit_parameters edit_parameters = {.apis = registry, .ec = ec, .state = &world, .view = &view, .nk = nk};
				for (fckc_size_t index = 0; index < loops.count; index++)
				{
					app_gameloop *gameloop = loops.values + index;
					gameloop->i->edit(gameloop->o, &edit_parameters);
				}
				fck_settings_editor(sprite, nk, view, &sprites);
			}
			nk->end(view);
		}

		memory->reset(memory->temp);

		fckc_u32 frame_index;
		const sht_image_view color_target = swapchain.vt->wait_and_acquire(swapchain, &frame_index);
		if (swapchain.vt->is_ok(swapchain, frame_index))
		{
			const sht_command_buffer command_buffer = command->acquire(driver, frame_index);
			if (command->is_ok(command_buffer))
			{
				{
					const fck_nk_colour colour = nk->get_style_colour(FCK_NK_COLOR_WINDOW);

					sht_render_desc desc = {
						.colour = {.view = color_target,
					               .load_op = sht_clear,
					               .store_op = sht_store,
					               .clear_value = {colour.r / 255.0f, colour.g / 255.0f, colour.b / 255.0f, 1.0f}},
						.depth = {.view = depth_view, .load_op = sht_clear, .store_op = sht_dont_care},
					};

					const sht_render_pass render_pass = command->render_pass->begin(command_buffer, &desc);
					if (command->render_pass->is_ok(render_pass))
					{
						sprite->present(&sprites, &command_buffer, frame_index);
						command->render_pass->end(command_buffer);
					}
				}

				{
					// Could this render pass live in nk->present directly?
					sht_render_desc desc = {.colour = {.view = color_target, .load_op = sht_load, .store_op = sht_store}};
					const sht_render_pass render_pass = command->render_pass->begin(command_buffer, &desc);
					if (command->render_pass->is_ok(render_pass))
					{
						nk->present(view, &command_buffer, frame_index);
						command->render_pass->end(command_buffer);
					}
				}

				command->submit(command_buffer, sht_queue_graphic);
			}
		}
		else
		{
			if (frame_index == sht_swapchain_needs_resize)
			{
				// NOTE: Maybe only resize if we get larger and never shrink?
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
