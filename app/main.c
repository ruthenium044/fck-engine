
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

#include <fck_png.h>

#include <fck_gfx.h>
#include <fck_nuklear.h>

#include <fck_sprite.h>

// #pragma optimize("", off)

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

static sht_image app_load_image(sht_driver driver, const void *pixels, sht_format format, int width, int height)
{
	sht_memory *memory = driver.vt->memory(driver);
	const sht_image_configuration config = {
		.format = format,
		.height = to_u32(height),
		.width = to_u32(width),
		.transfer = sht_transfer_target,
		.usage = sht_image_usage_sampled,
	};

	sht_image image = memory->image->create(memory->bump, &config, sht_memory_gpu);
	if (!memory->image->is_ok(&image))
	{
		return image;
	}
	const fckc_size_t size = (fckc_size_t)width * height * 4;
	driver.vt->upload_image(driver, &image, pixels, size);
	return image;
}

typedef struct fck_gameloop
{
	void *handle;
} fck_gameloop;

typedef struct fck_gameloop_interface
{
	fck_gameloop (*create)(void);
	void (*destroy)(fck_gameloop loop);

	int (*edit)(fck_gameloop);
	int (*tick)(fck_gameloop);
} fck_gameloop_interface;

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
	transform->width = nk->elements->f32(view, "width", 0.0f, transform->width, 256.0f, 4.0f);
	transform->height = nk->elements->f32(view, "height", 0.0f, transform->height, 256.0f, 4.0f);
	transform->rotation = nk->elements->f32(view, "rotation", 0.0f, transform->rotation, 360.0f, 1.0f);
	transform->scale = nk->elements->f32(view, "scale", 1.0f, transform->scale, 100.0f, 1.0f);
	transform->horizontal_index = nk->elements->i32(view, "horizontal index", 0, transform->horizontal_index, 10, 1);
	transform->vertical_index = nk->elements->i32(view, "vertical index", 0, transform->vertical_index, 10, 1);
}

