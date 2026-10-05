
#include "fck_sprite.h"

#include <fck_texture.h>

#include <fckc_apidef.h>
#include <fckc_assert.h>
#include <fckc_inttypes.h>
#include <fckc_math.h>

#include <fck_apis.h>
#include <kll.h>
#include <kll_format.h>
#include <kll_malloc.h>

#include <fck_db.h>
#include <fck_gfx.h>
#include <sht_render.h>

#include <fck_dynarr.h>
#include <fck_hash.h>
#include <fck_set.h>

static fck_api_registry *apis;

typedef struct fck_sprite_screen
{
	float width;
	float height;
	float sprite_width;
	float sprite_height;
} fck_sprite_screen;

typedef struct fck_batchy_batch_private
{
	fck_batchy_batch               id;
	fck_batchy_batch_configuration config;
	fck_sprite_transform          *transforms;
} fck_batchy_batch_private;

typedef struct fck_batchy_private
{
	struct kll_allocator *allocator;
	struct fck_db        *assets;
	struct sht_driver    *driver;

	fck_batchy_batch_private *set;

	fck_db_id id;

	fck_gfx      gfx;
	sht_elements indices;
} fck_batchy_private;

static fck_batchy fck_batchy_api_create(struct kll_allocator *allocator, const fck_batchy_create_args *args)
{
	// Base
	const fckc_size_t capacity = 16;

	fck_batchy batchy   = {0};
	batchy.o            = (fck_batchy_private *)kll_malloc(allocator, sizeof(*batchy.o));
	batchy.o->allocator = allocator;
	batchy.o->assets    = args->db;
	batchy.o->driver    = args->driver;

	batchy.o->set = fck_set_new(fck_batchy_batch_private, allocator, capacity);

	// Graphics
	fck_gfx_api *gfx = (fck_gfx_api *)apis->find(fck_gfx_api_name);
	fck_db_api  *db  = (fck_db_api *)apis->find(fck_db_api_name);

	db->asset->setup(*args->db, "batchy", fck_sprite_resource_path);

	batchy.o->id = db->object->create(*args->db);

	const fck_db_asset *sprite_vs = db->asset->find(*args->db, "batchy/sprite.vert");
	const fck_db_asset *sprite_fs = db->asset->find(*args->db, "batchy/textured.frag");

	sht_driver  driver = *args->driver;
	sht_memory *memory = args->driver->vt->memory(*args->driver);

	const fck_gfx_create_info create_info = {.flags = fck_gfx_target_all, .vertex = sprite_vs, .fragment = sprite_fs};
	batchy.o->gfx                         = gfx->create(kll->system, args->driver, &create_info);

	{
		const fckc_u32 index_data[]            = {0, 1, 2, 1, 3, 2};
		batchy.o->indices.count                = fck_arraysize(index_data);
		sht_buffer_configuration buffer_config = sht_buffer_target(sht_buffer_usage_index, sizeof(index_data));
		batchy.o->indices.buffer               = memory->malloc(memory->bump, &buffer_config, sht_memory_gpu);
		driver.vt->upload_buffer(driver, &batchy.o->indices.buffer, index_data, sizeof(index_data));
	}

	return batchy;
}

static fck_batchy_batch fck_batchy_api_add(fck_batchy batchy, const fck_batchy_batch_configuration *config)
{
	fckc_u64 hash = fck_hash((const char *)&config->asset, sizeof(config->asset));
	hash          = fck_hash_combine(hash, fck_hash((const char *)&config->sprite_width, sizeof(config->sprite_width)));
	hash          = fck_hash_combine(hash, fck_hash((const char *)&config->sprite_height, sizeof(config->sprite_height)));

	fck_batchy_batch batch = {.value = hash};

	const fckc_size_t has = fck_set_find(batchy.o->set, batch.value);
	if (!has)
	{
		// Add - Yay
		fck_sprite_transform    *transforms = fck_dynarr_new(fck_sprite_transform, batchy.o->allocator, 32);
		fck_batchy_batch_private entry      = {.id = batch, .config = *config, .transforms = transforms};

		fck_set_add(batchy.o->set, hash, entry);
	}
	else
	{
		// Update - Urgh, the function name
		fck_batchy_batch_private *entry = batchy.o->set + has - 1;
		entry->config                   = *config;
	}

	return batch;
}

