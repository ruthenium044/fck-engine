
#include "fck_gfx.h"

// TODO
#include <fck_glsl_reflection.h>

#include <fck_db.h>
#include <fck_os.h>
#include <fck_shader.h>

#include <sht_render.h>

#include <kll.h>
#include <kll_malloc.h>

#include <fckc_assert.h>
#include <fckc_inttypes.h>

#include <fck_apis.h>

#include <stddef.h>
#include <string.h>

static fck_api_registry *apis;

typedef struct fck_gfx_internal
{
	sht_bss bss;
	sht_graphics_pipeline pipeline;

	fck_shader_api *shader;
	sht_driver *driver;
	fck_gfx_create_info info;
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
	binding->id = var->binding;
	binding->stages = stage;
	if (sht_test(var->qualifiers, fck_glsl_reflection_declaration_qualifier_uniform))
	{
		fck_assert(count < capacity);
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
		binding->type = sht_binding_storage;
		count = count + 1;
	}
	return count;
}

static void fck_gfx_initialize(fck_gfx_internal *gfx, sht_driver *driver, const fck_gfx_create_info *info)
{
	fck_shader_api *shader = gfx->shader;

	fck_shader_compiler compiler = shader->create();
	fck_assert(shader->is_ok(compiler));

	fck_glsl_object vert = shader->asset->resolve(info->vertex);
	fck_glsl_object frag = shader->asset->resolve(info->fragment);

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

	gfx->pipeline = driver->vt->graphics_pipeline->create(*driver, gfx->bss, &graphic_desc);
	compiler.shutdown(&compiler);
}

static struct fck_gfx fck_gfx_api_create(kll_allocator *allocator, sht_driver *driver, const fck_gfx_create_info *info)
{
	fck_gfx_internal *gfx = (fck_gfx_internal *)kll_malloc(allocator, sizeof(*gfx));
	memset(gfx, 0, sizeof(*gfx));
	gfx->driver = driver;
	gfx->info = *info;

	fck_shader_api *shader = (fck_shader_api *)apis->find(fck_shader_api_name);
	gfx->shader = shader;

	// fck_gfx_initialize(gfx, driver, info);
	return (fck_gfx){.handle = gfx};
}

void static fck_gfx_api_maybe_reload(fck_gfx gfx)
{
	fck_gfx_internal *gfx_internal = (fck_gfx_internal *)gfx.handle;
	const int vs_dirty = gfx_internal->shader->asset->dirty(gfx_internal->info.vertex);
	const int fs_dirty = gfx_internal->shader->asset->dirty(gfx_internal->info.fragment);
	if (vs_dirty || fs_dirty)
	{
		sht_driver *driver = gfx_internal->driver;
		if (driver->vt->graphics_pipeline->is_ok(gfx_internal->pipeline))
		{
			driver->vt->idle(*driver);
			driver->vt->bss->destroy(&gfx_internal->bss);
			driver->vt->graphics_pipeline->destroy(gfx_internal->pipeline);
		}
		// TODO: We need to implement do not reload on error and keep the old one!!
		fck_gfx_initialize(gfx_internal, gfx_internal->driver, &gfx_internal->info);
	}
}

static struct sht_bss *fck_gfx_api_bss(fck_gfx gfx)
{
	fck_gfx_internal *gfx_internal = (fck_gfx_internal *)gfx.handle;
	fck_gfx_api_maybe_reload(gfx);
	return &gfx_internal->bss;
}

static struct sht_graphics_pipeline *fck_gfx_api_pipeline(fck_gfx gfx)
{
	fck_gfx_internal *gfx_internal = (fck_gfx_internal *)gfx.handle;
	fck_gfx_api_maybe_reload(gfx);
	// In here we can do a nice and dandy resolve for the shaders
	// Else, maybe a fck_gfx_asset would also make sense?
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