static void fck_sprite_transform_editor(fck_plugins_api *plugins, fck_sprite_api *sprite, fck_nuklear_api *nk, fck_nk view,
                                        app_sprite_pie_items *pie, fck_sprites *sprites, fck_sprite_id *selected_sprite_id)
{
	nk->panel->begin(view, "Core Panel", 300.0f);
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

		if (nk->panel->push(view, "Sprite Transforms"))
		{
			fck_sprite_transform *selected_transform = sprite->get(sprites, *selected_sprite_id);
			if (selected_transform)
			{
				fck_sprite_transform *transform = selected_transform;
				if (nk->panel->push(view, "Selection"))
				{
					fck_sprite_transform_property(nk, view, transform);
					nk->panel->pop(view);
				}
			}

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
							const char **names;
							const fckc_u32 names_count = sprite->batches->names(sprites, &names);
							const int new_index = nk->elements->dropdown(view, (int)batch_index, names, names_count);
							if (new_index != batch_index)
							{
								const fck_sprite_transform copy = transforms[index];
								const fck_sprite_id sprite_id = sprite->indexof(sprites, id, transforms + index);
								if (sprite->is_ok(sprites, sprite_id))
								{
									if (sprite->remove(sprites, sprite_id))
									{
										const fck_sprite_batch_id new_id = sprite->batches->index(sprites, batch_index);
										fck_assert(sprite->batches->is_ok(sprites, new_id));
										fck_sprite_transform *transform = sprite->add(sprites, new_id);
										*transform = copy;
										transform->horizontal_index = transform->vertical_index = 0;
									}
								}
							}
							else
							{
								fck_sprite_transform *transform = transforms + index;
								fck_sprite_transform_property(nk, view, transform);
							}
							nk->panel->pop(view);
						}
					}
					nk->panel->pop(view);
				}
			}
			nk->panel->pop(view);
		}
	}
	nk->panel->end(view);

	{
		*selected_sprite_id = sprite->invalid();
		const fck_nk_colour on = {0, 255, 0, 255};
		const fck_nk_colour off = {255, 0, 0, 255};

		const fckc_u32 batch_count = sprite->batches->count(sprites);
		for (fckc_u32 batch_index = 0; batch_index < batch_count; batch_index++)
		{
			const fck_sprite_batch_id id = {.value = batch_index};
			fck_sprite_transform *transforms = NULL;
			const fckc_u32 count = sprite->transforms(sprites, id, &transforms);

			for (fckc_size_t index = 0; index < count; index++)
			{
				fck_sprite_transform *transform = transforms + index;
				if (nk->select(view, transform, transform->x, transform->y, transform->width, transform->height, on))
				{
					*selected_sprite_id = sprite->indexof(sprites, id, transforms + index);
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
			const fck_sprite_batch_id id = sprite->batches->find_by_name(sprites, "Birds");
			fck_sprite_transform *transform = sprite->add(sprites, id);
			// Pie api is a bit clunky
			const fck_sprite_transform baseline = {
				.scale = 1.0f,
				.width = 256.0f,
				.height = 256.0f,
			};
			*transform = baseline;
			nk->pie->apply_position(view, &transform->x, &transform->y);
			nk->set_selection(view, transform);
		}

		if (nk->pie->happened(&pie->add_item))
		{
			const fck_sprite_batch_id id = sprite->batches->find_by_name(sprites, "Items");
			fck_sprite_transform *transform = sprite->add(sprites, id);
			// Pie api is a bit clunky
			const fck_sprite_transform baseline = {
				.scale = 1.0f,
				.width = 64.0f,
				.height = 64.0f,
			};
			*transform = baseline;
			nk->pie->apply_position(view, &transform->x, &transform->y);
			nk->set_selection(view, transform);
		}

		{
			fck_sprite_transform *selected_transform = sprite->get(sprites, *selected_sprite_id);
			if (nk->pie->happened(&pie->remove) && selected_transform)
			{
				sprite->remove(sprites, *selected_sprite_id);
				nk->set_selection(view, NULL);
			}

			if (nk->pie->happened(&pie->duplicate) && selected_transform)
			{
				const fck_sprite_transform copy = *selected_transform;
				fck_sprite_transform *transform = sprite->add(sprites, selected_sprite_id->batch);
				*transform = copy;
				*selected_sprite_id = sprite->indexof(sprites, selected_sprite_id->batch, transform);

				nk->pie->apply_position(view, &transform->x, &transform->y);
				nk->set_selection(view, transform);
			}
			if (nk->pie->happened(&pie->duplicate_left) && selected_transform)
			{
				const fck_sprite_transform copy = *selected_transform;
				fck_sprite_transform *transform = sprite->add(sprites, selected_sprite_id->batch);
				*transform = copy;
				*selected_sprite_id = sprite->indexof(sprites, selected_sprite_id->batch, transform);

				transform->x = transform->x - transform->width;
				nk->set_selection(view, transform);
			}
			if (nk->pie->happened(&pie->duplicate_right) && selected_transform)
			{
				const fck_sprite_transform copy = *selected_transform;
				fck_sprite_transform *transform = sprite->add(sprites, selected_sprite_id->batch);
				*transform = copy;
				*selected_sprite_id = sprite->indexof(sprites, selected_sprite_id->batch, transform);

				transform->x = transform->x + transform->width;
				nk->set_selection(view, transform);
			}
			if (nk->pie->happened(&pie->duplicate_up) && selected_transform)
			{
				const fck_sprite_transform copy = *selected_transform;
				fck_sprite_transform *transform = sprite->add(sprites, selected_sprite_id->batch);
				*transform = copy;
				*selected_sprite_id = sprite->indexof(sprites, selected_sprite_id->batch, transform);

				transform->y = transform->y - transform->height;
				nk->set_selection(view, transform);
			}
			if (nk->pie->happened(&pie->duplicate_down) && selected_transform)
			{
				const fck_sprite_transform copy = *selected_transform;
				fck_sprite_transform *transform = sprite->add(sprites, selected_sprite_id->batch);
				*transform = copy;
				*selected_sprite_id = sprite->indexof(sprites, selected_sprite_id->batch, transform);

				transform->y = transform->y + transform->height;
				nk->set_selection(view, transform);
			}
		}
	}
}

typedef enum app_entity_bits
{
	app_entity_alive_bit_count = 1,

	app_entity_index_bit_count = 23,
	app_entity_index_invalid = (1 << app_entity_index_bit_count) - 1,

	app_entity_generation_bit_count = 8,
	app_entity_generation_max = (1 << app_entity_generation_bit_count) - 1,
} app_entity_bits;

typedef struct app_entity_handle
{
	fckc_u32 index : app_entity_index_bit_count;
	fckc_u32 alive : app_entity_alive_bit_count;
	fckc_u32 generation : app_entity_generation_bit_count;
} app_entity;

typedef struct app_entity_lookup
{
	kll_allocator *allocator;
	app_entity *sparse;

	fckc_u32 free_list;

	fckc_u32 count;
	fckc_u32 capacity;
} app_entity_lookup;

static app_entity_lookup app_entity_lookup_api_create(kll_allocator *allocator)
{
	app_entity_lookup entities = {0};
	entities.allocator = allocator;
	entities.free_list = app_entity_index_invalid;
	return entities;
}

static void app_entity_lookup_ensure_capacity(app_entity_lookup *entities, fckc_u32 extra)
{
	if (entities->capacity <= entities->count + extra)
	{
		// Make more space - eh, the extra makes stuff a bit ugly
		const fckc_u32 next_capacity = entities->capacity ? (entities->capacity + extra) * 2 : 8;
		const fckc_size_t total = next_capacity * sizeof(*entities->sparse);
		app_entity *next_values = (app_entity *)kll_malloc(entities->allocator, total);
		if (entities->sparse)
		{
			const fckc_size_t prev_total = entities->count * sizeof(*entities->sparse);
			memcpy(next_values, entities->sparse, prev_total);
			kll_free(entities->allocator, entities->sparse);
		}
		entities->sparse = next_values;
		entities->capacity = next_capacity;
	}
}

static app_entity app_entity_lookup_api_add(app_entity_lookup *entities, fckc_u32 value)
{
	app_entity_lookup_ensure_capacity(entities, 1);

	fckc_u32 index;
	if (entities->free_list != app_entity_index_invalid)
	{
		index = entities->free_list;
		app_entity *free_item = entities->sparse + index;
		free_item->generation = (free_item->generation + 1) % (app_entity_generation_max + 1);
		entities->free_list = free_item->index;
	}
	else
	{
		index = entities->count;
		app_entity *free_item = entities->sparse + index;
		free_item->generation = 0;
		entities->count = entities->count + 1;
	}

	// We have enough space!
	app_entity *result = entities->sparse + index;
	result->index = value;
	result->alive = 1;
	return *result;
}

static int app_entity_lookup_api_alive(app_entity_lookup *entities, app_entity entity)
{
	if (entity.index >= entities->count)
	{
		return 0;
	}

	app_entity *result = entities->sparse + entity.index;
	if (result->alive == 0)
	{
		return 0;
	}
	if (result->generation != entity.generation)
	{
		return 0;
	}
	return 1;
}

static int app_entity_lookup_api_set(app_entity_lookup *entities, app_entity entity, fckc_u32 value)
{
	if (!app_entity_lookup_api_alive(entities, entity))
	{
		return 0;
	}

	app_entity *result = entities->sparse + entity.index;
	result->index = value;
	return 1;
}

static int app_entity_lookup_api_remove(app_entity_lookup *entities, app_entity entity)
{
	if (!app_entity_lookup_api_alive(entities, entity))
	{
		return 0;
	}

	app_entity *result = entities->sparse + entity.index;
	// Append to the free list - Maintain generation!
	result->index = entities->free_list;
	result->alive = 0;
	entities->free_list = entity.index;
	return 1;
}

typedef struct app_entity_lookup_api
{
	app_entity_lookup (*create)(kll_allocator *allocator);
	app_entity (*add)(app_entity_lookup *entities, fckc_u32 value);
	int (*remove)(app_entity_lookup *entities, app_entity entity);
	int (*alive)(app_entity_lookup *entities, app_entity entity);
} app_entity_lookup_api;

static app_entity_lookup_api entity_lookup_api = {
	.create = app_entity_lookup_api_create,
	.add = app_entity_lookup_api_add,
	.remove = app_entity_lookup_api_remove,
	.alive = app_entity_lookup_api_alive,
};

typedef struct app_entity_api
{
	app_entity_lookup_api *lookup;
} app_entity_api;

static app_entity_api entity_api = {
	.lookup = &entity_lookup_api,
};

static app_entity_api *entity = &entity_api;

int main(int argc, char **argv)
{
	app_entity_lookup entities = entity->lookup->create(kll->system);
	entity->lookup->add(&entities, 0);
	entity->lookup->add(&entities, 0);
	entity->lookup->add(&entities, 0);
	entity->lookup->add(&entities, 0);
	entity->lookup->add(&entities, 0);
	entity->lookup->add(&entities, 0);
	entity->lookup->add(&entities, 0);
	entity->lookup->add(&entities, 0);
	entity->lookup->add(&entities, 0);

	for (fckc_u32 index = 0; index < 9; index++)
	{
		if (index % 2 == 0)
		{
			const app_entity handle = {.index = index};
			entity->lookup->remove(&entities, handle);
		}
	}

	entity->lookup->add(&entities, 0);
	entity->lookup->add(&entities, 0);
	entity->lookup->add(&entities, 0);
	entity->lookup->add(&entities, 0);
	entity->lookup->add(&entities, 0);
	entity->lookup->add(&entities, 0);
	entity->lookup->add(&entities, 0);
	entity->lookup->add(&entities, 0);
	entity->lookup->add(&entities, 0);

	// TODO: We need to setup stable editor entities, or something like that
	load_config(argc, argv);

	purge_files("temp-*.dll");

	fck_api_registry *registry = fck_api_registry_load("fck-api.dll");
	fck_plugins_api *plugins = fck_plugins_load(registry, "fck-plugins.dll");

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
	fck_shader_api *shader = (fck_shader_api *)registry->find(fck_shader_api_name);
	fck_png_api *png = (fck_png_api *)registry->find(fck_png_api_name);
	fck_nuklear_api *nk = (fck_nuklear_api *)registry->find(fck_nuklear_api_name);
	fck_gfx_api *gfx = (fck_gfx_api *)registry->find(fck_gfx_api_name);
	fck_sprite_api *sprite = (fck_sprite_api *)registry->find("fck-sprite");

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

	fck_window window = os->win->create("Vulkan Test Application", 1280, 720);
	int window_width, window_height;
	os->win->size(window, &window_width, &window_height);

	// We have to do this a bit smarter... Maybe not now
	// os->win->text_input_start(window);

	const sht_instance instance = render->load(sht_header_version);
	if (!render->is_ok(instance))
	{
		return 0;
	}

	sht_driver driver = instance.vt->start(instance, &window);
	if (!instance.vt->is_ok(driver))
	{
		return 0;
	}
	sht_memory *memory = driver.vt->memory(driver);
	const sht_swapchain swapchain = driver.vt->swapchain(driver);
	sht_command_buffer_vt *command = driver.vt->command_buffer;

	const fck_nk view = nk->create(kll->system, &window, &driver);
	nk->theme(view, fck_nk_theme_ruta);

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

	sht_elements indices = {0};

	sht_sampler sampler = {0};
	sht_image texture_image = {0};
	sht_image_view texture_view = {0};

	{
		sampler = driver.vt->create_sampler(driver, sht_filter_nearest);
		const sht_image_configuration config = {
			.format = sht_format_r8g8b8a8_unorm,
			.width = 4,
			.height = 1,
			.transfer = sht_transfer_target,
			.usage = sht_image_usage_sampled,
		};
		texture_image = memory->image->create(memory->bump, &config, sht_memory_gpu);
		texture_view = memory->image->view(memory->bump, texture_image, sht_format_r8g8b8a8_unorm);

		fckc_u32 pixels[] = {0xFF0000FF, 0xFF00FF00, 0xFFFF0000, 0xFFFFFFFF};
		driver.vt->upload_image(driver, &texture_image, pixels, sizeof(pixels));
	}

	const fck_png bird_png = png->load(fck_resource_path "bird-sheet.png");
	const fck_png items_png = png->load(fck_resource_path "items-sheet.png");

	const sht_image bird_image = app_load_image(driver, bird_png.data, sht_format_r8g8b8a8_unorm, bird_png.width, bird_png.height);
	const sht_image_view bird_image_view = memory->image->view(memory->bump, bird_image, sht_format_r8g8b8a8_unorm);

	const sht_image items_image = app_load_image(driver, items_png.data, sht_format_r8g8b8a8_unorm, items_png.width, items_png.height);
	const sht_image_view items_image_view = memory->image->view(memory->bump, items_image, sht_format_r8g8b8a8_unorm);

	const fck_gfx_shader vertex_shader = {.name = "vertex", .path = fck_resource_path "sprite.vert"};
	const fck_gfx_shader fragment_shader = {.name = "textured", .path = fck_resource_path "textured.frag"};
	const fck_gfx_create_info create_info = {.has_depth = 1, .vertex = &vertex_shader, .fragment = &fragment_shader};
	const fck_gfx sprite_gfx = gfx->create(kll->system, &driver, &create_info);

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

	{
		fckc_u32 index_data[] = {0, 1, 2, 1, 3, 2};
		indices.count = fck_arraysize(index_data);
		indices.buffer = memory->malloc(memory->bump, &sht_buffer_target(sht_buffer_usage_index, sizeof(index_data)), sht_memory_gpu);
		driver.vt->upload_buffer(driver, &indices.buffer, index_data, sizeof(index_data));
	}

	fck_sprites sprites = sprite->create(kll->system);
	const fck_sprite_batch_id birds_batch = sprite->batches->add(&sprites, "Birds", &bird_image_view, 32.0f, 32.0f);
	const fck_sprite_batch_id items_batch = sprite->batches->add(&sprites, "Items", &items_image_view, 16.0f, 16.0f);
	{
		fck_sprite_transform *transform = sprite->add(&sprites, birds_batch);
		const fck_sprite_transform baseline = {
			.scale = 1.0f,
			.width = 256.0f,
			.height = 256.0f,
			.x = -200.0f,
			.y = 0.0f,
		};
		*transform = baseline;
	}

	{
		fck_sprite_transform *transform = sprite->add(&sprites, items_batch);
		const fck_sprite_transform baseline = {
			.scale = 1.0f,
			.width = 64.0f,
			.height = 64.0f,
			.x = 200.0f,
			.y = 0.0f,
		};
		*transform = baseline;
	}

	fckc_u64 time_point = os->chrono->ms();

	fckc_u64 accumulator = 0;

	// fckc_u32 selected_batch_index = 0;
	// fckc_u32 selected_transform_index = 0;
	fck_sprite_id selected_sprite_id = {0, 0};
	int is_running = 1;
	while (is_running)
	{
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

		accumulator = accumulator + delta;
		if (accumulator >= 160)
		{
			accumulator = accumulator - 160;
			fck_sprite_transform *bird_transforms;
			const fckc_u32 bird_count = sprite->transforms(&sprites, birds_batch, &bird_transforms);
			for (fckc_size_t index = 0; index < bird_count; index++)
			{
				bird_transforms[index].horizontal_index = (bird_transforms[index].horizontal_index + 1) % 4;
			}
		}
		// TODO: Make render-vk hotreloadable :)
		// How hard can it be?
		plugins->hotreload();

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
			if (nk->begin(view))
			{
				fck_sprite_transform_editor(plugins, sprite, nk, view, &sprite_pie, &sprites, &selected_sprite_id);
			}
			nk->end(view);
		}

		memory->reset(memory->temp);

		fckc_u32 frame_index;
		const sht_image_view color_target = swapchain.vt->wait_and_acquire(swapchain, &frame_index);
		if (swapchain.vt->is_ok(swapchain, frame_index))
		{
			const sht_extent extent = swapchain.vt->extent(swapchain);
			const sht_command_buffer command_buffer = command->acquire(driver, frame_index);
			if (command->is_ok(command_buffer))
			{
				sht_viewport viewport;
				viewport.offset.x = 0.0f;
				viewport.offset.y = 0.0f;
				viewport.depth.min = (float)0.0f;
				viewport.depth.max = (float)1.0f;

				sht_scissor scissor;
				scissor.offset.x = 0;
				scissor.offset.y = 0;
				scissor.extent = viewport.extent = swapchain.vt->extent(swapchain);

				{
					sht_render_desc desc = {
						.colour = {.view = color_target,
					               .load_op = sht_clear,
					               .store_op = sht_store,
					               .clear_value = {0.2f, 0.0f, 0.2f, 1.0f}},
						.depth = {.view = depth_view, .load_op = sht_clear, .store_op = sht_dont_care},
					};

					const sht_render_pass render_pass = command->render_pass->begin(command_buffer, &desc);
					if (command->render_pass->is_ok(render_pass))
					{
						command->viewport(command_buffer, &viewport);
						command->scissor(command_buffer, &scissor);

						const fckc_size_t batch_count = sprite->batches->count(&sprites);
						for (fckc_size_t batch_index = 0; batch_index < batch_count; batch_index++)
						{
							const fck_sprite_batch_id id = sprite->batches->index(&sprites, batch_index);
							fck_sprite_transform *transforms = NULL;
							const fckc_u32 count = sprite->transforms(&sprites, id, &transforms);

							if (count > 0)
							{
								float sprite_width = 0;
								float sprite_height = 0;
								sprite->batches->dimensions(&sprites, id, &sprite_width, &sprite_height);
								command->index_buffer(command_buffer, &indices.buffer, 0);

								sht_bss *bss = gfx->bss(sprite_gfx);
								sht_graphics_pipeline *pipeline = gfx->pipeline(sprite_gfx); //

								const app_screen screen = {
									.width = (float)extent.width,
									.height = (float)extent.height,
									.sprite_width = sprite_width,
									.sprite_height = sprite_height,
								};

								const sht_buffer_upload_desc screen_upload = {.data = &screen, .size = sizeof(screen), .count = 1};

								const sht_buffer_upload_desc transform_upload = {
									.data = transforms,
									.size = sizeof(*transforms),
									.count = count,
								};

								const sht_image_view *view = sprite->batches->image_view(&sprites, id);
								const sht_image_upload_desc image_upload = {.samplers = sampler, .views = *view};

								driver.vt->bss->upload_buffer(*bss, 0, &screen_upload);
								driver.vt->bss->upload_buffer(*bss, 1, &transform_upload);
								driver.vt->bss->upload_image(*bss, 3, &image_upload);
								command->bss(command_buffer, *bss);

								command->graphics_pipeline(command_buffer, *pipeline);

								const sht_draw_indexed_desc desc = {
									.first_index = 0,
									.index_count = to_u32(indices.count),
									.instance_count = count,
									.first_instance = 0,
									.vertex_offset = 0,
								};

								command->draw_indexed(command_buffer, &desc);
							}
						}

						command->render_pass->end(command_buffer);
					}
				}

				{
					sht_render_desc desc = {.colour = {.view = color_target, .load_op = sht_load, .store_op = sht_store}};
					const sht_render_pass render_pass = command->render_pass->begin(command_buffer, &desc);
					if (command->render_pass->is_ok(render_pass))
					{
						command->viewport(command_buffer, &viewport);
						command->scissor(command_buffer, &scissor);
						nk->present(view, &command_buffer, frame_index);

						command->render_pass->end(command_buffer);
					}
				}

				command->submit(command_buffer, sht_queue_graphic);
			}
		}
	}

	plugins->shutdown();
	purge_files("temp-*.dll");

	os->win->destroy(window);

	return 0;
}
