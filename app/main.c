
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

typedef struct fck_sprite_component
{
	// A sprite sheet config could be sick :) We gotta see!!
	fck_batchy_batch_configuration configuration;
	fck_sprite_transform           transform;
} fck_sprite_component;

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

static int fck_nk_db_object_configure(fck_db_api *db, fck_db assets, fck_nuklear_api *nk, fck_nk view, fck_texture_api *texture,
                                      sht_driver *driver, const char *name, fck_db_id id)
{
	const fck_db_accessor reader = db->object->read(assets, id);
	fckc_u32              offset = 0;
	fck_db_named_property named_property;

	int save = 0;
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
			case fck_db_type_asset: {
				nk->element->preview(view, property->asset);
				const fck_db_asset *asset = nk->element->asset(view, &assets, property->asset, fck_category_texture);
				if (asset != property->asset)
				{
					const fck_db_accessor editor = db->object->edit(assets, id);
					editor.edit->asset(editor, property_name, asset);
					editor.edit->commit(editor, fck_db_no_undo);
					save = 1;
				}
				break;
			}
			case fck_db_type_reference:
			case fck_db_type_object:
				save = fck_nk_db_object_configure(db, assets, nk, view, texture, driver, property_name, property->object) || save;
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
						save = fck_nk_db_object_configure(db, assets, nk, view, texture, driver, buffer, *child) || save;
						count++;
					}
					nk->panel->pop(view);
				}
				break;
			}
		}

		nk->panel->pop(view);
	}
	return save;
}

