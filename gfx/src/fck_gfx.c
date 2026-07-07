
#include "fck_gfx.h"

// TODO
#include <fck_glsl_reflection.h>

#include <fck_os.h>
#include <fck_shader.h>

#include <sht_render.h>

#include <kll.h>
#include <kll_malloc.h>

#include <fckc_assert.h>
#include <fckc_inttypes.h>

#include <stddef.h>

#include <fck_apis.h>

static fck_api_registry *apis;

typedef struct fck_gfx_internal
{
	sht_bss bss;
	sht_graphics_pipeline pipeline;
} fck_gfx_internal;

static fckc_size_t fck_gfx_bindings_add(sht_stage_flags stage, const fck_glsl_reflection_variable *var, sht_binding *bindings,
                                        fckc_size_t count, fckc_size_t capacity)
{
	fck_assert(var->binding >= 0);
	const fck_glsl_reflection_type *type = var->type;

	for (fckc_size_t index = 0; index < count; index++)
	{
		sht_binding *binding = bindings + index;
		if (binding->id == var->binding)
		{
			binding->stages = binding->stages | stage;
			return count;
		}
	}

	fck_glsl_reflection_api *glsl_reflection = (fck_glsl_reflection_api *)apis->find(fck_glsl_reflection_api_name);

	sht_binding *binding = bindings + count;
	if (sht_test(var->qualifiers, fck_glsl_reflection_declaration_qualifier_uniform))
	{
		fck_assert(count < capacity);
		binding->id = var->binding;
		binding->stages = stage;
		binding->type = sht_binding_uniform;
		if (glsl_reflection->is(type, "sampler2D"))
		{
			binding->type = sht_binding_readonly_image;
		}
		count = count + 1;
	}
	if (sht_test(var->qualifiers, fck_glsl_reflection_declaration_qualifier_buffer))
	{
		fck_assert(count < capacity);
		binding->id = var->binding;
		binding->stages = stage;
		binding->type = sht_binding_storage;
		count = count + 1;
	}
	return count;
}

static struct fck_gfx fck_gfx_api_create(kll_allocator *allocator, sht_driver *driver, const fck_gfx_create_info *info)
{
	fck_gfx_internal *gfx = (fck_gfx_internal *)kll_malloc(allocator, sizeof(*gfx));

	fck_shader_api *shader = (fck_shader_api *)apis->find(fck_shader_api_name);

	fck_shader_compiler compiler = shader->create();
	fck_assert(shader->is_ok(compiler));

	const char *vertex_path = info->vertex->path;
	const char *vertex_name = info->vertex->name;

	const char *fragment_path = info->fragment->path;
	const char *fragment_name = info->fragment->name;

	fck_file vert_file = os->fs->open(vertex_path, "r");
	fck_shader_desc vert_desc = (fck_shader_desc){fck_shader_vertex, vertex_name, "main"};
	fck_glsl_object vert = {0};
	vert = compiler.create_glsl_from_file(&compiler, &vert_desc, &vert_file);

	fck_file frag_file = os->fs->open(fragment_path, "r");
	fck_shader_desc frag_desc = (fck_shader_desc){fck_shader_fragment, fragment_name, "main"};
	fck_glsl_object frag = {0};
	frag = compiler.create_glsl_from_file(&compiler, &frag_desc, &frag_file);

	// Setup Reflection
	sht_binding bindings[16];
	fckc_size_t bindings_count = 0;

	fck_glsl_reflection_api *glsl_reflection = (fck_glsl_reflection_api *)apis->find(fck_glsl_reflection_api_name);
	{

		struct fck_glsl_reflection *reflection = glsl_reflection->reflect(vert.generic.source, fck_glsl_reflection_global);
		const fck_glsl_reflection_type *global = glsl_reflection->type_of(reflection, fck_glsl_reflection_global);
		const fck_glsl_reflection_variable *current = global->first;
		while (current)
		{
			if (current->binding >= 0)
			{
				bindings_count = fck_gfx_bindings_add(sht_stage_vertex_shader, current, bindings, bindings_count, fck_arraysize(bindings));
			}
			current = current->next;
		}
		glsl_reflection->free(reflection);
	}
	{
		struct fck_glsl_reflection *reflection = glsl_reflection->reflect(frag.generic.source, fck_glsl_reflection_global);
		const fck_glsl_reflection_type *global = glsl_reflection->type_of(reflection, fck_glsl_reflection_global);
		const fck_glsl_reflection_variable *current = global->first;
		while (current)
		{
			if (current->binding >= 0)
			{
				bindings_count =
					fck_gfx_bindings_add(sht_stage_fragment_shader, current, bindings, bindings_count, fck_arraysize(bindings));
			}
			current = current->next;
		}
		glsl_reflection->free(reflection);
	}

	// sht_binding const *bindings;
	sht_binding_desc binding_desc = {.bindings = bindings, .count = bindings_count};
	gfx->bss = driver->vt->bss->create(*driver, &binding_desc);

	// TODO: Deprecate
	sht_vertex_desc vertex_desc = {
		.stride = 0,
		.bindings = NULL,
		.count = 0,
	};

	sht_raster_desc raster_desc = {
		.cull_mode = sht_cull_mode_none,
		.topology = sht_triangle_list,
		.color = sht_format_b8g8r8a8_unorm,
		.depth = sht_format_undefined, // sht_format_d16_unorm,
	};
	if (info->has_depth)
	{
		raster_desc.depth = sht_format_d16_unorm;
	}

	const sht_graphic_desc graphic_desc = {
		.fragment = &frag.generic,
		.vertex = &vert.generic,
		.vertex_desc = &vertex_desc,
		.raster = raster_desc,
	};

	os->fs->close(vert_file);
	os->fs->close(frag_file);

	gfx->pipeline = driver->vt->graphics_pipeline->create(*driver, gfx->bss, &graphic_desc);

	compiler.destroy(&compiler, &vert.generic);
	compiler.destroy(&compiler, &frag.generic);
	compiler.shutdown(&compiler);
	return (fck_gfx){.handle = gfx};
}

static struct sht_bss *fck_gfx_api_bss(fck_gfx gfx)
{
	fck_gfx_internal *gfx_internal = (fck_gfx_internal *)gfx.handle;
	return &gfx_internal->bss;
}

static struct sht_graphics_pipeline *fck_gfx_api_pipeline(fck_gfx gfx)
{
	fck_gfx_internal *gfx_internal = (fck_gfx_internal *)gfx.handle;
	return &gfx_internal->pipeline;
}

static fck_gfx_api gfx_api = {
	.create = fck_gfx_api_create,
	.bss = fck_gfx_api_bss,
	.pipeline = fck_gfx_api_pipeline,
};

FCK_EXPORT_API fck_gfx_api *fck_gfx_load(fck_api_registry *registry, void *old)
{
	(void)old;
	apis = registry;
	registry->add(fck_gfx_api_name, &gfx_api);
	return &gfx_api;
}