
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
#include <sht_render.h>

#include "reflection.h"

#include <kll.h>
#include <kll_format.h>
#include <kll_malloc.h>

#include <stdio.h>

#include <ctype.h>
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

typedef struct app_screen
{
	float width;
	float height;
} app_screen;

typedef struct app_config
{
	fckc_i32 gradient;
	fckc_i32 is_sdf;
} app_config;

typedef struct app_quad_transform
{
	float x;
	float y;
	float z;
	float rotation;
	float width;
	float height;
	float scale;
} app_quad_transform;

typedef struct app_line_transform
{
	float sx;
	float sy;
	float ex;
	float ey;
	float z;
	float thickness;
	float scale;
} app_line_transform;

// This will backlash. Try to keep them the same size, else the stride and all that stuff needs to stay opaque! We are lucky for now :D
typedef union app_shape_transform {
	app_quad_transform quad;
	app_line_transform line;
} app_shape_transform;

typedef struct app_quads
{
	app_quad_transform transforms[64];
	fckc_u32 count;
} app_quads;

static void app_quads_add(app_quads *quads, float x, float y)
{
	const app_quad_transform init_transform = {
		.x = x,
		.y = y,
		.z = 0.0f,
		.rotation = 0.0f,
		.width = 100.0f,
		.height = 100.0f,
		.scale = 1.0f,
	};

	app_quad_transform *transform = quads->transforms + quads->count;
	*transform = init_transform;
	quads->count = quads->count + 1;
}

typedef struct app_lines
{
	app_line_transform transforms[64];
	fckc_u32 count;
} app_lines;

static void app_lines_add(app_lines *quads, float sx, float sy, float ex, float ey, float thickness)
{
	const app_line_transform init_transform = {
		.sx = sx,
		.sy = sy,
		.ex = ex,
		.ey = ey,
		.z = 0.0f,
		.thickness = thickness,
		.scale = 1.0f,
	};

	app_line_transform *transform = quads->transforms + quads->count;
	*transform = init_transform;
	quads->count = quads->count + 1;
}

typedef enum app_graphics_shape
{
	app_shape_quad,
	app_shape_line,
	app_shape_count,
} app_graphics_shape;

static const char *app_graphics_shape_to_string(app_graphics_shape primitive)
{
	switch (primitive)
	{
	case app_shape_quad:
		return "app_shape_quad";
	case app_shape_line:
		return "app_shape_line";
	case app_shape_count:
		return "app_shape_line";
	}
	return "app_shape_unknown";
}

typedef enum app_graphics_style
{
	app_style_solid,
	app_style_textured,
	app_style_rounded,
	app_style_count,
} app_graphics_style;

static const char *app_graphic_material_to_string(app_graphics_style material)
{
	switch (material)
	{
	case app_style_solid:
		return "app_style_solid";
	case app_style_textured:
		return "app_style_textured";
	case app_style_rounded:
		return "app_style_rounded";
	case app_style_count:
		break;
	}
	return "app_style_unknown";
}

