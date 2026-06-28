
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

#include "reflection.h"

#include "fck_gfx.h"
#include "fck_nuklear.h"

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
} app_screen;

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
		.resize_line_width = 8.0f,
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

	const fck_nk view = nk->create(kll->system, &window, &driver, shader);
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

	nk->hamburger->push(view, &help_menu_item);
	nk->hamburger->push(view, &about_menu_item);
	nk->hamburger->push(view, &setting_menu_item);

	fck_nk_pie pie = {0};
	nk->pie->push(&pie, &copy_pie_item);
	nk->pie->push(&pie, &paste_pie_item);
	nk->pie->push(&pie, &duplicate_pie_item);
	nk->pie->push(&pie, &delete_pie_item);
	nk->pie->push(&pie, &properties_pie_item);

	nk->pie->add_child(&properties_pie_item, &properties_child0_pie_item);
	nk->pie->add_child(&properties_pie_item, &properties_child1_pie_item);

	nk->pie->add_child(&properties_child0_pie_item, &properties_child_child_pie_item);
	nk->pie->add_child(&properties_child_child_pie_item, &properties_child_child_child_pie_item);

	sht_graphics_pipeline graphic_pipelines;

	sht_elements indices = {0};

	sht_sampler sampler = {0};
	sht_image texture_image = {0};
	sht_image_view texture_view = {0};

	{
		sampler = driver.vt->create_sampler(driver, sht_filter_linear);
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

	// sht_image depth_image = {0};
	// sht_image_view depth_view = {0};
	//{
	//	sht_extent extent = swapchain.vt->extent(swapchain);
	//	sht_image_configuration config = (sht_image_configuration){
	//		.format = SHT_FORMAT_D16_UNORM,
	//		.width = extent.width,
	//		.height = extent.height,
	//		.transfer = SHT_TRANSFER_RETAINED,
	//		.usage = SHT_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT,
	//	};

	//	depth_image = memory->image->create(memory->bump, &config,
	// SHT_MEMORY_GPU); 	depth_view = memory->image->view(memory->bump,
	// depth_image, SHT_FORMAT_UNDEFINED);
	//}

	{
		fckc_u32 index_data[] = {0, 1, 2, 1, 3, 2};
		indices.count = fck_arraysize(index_data);
		indices.buffer = memory->malloc(memory->bump, &sht_buffer_target(sht_buffer_usage_index, sizeof(index_data)), sht_memory_gpu);
		driver.vt->upload_buffer(driver, &indices.buffer, index_data, sizeof(index_data));
	}

	int is_running = 1;
	while (is_running)
	{
		// TODO: Make render-vk hotreloadable :)
		// How hard can it be?
		plugins->hotreload();

		nk->input->begin(view);

		fck_input_event events[32] = {0};
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
				nk->pie->execute(view, &pie, 125.0f);
			}
			nk->end(view);

			const fck_nk_control control = nk->control(view);
			if (control.close)
			{
				is_running = 0;
			}
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
				sht_render_desc desc = {
					.colour = {.view = color_target, .load_op = sht_clear, .store_op = sht_store, .clear_value = {0.0f, 0.0f, 0.2f, 1.0f}},
					//.depth = {.view = depth_view, .load_op = SHT_CLEAR, .store_op =
				    // SHT_DONT_CARE, .clear_value = 0.0f},
				};

				const app_screen screen = {
					.width = (float)extent.width,
					.height = (float)extent.height,
				};

				const sht_render_pass render_pass = command->render_pass->begin(command_buffer, &desc);

				sht_viewport viewport;
				viewport.offset.x = 0.0f;
				viewport.offset.y = 0.0f;
				viewport.depth.min = (float)0.0f;
				viewport.depth.max = (float)1.0f;

				sht_scissor scissor;
				scissor.offset.x = 0;
				scissor.offset.y = 0;
				scissor.extent = viewport.extent = swapchain.vt->extent(swapchain);

				if (command->render_pass->is_ok(render_pass))
				{
					command->viewport(command_buffer, &viewport);
					command->scissor(command_buffer, &scissor);

					// command->index_buffer(command_buffer, &indices.buffer, 0);

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
