
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

#pragma optimize("", off)

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
	float texture_chunk_size;
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

// From GLSL
typedef struct app_sprite_transform
{
	float x;
	float y;
	float z;
	float rotation;
	float width;
	float height;
	float scale;
	int horizontal_index;
	int vertical_index;
} app_sprite_transform;

int main(int argc, char **argv)
{
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

	const fck_window_configuration config = {
		.title_bar_height = 35.0f,
		.resize_line_width = 4.0f,
		.menu_area_width = 35.0f * 2.0f,
		.button_area_width = 35.0f * 2.0f,
	};
	os->win->configuration(window, &config);

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

	fck_nk_pie_item copy_pie_item = {
		.name = "Copy",
	};
	fck_nk_pie_item paste_pie_item = {
		.name = "Paste",
	};
	fck_nk_pie_item duplicate_pie_item = {
		.name = "Duplicate",
	};
	fck_nk_pie_item delete_pie_item = {
		.name = "Delete",
	};
	fck_nk_pie_item properties_pie_item = {
		.name = "Properties",
	};

	fck_nk_pie_item properties_child0_pie_item = {
		.name = "Extra",
	};

	fck_nk_pie_item properties_child_child_pie_item = {
		.name = "X",
	};

	fck_nk_pie_item properties_child_child_child_pie_item = {
		.name = "X",
	};

	fck_nk_pie_item properties_child1_pie_item = {
		.name = "Extra",
	};

	fck_nk_pie_item properties_add_item = {
		.name = "Add",
	};

	fck_nk_pie_item properties_add_bird_item = {
		.name = "Bird",
	};

	nk->hamburger->push(view, &help_menu_item);
	nk->hamburger->push(view, &about_menu_item);
	nk->hamburger->push(view, &setting_menu_item);

	fck_nk_pie pie = {0};
	nk->pie->push(&pie, &copy_pie_item);
	nk->pie->push(&pie, &paste_pie_item);
	nk->pie->push(&pie, &duplicate_pie_item);
	nk->pie->push(&pie, &delete_pie_item);
	nk->pie->push(&pie, &properties_pie_item);

	nk->pie->push(&pie, &properties_add_item);
	nk->pie->add_child(&properties_add_item, &properties_add_bird_item);

	nk->pie->add_child(&properties_pie_item, &properties_child0_pie_item);
	nk->pie->add_child(&properties_pie_item, &properties_child1_pie_item);

	nk->pie->add_child(&properties_child0_pie_item, &properties_child_child_pie_item);
	nk->pie->add_child(&properties_child_child_pie_item, &properties_child_child_child_pie_item);

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

	fck_png bird_png = png->load(fck_resource_path "bird-sheet.png");
	fck_png items_png = png->load(fck_resource_path "items-sheet.png");
	const sht_image bird_image = app_load_image(driver, bird_png.data, sht_format_r8g8b8a8_unorm, bird_png.width, bird_png.height);
	const sht_image_view bird_image_view = memory->image->view(memory->bump, bird_image, sht_format_r8g8b8a8_unorm);

	const fck_gfx_shader vertex_shader = {.name = "vertex", .path = fck_resource_path "sprite.vert"};
	const fck_gfx_shader fragment_shader = {.name = "textured", .path = fck_resource_path "textured.frag"};
	const fck_gfx_create_info create_info = {.vertex = &vertex_shader, .fragment = &fragment_shader};
	const fck_gfx bird_gfx = gfx->create(kll->system, &driver, &create_info);

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

	fckc_u32 bird_transforms_count = 0;
	app_sprite_transform bird_transforms[64] = {
		{.x = -100.0f, .y = -100.0f, .scale = 1.0f, .width = 256.0f, .height = 256.0f, .vertical_index = 0},
		{.x = 200.0f, .y = 200.0f, .scale = 1.0f, .width = 256.0f, .height = 256.0f, .vertical_index = 2},
		{.x = -100, .y = 200.0f, .scale = 1.0f, .width = 256.0f, .height = 256.0f, .vertical_index = 9},
		{.x = 200, .y = -100.0f, .scale = 1.0f, .width = 256.0f, .height = 256.0f, .vertical_index = 11}};

	fckc_u64 time_point = os->chrono->ms();

	fckc_u64 accumulator = 0;

	app_sprite_transform* selected_bird = NULL;

	int is_running = 1;
	while (is_running)
	{
		const fckc_u64 now = os->chrono->ms();
		const fckc_u64 delta = now - time_point;
		time_point = now;

		/*accumulator = accumulator + delta;
		if (accumulator >= 160)
		{
		    accumulator = accumulator - 160;
		    for (fckc_size_t index = 0; index < fck_arraysize(bird_transforms); index++)
		    {
		        bird_transforms[index].horizontal_index = (bird_transforms[index].horizontal_index + 1) % 4;
		    }
		}*/
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
				nk->panel->begin(view, "Inspector", 300.0f);
				{
					if (nk->panel->push(view, "Birds %d", bird_transforms_count))
					{
						for (fckc_size_t index = 0; index < bird_transforms_count; index++)
						{
							if (nk->panel->push(view, "Bird %d", index))
							{
								app_sprite_transform *bird = bird_transforms + index;
								bird->x = nk->elements->f32(view, "x", -1280.0f, bird->x, 1280.0f, 1.0f);
								bird->y = nk->elements->f32(view, "y", -720.0f, bird->y, 720.0f, 1.0f);
								bird->z = nk->elements->f32(view, "z", 0.0f, bird->z, 1.0f, 0.1f);
								bird->width = nk->elements->f32(view, "width", 0.0f, bird->width, 256.0f, 4.0f);
								bird->height = nk->elements->f32(view, "height", 0.0f, bird->height, 256.0f, 4.0f);
								bird->rotation = nk->elements->f32(view, "rotation", 0.0f, bird->rotation, 360.0f, 1.0f);
								bird->scale = nk->elements->f32(view, "scale", 1.0f, bird->scale, 100.0f, 1.0f);
								bird->horizontal_index = nk->elements->i32(view, "horizontal index", 0, bird->horizontal_index, 10, 1);
								bird->vertical_index = nk->elements->i32(view, "vertical index", 0, bird->vertical_index, 10, 1);
								nk->panel->pop(view);
							}
						}
						nk->panel->pop(view);
					}

					if(selected_bird) {
						if (nk->panel->push(view, "Selected Bird"))
						{
							app_sprite_transform* bird = selected_bird;
							bird->x = nk->elements->f32(view, "x", -1280.0f, bird->x, 1280.0f, 1.0f);
							bird->y = nk->elements->f32(view, "y", -720.0f, bird->y, 720.0f, 1.0f);
							bird->z = nk->elements->f32(view, "z", 0.0f, bird->z, 1.0f, 0.1f);
							bird->width = nk->elements->f32(view, "width", 0.0f, bird->width, 256.0f, 4.0f);
							bird->height = nk->elements->f32(view, "height", 0.0f, bird->height, 256.0f, 4.0f);
							bird->rotation = nk->elements->f32(view, "rotation", 0.0f, bird->rotation, 360.0f, 1.0f);
							bird->scale = nk->elements->f32(view, "scale", 1.0f, bird->scale, 100.0f, 1.0f);
							bird->horizontal_index = nk->elements->i32(view, "horizontal index", 0, bird->horizontal_index, 10, 1);
							bird->vertical_index = nk->elements->i32(view, "vertical index", 0, bird->vertical_index, 10, 1);
							nk->panel->pop(view);
						}
					}
				}
				nk->panel->end(view);

				{
					const fck_nk_colour on = {0, 255, 0, 255};
					const fck_nk_colour off = {255, 0, 0, 255};
					fckc_size_t index;
					for (index = 0; index < bird_transforms_count; index++)
					{
						app_sprite_transform *bird = bird_transforms + index;
						if (nk->select(view, bird, bird->x, bird->y, bird->width, bird->height, on))
						{
							selected_bird = bird;
						}
						if (nk->control_point(view, bird, &bird->x, &bird->y, 16.0f, 2.0f, on, off))
						{
							// break;
						}
					}
				}
				// TODO: Pie api is clunky, we should create pies through nk and then pie can reference upward!
				nk->pie->execute(view, &pie, 125.0f);
			}
			nk->end(view);

			const fck_nk_control control = nk->control(view);
			if (control.close)
			{
				is_running = 0;
			}
		}

		if (nk->pie->happened(&properties_add_bird_item))
		{
			os->io->log("Create Bird");
			app_sprite_transform *transform = bird_transforms + bird_transforms_count;
			// Pie api is a bit clunky
			const app_sprite_transform baseline = {
				.x = pie.x,
				.y = pie.y,
				.scale = 1.0f,
				.width = 256.0f,
				.height = 256.0f,
			};
			*transform = baseline;
			nk->to_world(view, &transform->x, &transform->y);
			bird_transforms_count = bird_transforms_count + 1;
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
				const app_screen screen = {
					.width = (float)extent.width,
					.height = (float)extent.height,
					.texture_chunk_size = 32.0f,
				};

				sht_viewport viewport;
				viewport.offset.x = 0.0f;
				viewport.offset.y = 0.0f;
				viewport.depth.min = (float)0.0f;
				viewport.depth.max = (float)1.0f;

				sht_scissor scissor;
				scissor.offset.x = 0;
				scissor.offset.y = 0;
				scissor.extent = viewport.extent = swapchain.vt->extent(swapchain);

				sht_render_desc desc = {
					.colour = {.view = color_target, .load_op = sht_clear, .store_op = sht_store, .clear_value = {0.0f, 0.0f, 0.2f, 1.0f}},
					//.depth = {.view = depth_view, .load_op = sht_clear, .store_op = sht_dont_care, .clear_value = 1.0f},
				};

				const sht_render_pass render_pass = command->render_pass->begin(command_buffer, &desc);
				if (command->render_pass->is_ok(render_pass))
				{
					command->viewport(command_buffer, &viewport);
					command->scissor(command_buffer, &scissor);

					if (bird_transforms_count > 0)
					{
						command->index_buffer(command_buffer, &indices.buffer, 0);

						sht_bss *bss = gfx->bss(bird_gfx);
						sht_graphics_pipeline *pipeline = gfx->pipeline(bird_gfx);

						const sht_buffer_upload_desc screen_upload = {.data = &screen, .size = sizeof(screen), .count = 1};
						const sht_buffer_upload_desc transform_upload = {
							.data = &bird_transforms,
							.size = sizeof(*bird_transforms),
							.count = bird_transforms_count,
						};
						const sht_image_upload_desc image_upload = {.samplers = sampler, .views = bird_image_view};

						driver.vt->bss->upload_buffer(*bss, 0, &screen_upload);
						driver.vt->bss->upload_buffer(*bss, 1, &transform_upload);
						driver.vt->bss->upload_image(*bss, 3, &image_upload);
						command->bss(command_buffer, *bss);

						command->graphics_pipeline(command_buffer, *pipeline);

						const sht_draw_indexed_desc desc = {
							.first_index = 0,
							.index_count = to_u32(indices.count),
							.instance_count = bird_transforms_count,
							.first_instance = 0,
							.vertex_offset = 0,
						};

						command->draw_indexed(command_buffer, &desc);
					}

					nk->present(view, &command_buffer, frame_index);

					command->render_pass->end(command_buffer);
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
