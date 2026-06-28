// #include "SDL3/SDL_vulkan.h"

#include <fck_os.h>
#include <fck_pkey.h>

#include <fck_input.h>
#include <kll.h>
#include <kll_malloc.h>

#include <fckc_assert.h>
#include <fckc_inttypes.h>

#include <memory.h>

#include <fck_apis.h>
#include <fck_shader.h>
#include <sht_render.h>

typedef enum fck_test_app_result
{
	FCK_TEST_APP_RESULT_CONTINUE,
	FCK_TEST_APP_RESULT_DONE,
} fck_test_app_result;

typedef struct sht_mvp
{
	fckc_f32 model[4][4];
	fckc_f32 view[4][4];
	fckc_f32 projection[4][4];

	fckc_f32 color[4];
} sht_mvp;

typedef struct sht_standard_vertex
{
	fckc_f32 position[3];
	fckc_f32 color[3];
	fckc_f32 uv[2];
} sht_standard_vertex;

const sht_vertex_binding vertex_bindings[] = {
	{.format = sht_format_r32g32b32_sfloat, .offset = offsetof(sht_standard_vertex, position), .location = 0},
	{.format = sht_format_r32g32b32_sfloat, .offset = offsetof(sht_standard_vertex, color), .location = 1},
	{.format = sht_format_r32_G32_sfloat, .offset = offsetof(sht_standard_vertex, uv), .location = 2}};

typedef struct fck_test_app_application
{
	fck_window window;

	sht_sampler sampler;
	sht_image texture_image;
	sht_image_view texture_view;
	fck_input *input;

	sht_mvp mvp;
	sht_bss bss;
	sht_elements vertices;
	sht_elements indices;

	sht_graphics_pipeline pipeline;
	sht_image depth_image;
	sht_image_view depth_view;

	sht_instance instance;
	sht_driver driver;
} fck_test_app_application;

typedef fck_input *(fck_input_load_prototype)(void);
#define to_fck_input_load(v) (fck_input_load_prototype *)(v)
#define fck_input_load_name "fck_input_load"

static fck_api_registry *fck_api_registry_load(const char *path)
{
	// This badboy needs to get released
	const fck_shared_object so = os->so->load(path);
	fck_load_func *loader = (fck_load_func *)os->so->symbol(so, "fck_api_load");
	fck_api_registry *registry = (fck_api_registry *)loader(NULL, NULL);
	return registry;
}