static fckc_size_t app_graphic_pipeline_bindings(app_graphics_shape primitive, app_graphics_style material,
                                                 sht_binding const **out_bindings)
{
	switch (primitive)
	{
	case app_shape_quad:
	case app_shape_line:
		switch (material)
		{
		case app_style_solid:
		case app_style_rounded:
			static const sht_binding bindings[] = {
				{.id = 0, .type = SHT_BINDING_UNIFORM, .stages = SHT_STAGE_VERTEX_SHADER},
				{.id = 1, .type = SHT_BINDING_STORAGE, .stages = SHT_STAGE_VERTEX_SHADER},
				{.id = 2, .type = SHT_BINDING_UNIFORM, .stages = SHT_STAGE_VERTEX_SHADER},
				{.id = 3, .type = SHT_BINDING_READ_ONLY_IMAGE, .stages = SHT_STAGE_FRAGMENT_SHADER},
				{.id = 5, .type = SHT_BINDING_UNIFORM, .stages = SHT_STAGE_VERTEX_SHADER | SHT_STAGE_FRAGMENT_SHADER},
				// TODO: Configuration binding
			};
			*out_bindings = bindings;
			return fck_arraysize(bindings);
		case app_style_textured:
			static const sht_binding textured_bindings[] = {
				{.id = 0, .type = SHT_BINDING_UNIFORM, .stages = SHT_STAGE_VERTEX_SHADER},
				{.id = 1, .type = SHT_BINDING_STORAGE, .stages = SHT_STAGE_VERTEX_SHADER},
				{.id = 2, .type = SHT_BINDING_UNIFORM, .stages = SHT_STAGE_VERTEX_SHADER},
				{.id = 3, .type = SHT_BINDING_READ_ONLY_IMAGE, .stages = SHT_STAGE_FRAGMENT_SHADER},
				{.id = 5, .type = SHT_BINDING_UNIFORM, .stages = SHT_STAGE_VERTEX_SHADER | SHT_STAGE_FRAGMENT_SHADER},
			};
			*out_bindings = textured_bindings;
			return fck_arraysize(textured_bindings);
		case app_style_count:
			break;
		}
		break;
	case app_shape_count:
		break;
	}
	return 0;
}

typedef struct app_property_string
{
	// If there is a need for a longer string, fuck you
	const char *value;
} app_property_string;

typedef enum app_property_type
{
	// What else do you need for now?
	app_property_float,
	app_property_int,
} app_property_type;

#define app_properties_capacity 16
#define app_property_blocks_capacity 8

typedef struct app_properties
{
	app_property_string keys[app_properties_capacity];
	app_property_type types[app_properties_capacity];
	fckc_size_t offsets[app_properties_capacity];
	// TODO: deal with capacity...
	fckc_u8 buffer[sizeof(float) * app_properties_capacity];
	fckc_size_t size;
} app_properties;

typedef struct app_property_block
{
	app_property_string name;
	app_properties properties;
} app_property_block;

typedef struct app_prroperty_blocks
{
	app_property_block values[app_property_blocks_capacity];
	fckc_size_t count;
} app_prroperty_blocks;

static void *app_property_structure_reserve(app_properties *properties, const char *name, fckc_size_t size)
{
	for (fckc_size_t index = 0; index < app_properties_capacity; index++)
	{
		app_property_string *key = properties->keys + index;
		if (key->value == NULL)
		{
			key->value = name;
		}
		if (strcmp(key->value, name) == 0)
		{
			app_property_type *type = properties->types + index;
			*type = app_property_float;

			// Do not forget alignup later! :)
			fckc_size_t *offset = properties->offsets + index;
			*offset = properties->size;
			properties->size = properties->size + size;

			fckc_u8 *dst = properties->buffer + *offset;
			memset(dst, 0, size);
			return dst;
		}
	}
	return NULL;
}

static void app_property_structure_set_float(app_properties *properties, const char *name, float value)
{
	void *destination = app_property_structure_reserve(properties, name, sizeof(value));
	if (destination)
	{
		memcpy(destination, &value, sizeof(value));
	}
}

static void app_property_structure_set_int(app_properties *properties, const char *name, int value)
{
	void *destination = app_property_structure_reserve(properties, name, sizeof(value));
	if (destination)
	{
		memcpy(destination, &value, sizeof(value));
	}
}

static void app_property_structure_upload(app_properties *properties, sht_driver driver, sht_bss bss)
{
	if (properties->size == 0)
	{
		return;
	}

	const sht_buffer_upload_desc upload = {
		.data = properties->buffer,
		.size = properties->size,
		.count = 1,
	};
	// TODO: Hardcoded binding!!!
	driver.vt->bss->upload_buffer(bss, 5, &upload);
}

typedef struct app_graphic_pipeline
{
	// ... I think with alignment this one is even generic enough to work as an arena or alloctor lol
	kll_arena *strings;

	sht_graphics_pipeline pipeline;
	sht_bss bss;

	app_properties properties;

	app_shape_transform *transforms;
	fckc_u32 count;
	fckc_u32 capacity;
} app_graphic_pipeline;