static void fck_batchy_api_clear(fck_batchy batchy, fck_batchy_batch batch)
{
	const fckc_size_t has = fck_set_find(batchy.o->set, batch.value);
	if (has)
	{
		fck_batchy_batch_private *entry = batchy.o->set + has - 1;
		fck_dynarr_clear(entry->transforms);
	}
	// TODO: See how we handle issues
}

static void fck_batchy_api_remove(fck_batchy batchy, fck_batchy_batch batch)
{
	const fckc_size_t has = fck_set_find(batchy.o->set, batch.value);
	if (has)
	{
		fck_batchy_batch_private *entry = batchy.o->set + has - 1;
		fck_dynarr_destroy(entry->transforms);
		fck_set_remove(batchy.o->set, batch.value);
	}
}

static void fck_batchy_api_destroy(fck_batchy batchy)
{
	fckc_size_t current = fck_set_begin(batchy.o->set);
	while (fck_set_next(batchy.o->set, current))
	{
		fck_batchy_batch_private *entry = batchy.o->set + current;
		fck_dynarr_destroy(entry->transforms);
	}
	fck_set_destroy(batchy.o->set);
	kll_free(batchy.o->allocator, batchy.o);
}

static fckc_size_t fck_batchy_api_count(fck_batchy batchy)
{
	const fckc_size_t count = fck_set_count(batchy.o->set);
	return count;
}

static fck_sprite_transform *fck_batchy_api_batchup(fck_batchy batchy, fck_batchy_batch batch)
{
	const fckc_size_t has = fck_set_find(batchy.o->set, batch.value);
	if (has)
	{
		fck_batchy_batch_private  *entry = batchy.o->set + has - 1;
		const fck_sprite_transform zero  = {0};

		fck_dynarr_add(entry->transforms, zero);
		return entry->transforms + fck_dynarr_count(entry->transforms) - 1;
	}
	return NULL;
}

static int fck_batchy_api_iterate(fck_batchy batchy, fck_batchy_batch **batch)
{
	// TODO:
	fckc_size_t current = *batch != NULL ? *batch - batchy.o->set : fck_set_begin(batchy.o->set);
	if (!fck_set_next(batchy.o->set, current))
	{
		return 0;
	}
	*batch = batchy.o->set + current;
	return 1;
}

static int fck_batchy_api_inspect(fck_batchy batchy, fck_batchy_batch batch, fck_batchy_batch_configuration *config)
{
	const fckc_size_t has = fck_set_find(batchy.o->set, batch.value);
	if (has)
	{
		fck_batchy_batch_private *entry = batchy.o->set + has - 1;

		*config = entry->config;
		return 1;
	}
	return 0;
}

static fckc_size_t fck_batchy_api_batch(fck_batchy batchy, fck_batchy_batch batch, fck_sprite_transform **transforms)
{
	const fckc_size_t has = fck_set_find(batchy.o->set, batch.value);
	if (has)
	{
		fck_batchy_batch_private *entry = batchy.o->set + has - 1;

		*transforms = entry->transforms;
		return fck_dynarr_count(entry->transforms);
	}
	return 0;
}

static const struct fck_db_asset *fck_batchy_api_save(fck_batchy batchy, const char *scope, const char *path)
{
	// TODO:
	return NULL;
}