fck_test_app_result fck_test_app_app_init(void **app_state, int argc, char **argv)
{
	fck_api_registry *registry = fck_api_registry_load("fck-api.dll");

	fck_test_app_application *app = (fck_test_app_application *)kll_malloc(kll->system, sizeof(*app));
	memset(app, 0, sizeof(*app));
	*app_state = app;

	app->window = os->win->create("fck-vk", 1400, 600);

	fck_shared_object api_so = os->so->load("fck-render-vk");
	fck_shared_object shader_so = os->so->load("fck-shader");
	fck_shader_api *shader_api =
		(fck_shader_api *)((void *(*)(void *, void *))os->so->symbol(shader_so, "fck_shader_load"))(registry, NULL);
	sht_render_api *loader = (sht_render_api *)((void *(*)(void *, void *))os->so->symbol(api_so, "fck_render_vk_load"))(registry, NULL);

	app->instance = loader->load(sht_header_version);
	fck_assert(loader->is_ok(app->instance));
	app->driver = app->instance.vt->start(app->instance, &app->window);
	fck_assert(app->instance.vt->is_ok(app->driver));

	float mvp_model[4][4] = {
		{1.0f, 0.0f, 0.0f, 0.0f},
		{0.0f, 1.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 1.0f, 0.0f},
		{0.0f, 0.0f, 0.0f, 1.0f},
	};
	float mvp_view[4][4] = {
		{1.0f, 0.0f, 0.0f, 0.0f},
		{0.0f, 1.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 1.0f, 0.0f},
		{0.0f, 0.0f, 0.0f, 1.0f},
	};
	float mvp_projection[4][4] = {
		{0.666667f, 0.0f, 0.0f, 0.0f},
		{0.0f, 0.666667f, 0.0f, 0.0f},
		{0.0f, 0.0f, -1.0f, 0.0f},
		{0.0f, 0.0f, 0.0f, 1.0f},
	};
	app->mvp.color[0] = 0.0f;
	app->mvp.color[1] = 1.0f;
	app->mvp.color[2] = 1.0f;
	app->mvp.color[3] = 1.0f;

	// Then set:
	memcpy(app->mvp.projection, mvp_projection, sizeof(mvp_projection));
	memcpy(app->mvp.view, mvp_view, sizeof(mvp_view));
	memcpy(app->mvp.model, mvp_model, sizeof(mvp_model));

	sht_driver driver = app->driver;
	sht_memory *mem = driver.vt->memory(driver);
	sht_swapchain swapchain = driver.vt->swapchain(driver);

	sht_standard_vertex vertex_data[] = {
		{.position = {0.0f, 1.0f, 0.0f}, .color = {1.0f, 1.0f, 1.0f}, .uv = {0.0f, 1.0f}},
		{.position = {1.0f, 1.0f, 0.0f}, .color = {1.0f, 1.0f, 1.0f}, .uv = {1.0f, 1.0f}},
		{.position = {0.0f, 0.0f, 0.0f}, .color = {1.0f, 1.0f, 1.0f}, .uv = {0.0f, 0.0f}},
		{.position = {1.0f, 0.0f, 0.0f}, .color = {1.0f, 1.0f, 1.0f}, .uv = {1.0f, 0.0f}},
	};

	app->vertices.count = fck_arraysize(vertex_data);
	app->vertices.buffer = mem->malloc(mem->bump, &sht_buffer_target(sht_buffer_usage_vertex, sizeof(vertex_data)), sht_memory_gpu);

	fckc_u32 index_data[] = {0, 1, 2, 1, 3, 2};
	app->indices.count = fck_arraysize(index_data);
	app->indices.buffer = mem->malloc(mem->bump, &sht_buffer_target(sht_buffer_usage_index, sizeof(index_data)), sht_memory_gpu);

	{
		driver.vt->upload_buffer(driver, &app->vertices.buffer, vertex_data, sizeof(vertex_data));
		driver.vt->upload_buffer(driver, &app->indices.buffer, index_data, sizeof(index_data));
	}

	app->sampler = driver.vt->create_sampler(driver, sht_filter_linear);
	app->texture_image = mem->image->create(mem->bump,
	                                        &(sht_image_configuration){
												.format = sht_format_r8g8b8a8_unorm,
												.width = 4,
												.height = 1,
												.transfer = sht_transfer_target,
												.usage = sht_image_usage_sampled,
											},
	                                        sht_memory_gpu);
	app->texture_view = mem->image->view(mem->bump, app->texture_image, sht_format_r8g8b8a8_unorm);

	fckc_u32 pixels[] = {0xFF0000FF, 0xFFFF0000, 0xFF00FF00, 0xFFFFFFFF};
	driver.vt->upload_image(driver, &app->texture_image, pixels, sizeof(pixels));

	sht_extent extent = swapchain.vt->extent(swapchain);
	sht_image_configuration config = (sht_image_configuration){
		.format = sht_format_d16_unorm,
		.width = (fckc_u32)extent.width,
		.height = (fckc_u32)extent.height,
		.transfer = sht_transfer_retained,
		.usage = sht_image_usage_depth_stencil_attachment,
	};

	app->depth_image = mem->image->create(mem->bump, &config, sht_memory_gpu);
	app->depth_view = mem->image->view(mem->bump, app->depth_image, sht_format_undefined);

	sht_binding bindings[] = {
		{.id = 0, .type = sht_binding_uniform, .stages = sht_stage_vertex_shader},
		{.id = 1, .type = sht_binding_readonly_image, .stages = sht_stage_fragment_shader},
	};

	app->bss = driver.vt->bss->create(driver, &(sht_binding_desc){.bindings = bindings, .count = fck_arraysize(bindings)});

	{
		fck_shader_compiler compiler = shader_api->create();

		fck_file vert_file = os->fs->open("hlsl/triangle.vert", "r");
		fck_shader_desc vert_desc = (fck_shader_desc){FCK_SHADER_VERTEX, "triangle-vert", "main"};

		fck_file frag_file = os->fs->open("hlsl/triangle.frag", "r");
		fck_shader_desc frag_desc = (fck_shader_desc){FCK_SHADER_FRAGMENT, "triangle-frag", "main"};

		fck_hlsl_object vert = compiler.create_hlsl_from_file(&compiler, &vert_desc, &vert_file);
		fck_hlsl_object frag = compiler.create_hlsl_from_file(&compiler, &frag_desc, &frag_file);

		sht_graphic_desc desc = (sht_graphic_desc){.fragment = &frag.generic,
		                                           .vertex = &vert.generic,
		                                           .vertex_desc = &(sht_vertex_desc){.stride = sizeof(sht_standard_vertex),
		                                                                             .bindings = vertex_bindings,
		                                                                             .count = fck_arraysize(vertex_bindings)},
		                                           .raster = (sht_raster_desc){
													   .cull_mode = sht_cull_mode_none,
													   .topology = sht_triangle_list,
													   .color = sht_format_b8g8r8a8_unorm,
													   .depth = sht_format_d16_unorm,
												   }};
		app->pipeline = driver.vt->graphics_pipeline->create(driver, app->bss, &desc);
		compiler.destroy(&compiler, &vert.generic);
		compiler.destroy(&compiler, &frag.generic);
		compiler.shutdown(&compiler);
	}

	return FCK_TEST_APP_RESULT_CONTINUE;
}

