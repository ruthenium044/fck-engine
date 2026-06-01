
#include <fck_apis.h>
#include <fck_hash.h>
#include <fck_input.h>
#include <fck_os.h>
#include <fck_pkey.h>
#include <fck_plugins.h>
#include <fck_shader.h>
#include <fckc_assert.h>
#include <fckc_inttypes.h>
#include <sht_render.h>

#include <stdio.h>

#include <stdlib.h>
#include <string.h>

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

// typedef struct fck_solid_vertex
//{
//	fckc_f32 position[3];
//	fckc_f32 color[3];
//	fckc_f32 uv[2];
// } fck_solid_vertex;

// const sht_vertex_binding vertex_bindings[] = {
//	{.format = SHT_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(fck_solid_vertex, position), .location = 0},
//	{.format = SHT_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(fck_solid_vertex, color), .location = 1},
//	{.format = SHT_FORMAT_R32G32_SFLOAT, .offset = offsetof(fck_solid_vertex, uv), .location = 2}};

typedef struct app_screen
{
	float width;
	float height;
} app_screen;

typedef struct app_quad_transform
{
	float x;
	float y;
	float z;
	float rotation;
	float width;
	float height;
} app_quad_transform;

typedef struct app_quads
{
	app_quad_transform transforms[64];
	fckc_u32 count;
} app_quads;

static void app_quads_add(app_quads *quads, float x, float y)
{
	const app_quad_transform init_transform = {
		.x = 0.0f,
		.y = 0.0f,
		.z = 0.0f,
		.rotation = 0.0f,
		.width = 100.0f,
		.height = 100.0f,
	};

	app_quad_transform *transform = quads->transforms + quads->count;
	*transform = init_transform;
	transform->x = x;
	transform->y = y;
	quads->count = quads->count + 1;
}

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

	fck_window window = os->win->create("Test", 1920, 1080);

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
	sht_swapchain swapchain = driver.vt->swapchain(driver);
	sht_command_buffer_vt *command = driver.vt->command_buffer;
	sht_bss_vt *bss = driver.vt->bss;

	fck_glsl_object vert = {0};
	fck_glsl_object frag = {0};
	sht_graphics_pipeline pipeline = {0};
	sht_bss gpu_data = {0};
	sht_elements indices = {0};

	app_quads quads = {0};
	app_quads_add(&quads, 0.0f, 0.0f);
	app_quads_add(&quads, 50.0f, 50.0f);
	app_quads_add(&quads, 200.0f, 200.0f);
	app_quads_add(&quads, 400.0f, 400.0f);

	{
		// Ok, All I need is a solid texture pipeline... We only do 2D
		fck_shader_compiler compiler = shader->create();
		if (!shader->is_ok(compiler))
		{
			return 0;
		}

		fck_file vert_file = os->fs->open(fck_resource_path "solid.vert", "r");
		fck_shader_desc vert_desc = (fck_shader_desc){FCK_SHADER_VERTEX, "solid-vert", "main"};

		fck_file frag_file = os->fs->open(fck_resource_path "solid.frag", "r");
		fck_shader_desc frag_desc = (fck_shader_desc){FCK_SHADER_FRAGMENT, "solid-frag", "main"};

		vert = compiler.create_glsl_from_file(&compiler, &vert_desc, &vert_file);
		frag = compiler.create_glsl_from_file(&compiler, &frag_desc, &frag_file);

		sht_binding bindings[] = {
			{.id = 0, .type = SHT_BINDING_UNIFORM, .stages = SHT_STAGE_VERTEX_SHADER},
			{.id = 1, .type = SHT_BINDING_STORAGE, .stages = SHT_STAGE_VERTEX_SHADER},
		};

		sht_binding_desc binding_desc = {.bindings = bindings, .count = fck_arraysize(bindings)};
		gpu_data = driver.vt->bss->create(driver, &binding_desc);

		sht_vertex_desc vertex_desc = {
			.stride = 0,
			.bindings = NULL,
			.count = 0,
		};
		const sht_raster_desc raster_desc = {
			.cull_mode = SHT_CULL_MODE_NONE,
			.topology = SHT_TRIANGLE_LIST,
			.color = SHT_FORMAT_B8G8R8A8_UNORM,
			.depth = SHT_FORMAT_UNDEFINED,
		};
		sht_graphic_desc graphic_desc = {
			.fragment = &frag.generic,
			.vertex = &vert.generic,
			.vertex_desc = &vertex_desc,
			.raster = raster_desc,
		};

		pipeline = driver.vt->graphics_pipeline->create(driver, gpu_data, &graphic_desc);
		compiler.destroy(&compiler, &vert.generic);
		compiler.destroy(&compiler, &frag.generic);
		compiler.shutdown(&compiler);

		fckc_u32 index_data[] = {0, 1, 2, 1, 3, 2};
		indices.count = fck_arraysize(index_data);
		indices.buffer = memory->malloc(memory->bump, &sht_buffer_target(SHT_BUFFER_USAGE_INDEX, sizeof(index_data)), SHT_MEMORY_GPU);
		driver.vt->upload_buffer(driver, &indices.buffer, index_data, sizeof(index_data));
	}

	int is_running = 1;
	while (is_running)
	{
		plugins->hotreload();

		fck_input_event events[32] = {0};
		const fckc_size_t result = input->events(events, fck_arraysize(events));
		for (fckc_size_t index = 0; index < result; index++)
		{
			fck_input_event *e = events + index;
			if (e->source->type == fck_input_source_keyboard)
			{
				if (e->description->id == fck_pkey_escape)
				{
					is_running = 0;
				}
			}
		}

		memory->reset(memory->temp);

		fckc_u32 frame_index;
		const sht_image_view color_target = swapchain.vt->wait_and_acquire(swapchain, &frame_index);
		if (swapchain.vt->is_ok(swapchain, frame_index))
		{
			sht_extent extent = swapchain.vt->extent(swapchain);

			const sht_command_buffer command_buffer = command->acquire(driver, frame_index);
			if (command->is_ok(command_buffer))
			{
				sht_render_desc desc = {
					.colour =
						{
							.view = color_target,
							.load_op = SHT_CLEAR,
							.store_op = SHT_STORE,
							.clear_value = {0.0f, 0.0f, 0.2f, 1.0f},
						},
				};

				app_screen screen = {
					.width = (float)extent.width,
					.height = (float)extent.height,
				};

				driver.vt->bss->upload(gpu_data, 0, sht_upload_params{.data = &screen, .size = sizeof(screen)});
				driver.vt->bss->upload(gpu_data, 1, sht_upload_params{.data = &quads, .size = sizeof(*quads.transforms) * quads.count});

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

					command->bss(command_buffer, gpu_data);

					command->graphics_pipeline(command_buffer, pipeline);
					command->index_buffer(command_buffer, &indices.buffer, 0);
					sht_draw_indexed_desc indexed = {
						.first_index = 0,
						.first_instance = 0,
						.index_count = to_u32(indices.count),
						.instance_count = quads.count,
						.vertex_offset = 0,
					};
					command->draw_indexed(command_buffer, &indexed);

					command->render_pass->end(command_buffer);
				}
				command->submit(command_buffer, SHT_QUEUE_GRAPHIC);
			}
		}
	}

	plugins->shutdown();
	purge_files("temp-*.dll");

	os->win->destroy(window);

	return 0;
}