static int fck_batchy_api_present(void *userdata, const struct fck_gfx_args *args)
{
	fck_batchy *batchy = (fck_batchy *)userdata;

	sht_driver *driver = batchy->o->driver;
	fck_assert(driver == args->driver);

	sht_command_buffer_vt *command     = driver->vt->command_buffer;
	const sht_render_pass  render_pass = command->render_pass->begin(*args->commands, args->suggestion);
	if (!command->render_pass->is_ok(render_pass))
	{
		return 0;
	}

	const sht_swapchain swapchain = driver->vt->swapchain(*driver);
	const sht_extent    extent    = swapchain.vt->extent(swapchain);

	{
		sht_viewport viewport;
		viewport.offset.x  = 0.0f;
		viewport.offset.y  = 0.0f;
		viewport.depth.min = (float)0.0f;
		viewport.depth.max = (float)1.0f;
		viewport.extent    = extent;
		command->viewport(*args->commands, &viewport);

		sht_scissor scissor;
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		scissor.extent   = extent;
		command->scissor(*args->commands, &scissor);
	}

	command->index_buffer(*args->commands, &batchy->o->indices.buffer, 0);

	fck_gfx_api     *gfx = (fck_gfx_api *)apis->find(fck_gfx_api_name);
	fck_texture_api *png = (fck_texture_api *)apis->find(fck_texture_api_name);

	// TODO: Prefer usage of public API
	fckc_size_t current = fck_set_begin(batchy->o->set);
	while (fck_set_next(batchy->o->set, current))
	{
		fck_batchy_batch_private *entry = batchy->o->set + current;
		const fckc_size_t         count = fck_dynarr_count(entry->transforms);

		if (count > 0)
		{
			const fck_db_asset *asset = entry->config.asset;

			sht_bss               *bss      = gfx->bss(batchy->o->gfx);
			sht_graphics_pipeline *pipeline = gfx->pipeline(batchy->o->gfx);

			const fck_sprite_screen screen = {
				.width         = (float)extent.width,
				.height        = (float)extent.height,
				.sprite_width  = entry->config.sprite_width,
				.sprite_height = entry->config.sprite_height,
			};
			const sht_buffer_upload_desc screen_upload = {.data = &screen, .size = sizeof(screen), .count = 1};

			const sht_buffer_upload_desc transform_upload = {
				.data  = entry->transforms,
				.size  = sizeof(*entry->transforms),
				.count = count,
			};

			const sht_image_view       *view         = png->asset->gpu(asset);
			const sht_image_upload_desc image_upload = {.view = view};

			driver->vt->bss->upload_buffer(*bss, 0, &screen_upload);
			driver->vt->bss->upload_buffer(*bss, 1, &transform_upload);
			driver->vt->bss->upload_image(*bss, 3, &image_upload);
			command->bss(*args->commands, *bss);

			command->graphics_pipeline(*args->commands, *pipeline);

			const sht_draw_indexed_desc desc = {
				.first_index    = 0,
				.index_count    = to_u32(batchy->o->indices.count),
				.instance_count = count,
				.first_instance = 0,
				.vertex_offset  = 0,
			};

			command->draw_indexed(*args->commands, &desc);
		}
	}

	command->render_pass->end(*args->commands);
	return 1;
}

static fck_batchy_api batchy_api = {
	.create  = fck_batchy_api_create,
	.destroy = fck_batchy_api_destroy,
	.count   = fck_batchy_api_count,
	.add     = fck_batchy_api_add,
	.remove  = fck_batchy_api_remove,
	.clear   = fck_batchy_api_clear,
	.batchup = fck_batchy_api_batchup,
	//.batchdown = fck_batchy_api_batchdown,
    //.sprite    = fck_batchy_api_sprite,
	.iterate = fck_batchy_api_iterate,
	.inspect = fck_batchy_api_inspect,
	.batch   = fck_batchy_api_batch,
	.save    = fck_batchy_api_save,
	.present = fck_batchy_api_present,
};

FCK_EXPORT_API fck_batchy_api *fck_sprite_load(fck_api_registry *registry, void *old)
{
	(void)old;
	apis = registry;
	registry->add(fck_batchy_api_name, &batchy_api);
	return &batchy_api;
}