fck_test_app_result fck_test_app_app_draw(fck_test_app_application *app)
{
	sht_driver driver = app->driver;

	sht_memory *mem = driver.vt->memory(driver);
	sht_swapchain swapchain = driver.vt->swapchain(driver);
	sht_command_buffer_vt *command = driver.vt->command_buffer;

	mem->reset(mem->temp);

	fckc_u32 frame_index;
	sht_image_view color_target = swapchain.vt->wait_and_acquire(swapchain, &frame_index);
	if (!swapchain.vt->is_ok(swapchain, frame_index))
	{
		if (frame_index == sht_swapchain_needs_resize)
		{
			// Take a leap!
			sht_extent extent = swapchain.vt->extent(swapchain);
			mem->image->recreate(mem->bump, &app->depth_image, extent, &app->depth_view, 1);
			return FCK_TEST_APP_RESULT_CONTINUE;
		}
		return FCK_TEST_APP_RESULT_DONE;
	}

	const sht_buffer_upload_desc mvp_upload = {.data = &app->mvp, .size = sizeof(app->mvp), .count = 1};
	driver.vt->bss->upload_buffer(app->bss, 0, &mvp_upload);

	const sht_image_upload_desc image_upload = {.views = app->texture_view, .samplers = app->sampler};
	driver.vt->bss->upload_image(app->bss, 1, &image_upload);

	sht_viewport viewport;
	viewport.offset.x = 0.0f;
	viewport.offset.y = 0.0f;
	viewport.depth.min = (float)0.0f;
	viewport.depth.max = (float)1.0f;

	sht_scissor scissor;
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent = viewport.extent = swapchain.vt->extent(swapchain);

	sht_command_buffer command_buffer = command->acquire(driver, frame_index);
	if (command->is_ok(command_buffer))
	{
		sht_render_desc desc = (sht_render_desc){
			.colour = {.view = color_target, .load_op = sht_clear, .store_op = sht_store, .clear_value = {0.0f, 0.0f, 0.2f, 1.0f}},
			.depth = {.view = app->depth_view, .load_op = sht_clear, .store_op = sht_dont_care, .clear_value = 1.0f},
		};
		sht_render_pass render_pass = command->render_pass->begin(command_buffer, &desc);
		if (command->render_pass->is_ok(render_pass))
		{
			command->viewport(command_buffer, &viewport);
			command->scissor(command_buffer, &scissor);

			command->bss(command_buffer, app->bss);
			command->graphics_pipeline(command_buffer, app->pipeline);
			command->vertex_buffer(command_buffer, &app->vertices.buffer, 0);
			command->index_buffer(command_buffer, &app->indices.buffer, 0);

			command->draw_indexed(command_buffer, sht_draw_indexed_params{
													  .first_index = 0,
													  .first_instance = 0,
													  .index_count = app->indices.count,
													  .instance_count = 1,
													  .vertex_offset = 0,
												  });
			command->render_pass->end(command_buffer);
		}
		command->submit(command_buffer, sht_queue_graphic);
	}
	return FCK_TEST_APP_RESULT_CONTINUE;
}

fck_test_app_result fck_test_app_app_tick(void *app_state)
{
	fck_test_app_application *app = (fck_test_app_application *)app_state;
	// fck_input_poll(app->input, e, {
	//	if (app->input->is(e->source, "physical-keyboard"))
	//	{
	//		if (e->description->id == fck_pkey_escape)
	//		{
	//			if (e->data.scalar > 0.0f)
	//			{
	//				return FCK_TEST_APP_RESULT_DONE;
	//			}
	//		}
	//	}
	// });

	fck_test_app_result result = fck_test_app_app_draw(app);
	return result;
}

void fck_test_app_app_quit(void *app_state, fck_test_app_result result)
{
	fck_test_app_application *app = (fck_test_app_application *)app_state;
	sht_driver driver = app->driver;
	sht_memory *mem = driver.vt->memory(driver);

	driver.vt->idle(driver);

	driver.vt->graphics_pipeline->destroy(app->pipeline);
	driver.vt->bss->destroy(&app->bss);
	mem->image->discard(mem->bump, &app->depth_view);
	mem->image->destroy(mem->bump, &app->depth_image);
	mem->image->discard(mem->bump, &app->texture_view);
	mem->image->destroy(mem->bump, &app->texture_image);
	driver.vt->destroy_sampler(driver, &app->sampler);
	mem->free(mem->bump, &app->vertices.buffer);
	mem->free(mem->bump, &app->indices.buffer);

	app->driver.vt->shutdown(&app->driver);
	app->instance.vt->unload(&app->instance);
}

int main(int argc, char *argv[])
{
	struct fck_app_api *app = NULL;
	fck_test_app_result result = fck_test_app_app_init((void **)&app, argc, argv);
	for (;;)
	{
		if (result != FCK_TEST_APP_RESULT_CONTINUE)
		{
			break;
		}
		result = fck_test_app_app_tick(app);
	}
	fck_test_app_app_quit(app, result);
	return 0;
}