typedef struct app_graphics
{
	fck_shader_api *shader;
	sht_driver driver;
	app_graphic_pipeline values[app_shape_count][app_style_count];
} app_graphics;

static void app_graphics_init(app_graphics *graphics, fck_shader_api *shader, sht_driver driver)
{
	graphics->shader = shader;
	graphics->driver = driver;
}

static void app_graphics_create(app_graphics *pipelines, app_graphics_shape primitive, app_graphics_style material, const char *vertex,
                                const char *fragment)
{
	fck_shader_compiler compiler = pipelines->shader->create();
	if (!pipelines->shader->is_ok(compiler))
	{
		return;
	}

	app_graphic_pipeline *gfx = &pipelines->values[primitive][material];

	fck_file vert_file = os->fs->open(vertex, "r");
	fck_shader_desc vert_desc = (fck_shader_desc){FCK_SHADER_VERTEX, app_graphics_shape_to_string(primitive), "main"};

	fck_file frag_file = os->fs->open(fragment, "r");
	fck_shader_desc frag_desc = (fck_shader_desc){FCK_SHADER_FRAGMENT, app_graphic_material_to_string(material), "main"};
	fck_glsl_object vert = {0};
	fck_glsl_object frag = {0};
	vert = compiler.create_glsl_from_file(&compiler, &vert_desc, &vert_file);
	frag = compiler.create_glsl_from_file(&compiler, &frag_desc, &frag_file);

	sht_binding const *bindings;
	const fckc_size_t binding_count = app_graphic_pipeline_bindings(primitive, material, &bindings);

	sht_binding_desc binding_desc = {.bindings = bindings, .count = binding_count};
	gfx->bss = pipelines->driver.vt->bss->create(pipelines->driver, &binding_desc);

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

	os->fs->close(vert_file);
	os->fs->close(frag_file);

	gfx->pipeline = pipelines->driver.vt->graphics_pipeline->create(pipelines->driver, gfx->bss, &graphic_desc);

	const fckc_size_t capacity = 64;
	gfx->transforms = (app_shape_transform *)kll_malloc(kll->system, sizeof(*gfx->transforms) * capacity);
	gfx->capacity = capacity;
	gfx->count = 0;

	// Setup Reflection
	{
		struct fck_glsl_reflection *reflection = glsl_reflection->reflect(frag.generic.source, fck_glsl_reflection_global);
		const fck_glsl_reflection_type *global_type = glsl_reflection->type_of(reflection, fck_glsl_reflection_global);
		const fck_glsl_reflection_type *float_type = glsl_reflection->type_of(reflection, "float");
		const fck_glsl_reflection_type *int_type = glsl_reflection->type_of(reflection, "int");
		const fck_glsl_reflection_type* sampler2D_type = glsl_reflection->type_of(reflection, "sampler2D");

		gfx->strings = kll->arena->create(kll->system, 256);

		const fck_glsl_reflection_variable *current = global_type->first;
		while (current)
		{
			if (current->type->binding >= 0)
			{
				os->io->log("Binding: %d -- Type: %s - Name: %s", current->type->binding, current->type->name, current->name);
				const fck_glsl_reflection_variable *child_current = current->type->first;
				while (child_current)
				{
					const fck_glsl_reflection_variable *child = child_current;
					child_current = child_current->next;

					if (child->type == float_type)
					{
						const char *name = kll_format(gfx->strings, child->name);
						app_property_structure_set_float(&gfx->properties, name, 0.0f);
						continue;
					}
					if (child->type == int_type)
					{
						const char *name = kll_format(gfx->strings, child->name);
						app_property_structure_set_int(&gfx->properties, name, 0);
						continue;
					}
				}
			}
			else
			{
				os->io->log("Type: %s - Name: %s", current->type->name, current->name);
			}

			current = current->next;
		}
		glsl_reflection->free(reflection);
	}

	compiler.destroy(&compiler, &vert.generic);
	compiler.destroy(&compiler, &frag.generic);
	compiler.shutdown(&compiler);
}