static void fck_sprite_transform_editor(fck_ec_api *ec, fck_ec world, fck_plugins_api *plugins, fck_nuklear_api *nk, fck_nk view,
                                        app_sprite_pie_items *pie, fck_entity *selected_entity, fck_db assets, fck_db_api *db)
{
	const int is_selected_entity_ok = ec->entity->is_ok(world, *selected_entity);
	if (!is_selected_entity_ok)
	{
		*selected_entity = ec->entity->invalid(world);
	}

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
		}

		if (nk->panel->push(view, "Sprite Transforms"))
		{
			nk->panel->pop(view);
		}
		nk->panel->end(view);
	}

	{
		const fck_component_id id = ec->registry->id(world, fck_nameof(fck_sprite_component));

		const fck_nk_colour on  = {0, 255, 0, 255};
		const fck_nk_colour off = {255, 0, 0, 255};

		const fck_entity *entities;
		const fckc_u32    count = ec->entity->all(world, &entities);
		for (fckc_u32 index = 0; index < count; index++)
		{
			const fck_entity entity = entities[index];
			void            *opaque = ec->component->get(world, entity, id);
			if (opaque)
			{
				fck_sprite_component *component = (fck_sprite_component *)opaque;
				const float           x         = component->transform.x;
				const float           y         = component->transform.y;
				const float           w         = component->configuration.sprite_width * component->transform.scale;
				const float           h         = component->configuration.sprite_height * component->transform.scale;

				if (nk->select(view, component, component->transform.x, component->transform.y, w, h, on))
				{
					*selected_entity = entity;
				}
				if (nk->control_point(view, component, &component->transform.x, &component->transform.y, 16.0f, on, off))
				{
					// break;
				}
			}
		}
	}

	{
		{
			if (nk->pie->happened(&pie->remove) && is_selected_entity_ok)
			{
				ec->entity->destroy(world, *selected_entity);
				nk->set_selection(view, NULL);
			}

			if (nk->pie->happened(&pie->duplicate) && is_selected_entity_ok)
			{
			}
			if (nk->pie->happened(&pie->duplicate_left) && is_selected_entity_ok)
			{
			}

			if (nk->pie->happened(&pie->duplicate_right) && is_selected_entity_ok)
			{
			}
			if (nk->pie->happened(&pie->duplicate_up) && is_selected_entity_ok)
			{
			}
			if (nk->pie->happened(&pie->duplicate_down) && is_selected_entity_ok)
			{
			}
		}
	}
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
	fck_ec_api      *ec      = (fck_ec_api *)registry->find(fck_ec_api_name);
	fck_db_api      *db      = (fck_db_api *)registry->find(fck_db_api_name);
	fck_texture_api *texture = (fck_texture_api *)registry->find(fck_texture_api_name);
	fck_batchy_api  *batchy  = (fck_batchy_api *)registry->find(fck_batchy_api_name);

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

	// TODO: idof
	ec->registry->declare(world, fck_nameof(fck_sprite_component), sizeof(fck_sprite_component));

	// TODO: Batches work - Clean it up, ship it!!!!!!!
	const fck_batchy_create_args batchy_create_args = {.db = &assets, .driver = &driver};
	fck_batchy                   bchy               = batchy->create(kll->system, &batchy_create_args);
	/*const fck_batchy_batch_configuration bird_sprite_config = {
	    .asset         = db->asset->lazy(assets, "app/bird-sheet", fck_category_texture),
	    .sprite_width  = 32.0f,
	    .sprite_height = 32.0f,
	};
	fck_batchy_batch bird_batch = batchy->add(bchy, &bird_sprite_config);
	for (fckc_u32 index = 0; index < 32; index++)
	{
	    fck_sprite_transform *bird_transform = batchy->batchup(bchy, bird_batch);
	    bird_transform->horizontal_index     = 0;
	    bird_transform->vertical_index       = 0;
	    bird_transform->rotation             = 0.0f;
	    bird_transform->scale                = 4.0f;
	    bird_transform->x                    = 32.0f * index - 620.0f;
	    bird_transform->y                    = 64.0f;
	    bird_transform->z                    = 0.25f;
	}

	const fck_batchy_batch_configuration item_sprite_config = {
	    .asset         = db->asset->lazy(assets, "app/items-sheet", fck_category_texture),
	    .sprite_width  = 16.0f,
	    .sprite_height = 16.0f,
	};
	fck_batchy_batch item_batch = batchy->add(bchy, &item_sprite_config);
	for (fckc_u32 index = 0; index < 64; index++)
	{
	    fck_sprite_transform *bird_transform = batchy->batchup(bchy, item_batch);
	    bird_transform->horizontal_index     = index % 2;
	    bird_transform->vertical_index       = 0;
	    bird_transform->rotation             = 0.0f;
	    bird_transform->scale                = 4.0f;
	    bird_transform->x                    = 16.0f * index - 620.0f;
	    bird_transform->y                    = 0.0f;
	    bird_transform->z                    = 0.25f;
	}*/

	fckc_u64            time_point = os->chrono->ms();
	const fck_db_asset *ass        = db->asset->find(assets, "app/test.json");

	// const fck_db_id sprite_batches_id = db->object->create(assets);
	fck_entity selected_entity = ec->entity->invalid(world);
	int        is_running      = 1;
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
				const fck_gameloop_tick_parameters tick_parameters = {
					.apis  = registry,
					.ec    = ec,
					.state = &world,
				};
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
										const fckc_size_t             count = db->asset->all_of(assets, ext, &references);
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

				fck_sprite_transform_editor(ec, world, plugins, nk, view, &sprite_pie, &selected_entity, assets, db);

				const fck_gameloop_edit_parameters edit_parameters = {.apis = registry, .ec = ec, .state = &world, .view = &view, .nk = nk};
				for (fckc_size_t index = 0; index < loops.count; index++)
				{
					app_gameloop *gameloop = loops.values + index;
					gameloop->i->edit(gameloop->o, &edit_parameters);
				}
			}
			nk->end(view);
		}

		{
			const fck_batchy_batch *batch = NULL;
			while (batchy->iterate(bchy, &batch))
			{
				batchy->clear(bchy, *batch);
			}
		}

		{
			typedef struct fck_sprite_query
			{
				fck_sprite_component *sprite;
			} fck_sprite_query;

			fck_query_component components[] = {
				[0] = {.name = fck_nameof(fck_sprite_component), .offset = offsetof(fck_sprite_query, sprite)},
			};

			fck_query_description desc = {
				.name       = fck_nameof(fck_sprite_query),
				.components = components,
				.count      = fck_arraysize(components),
			};

			const fck_query_id query = ec->query->get(world, &desc);
			fck_query_iterator it    = ec->query->iterator(world, query);
			fck_sprite_query   results[16];
			fckc_u32           count = 0;
			while ((count = ec->query->match(&it, results, fck_arraysize(results))))
			{
				for (fckc_u32 index = 0; index < count; index++)
				{
					fck_sprite_query *query_result = results + index;
					query_result->sprite->configuration.asset = db->asset->find(assets, "app/bird-sheet.png");
					query_result->sprite->configuration.sprite_height = 32.0f;
					query_result->sprite->configuration.sprite_width = 32.0f;
					
					const fck_batchy_batch batch = batchy->add(bchy, &query_result->sprite->configuration);
					fck_sprite_transform* transform = batchy->batchup(bchy, batch);
					query_result->sprite->transform.z = 0.25f;
					query_result->sprite->transform.scale = 4.0f;
					*transform = query_result->sprite->transform;
				}
			}
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
				const fckc_size_t             count = db->asset->all_of(assets, extension, &references);
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

				batchy->present(&bchy, &args);

				desc.colour.load_op = sht_load;
				desc.depth.load_op  = sht_dont_care;

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

	db->close(assets);

	plugins->shutdown();
	purge_files("temp-fck-*");

	os->win->destroy(window);

	return 0;
}