static void app_graphics_add_line(app_graphics *graphics, app_graphics_style material, float sx, float sy, float ex, float ey,
                                  float thickness)
{
	app_graphic_pipeline *g = &graphics->values[app_shape_line][material];
	fck_assert(g->count < g->capacity);

	const app_line_transform init_transform = {
		.sx = sx,
		.sy = sy,
		.ex = ex,
		.ey = ey,
		.z = 0.0f,
		.thickness = thickness,
		.scale = 1.0f,
	};

	app_shape_transform *transform = g->transforms + g->count;
	transform->line = init_transform;
	g->count = g->count + 1;
}

static void app_graphics_add_quad(app_graphics *graphics, app_graphics_style material, float x, float y)
{
	app_graphic_pipeline *g = &graphics->values[app_shape_quad][material];
	fck_assert(g->count < g->capacity);

	const app_quad_transform init_transform = {
		.x = x,
		.y = y,
		.z = 0.0f,
		.rotation = 0.0f,
		.width = 100.0f,
		.height = 100.0f,
		.scale = 1.0f,
	};
	app_shape_transform *transform = g->transforms + g->count;
	transform->quad = init_transform;
	g->count = g->count + 1;
}

int main(int argc, char **argv)
{
	load_config(argc, argv);

	purge_files("temp-*.dll");

	// app_properties properties = {0};
	// app_property_structure_set_float(&properties, "r", 0.0f);
	// app_property_structure_set_float(&properties, "g", 0.0f);
	// app_property_structure_set_float(&properties, "b", 0.0f);
	// app_property_structure_set_float(&properties, "a", 0.0f);
	// app_property_structure_set_float(&properties, "roundness", 0.0f);

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

	fck_window window = os->win->create("Test", 1280, 720);

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

	sht_graphics_pipeline graphic_pipelines;

	sht_memory *memory = driver.vt->memory(driver);
	sht_swapchain swapchain = driver.vt->swapchain(driver);
	sht_command_buffer_vt *command = driver.vt->command_buffer;

	sht_elements indices = {0};

	sht_sampler sampler = {0};
	sht_image texture_image = {0};
	sht_image_view texture_view = {0};

	{
		sampler = driver.vt->create_sampler(driver, sht_filter_linear);
		texture_image = memory->image->create(memory->bump,
		                                      &(sht_image_configuration){
												  .format = SHT_FORMAT_R8G8B8A8_UNORM,
												  .width = 4,
												  .height = 1,
												  .transfer = SHT_TRANSFER_TARGET,
												  .usage = SHT_IMAGE_USAGE_SAMPLED,
											  },
		                                      SHT_MEMORY_GPU);
		texture_view = memory->image->view(memory->bump, texture_image, SHT_FORMAT_R8G8B8A8_UNORM);

		fckc_u32 pixels[] = {0xFF0000FF, 0xFF00FF00, 0xFFFF0000, 0xFFFFFFFF};
		driver.vt->upload_image(driver, &texture_image, pixels, sizeof(pixels));
	}

	{
		fckc_u32 index_data[] = {0, 1, 2, 1, 3, 2};
		indices.count = fck_arraysize(index_data);
		indices.buffer = memory->malloc(memory->bump, &sht_buffer_target(SHT_BUFFER_USAGE_INDEX, sizeof(index_data)), SHT_MEMORY_GPU);
		driver.vt->upload_buffer(driver, &indices.buffer, index_data, sizeof(index_data));
	}

	app_graphics graphics = {0};
	app_graphics_init(&graphics, shader, driver);
	app_graphics_create(&graphics, app_shape_quad, app_style_solid, fck_resource_path "quad.vert", fck_resource_path "solid.frag");
	app_graphics_create(&graphics, app_shape_quad, app_style_textured, fck_resource_path "quad.vert", fck_resource_path "textured.frag");
	app_graphics_create(&graphics, app_shape_quad, app_style_rounded, fck_resource_path "quad.vert", fck_resource_path "round.frag");
	app_graphics_create(&graphics, app_shape_line, app_style_solid, fck_resource_path "line.vert", fck_resource_path "solid.frag");
	app_graphics_create(&graphics, app_shape_line, app_style_textured, fck_resource_path "line.vert", fck_resource_path "textured.frag");
	app_graphics_create(&graphics, app_shape_line, app_style_rounded, fck_resource_path "line.vert", fck_resource_path "round.frag");

	app_graphics_add_quad(&graphics, app_style_textured, 0.0f, 0.0f);
	app_graphics_add_quad(&graphics, app_style_solid, 300.0f, 0.0f);
	app_graphics_add_quad(&graphics, app_style_rounded, 500.0f, 0.0f);

	app_graphics_add_line(&graphics, app_style_solid, -50.0f, -50.0f, 50.0f, 50.0f, 16.0f);
	app_graphics_add_line(&graphics, app_style_solid, 50.0f, -50.0f, -50.0f, 50.0f, 16.0f);

	app_graphics_add_line(&graphics, app_style_textured, -300.0f, 0.0f, -350.0f, 100.0f, 24.0f);
	app_graphics_add_line(&graphics, app_style_rounded, -500.0f, 0.0f, -550.0f, 100.0f, 28.0f);

	// How do I scale this to vertex and fragment shader stuff...
	// HMMMMMMM
	for (fckc_size_t index = 0; index < app_shape_count; index++)
	{
		app_properties *properties = &graphics.values[index][app_style_rounded].properties;
		app_property_structure_set_float(properties, "roundness", 0.5f);
	}

	int is_running = 1;
	while (is_running)
	{
		// TODO: Make render-vk hotreloadable :)
		// How hard can it be?
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
			if (e->source->type == fck_input_source_mouse)
			{
				if (e->description->id == fck_mouse_left)
				{
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

					command->index_buffer(command_buffer, &indices.buffer, 0);

					for (fckc_size_t shape_index = 0; shape_index < app_shape_count; shape_index++)
					{
						for (fckc_size_t style_index = 0; style_index < app_style_count; style_index++)
						{
							app_graphic_pipeline *graphic = &graphics.values[shape_index][style_index];
							if (driver.vt->graphics_pipeline->is_ok(graphic->pipeline))
							{
								if (graphic->count > 0)
								{
									const sht_buffer_upload_desc screen_upload = {
										.data = &screen,
										.size = sizeof(screen),
										.count = 1,
									};
									const sht_buffer_upload_desc quads_upload = {
										.data = graphic->transforms,
										.size = sizeof(*graphic->transforms),
										.count = graphic->count,
									};

									app_config config = {.gradient = 0, .is_sdf = style_index == app_style_rounded};

									const sht_buffer_upload_desc config_upload = {
										.data = &config,
										.size = sizeof(config),
										.count = 1,
									};
									const sht_image_upload_desc image_upload = {
										.views = texture_view,
										.samplers = sampler,
									};

									driver.vt->bss->upload_buffer(graphic->bss, 0, &screen_upload);
									driver.vt->bss->upload_buffer(graphic->bss, 1, &quads_upload);
									driver.vt->bss->upload_buffer(graphic->bss, 2, &config_upload);
									driver.vt->bss->upload_image(graphic->bss, 3, &image_upload);

									app_property_structure_upload(&graphic->properties, driver, graphic->bss);

									command->bss(command_buffer, graphic->bss);

									command->graphics_pipeline(command_buffer, graphic->pipeline);

									sht_draw_indexed_desc indexed = {
										.first_index = 0,
										.first_instance = 0,
										.index_count = to_u32(indices.count),
										.instance_count = graphic->count,
										.vertex_offset = 0,
									};
									command->draw_indexed(command_buffer, &indexed);
								}
							}
						}
					}

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
