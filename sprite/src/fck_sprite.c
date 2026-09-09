
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

#include <string.h>

static fck_api_registry *apis;

typedef struct fck_sprite_batch
{
	kll_allocator *allocator;

	const fck_db_asset *asset;

	fck_sprite_transform *transforms;

	fckc_u32 count;
	fckc_u32 capacity;
} fck_sprite_batch;

typedef struct fck_sprite_batch_index
{
	fckc_u32 value : 31;
	fckc_u32 is_ok : 1;
} fck_sprite_batch_index;

typedef struct fck_sprite_batch_dense_index
{
	fckc_u32 value;
} fck_sprite_batch_dense_index;

typedef struct fck_sprite_stable_batch
{
	fck_sprite_batch base;

	fck_sprite_batch_index       *sparse;
	fck_sprite_batch_dense_index *dense;

	fckc_u32 capacity;
	float    sprite_width;
	float    sprite_height;
} fck_sprite_stable_batch;

typedef struct fck_sprites_internal
{
	kll_allocator *allocator;
	kll_arena     *strings;

	const char **names;

	fck_sprite_stable_batch *batches;
	fckc_u32                 count;
	fckc_u32                 capacity;

	// Render Data
	sht_driver    *driver;
	sht_elements   indices;
	sht_image      white_image;
	sht_image_view white_view;

	fck_gfx gfx;
} fck_sprites_internal;

static fck_sprites_internal *fck_sprites_to_internal(fck_sprites *sprites, fck_sprites_internal *internal_sprites)
{
	const fckc_size_t size = fck_min(sizeof(*sprites), sizeof(*internal_sprites));
	memcpy(internal_sprites, sprites, size);
	return internal_sprites;
}

static fck_sprites *fck_sprites_to_external(fck_sprites_internal *internal_sprites, fck_sprites *sprites)
{
	const fckc_size_t size = fck_min(sizeof(*sprites), sizeof(*internal_sprites));
	memcpy(sprites, internal_sprites, size);
	return sprites;
}

/* Implementation */

static fck_sprite_batch fck_sprite_batch_create(kll_allocator *allocator, const fck_db_asset *asset)
{
	const fck_sprite_batch batch = {
		.allocator = allocator,
		.asset     = asset,
	};
	return batch;
}

static int fck_sprite_batch_destroy(fck_sprite_batch *batch)
{
	if (batch->transforms)
	{
		kll_free(batch->allocator, batch->transforms);
		memset(batch, 0, sizeof(*batch));
		return 1;
	}
	return 0;
}

static fckc_u32 fck_sprite_batch_add(fck_sprite_batch *batch)
{
	if (batch->count >= batch->capacity)
	{
		const fckc_u32        initial       = 8;
		const fckc_u32        next_capacity = batch->capacity ? batch->capacity * 2 : 8;
		fck_sprite_transform *next = (fck_sprite_transform *)kll_malloc(batch->allocator, next_capacity * sizeof(*batch->transforms));
		if (batch->transforms)
		{
			memcpy(next, batch->transforms, batch->capacity * sizeof(*batch->transforms));
			kll_free(batch->allocator, batch->transforms);
		}
		batch->capacity   = next_capacity;
		batch->transforms = next;
	}

	const fckc_u32        at     = batch->count;
	fck_sprite_transform *target = batch->transforms + at;
	memset(target, 0, sizeof(*target));
	batch->count = batch->count + 1;
	return at;
}

static fckc_u32 fck_sprite_batch_index_of(fck_sprite_batch *batch, const fck_sprite_transform *transform)
{
	const fckc_size_t root   = to_size_t(batch->transforms);
	const fckc_size_t target = to_size_t(transform);
	const fckc_size_t offset = (target - root) / sizeof(*transform);
	if (offset < batch->count)
	{
		return offset + 1;
	}
	return 0;
}

static int fck_sprite_batch_remove(fck_sprite_batch *batch, fckc_u32 index)
{
	if (index < batch->count)
	{
		const fck_sprite_transform *last    = batch->transforms + batch->count - 1;
		fck_sprite_transform       *current = batch->transforms + index;
		*current                            = *last;
		batch->count                        = batch->count - 1;
		return 1;
	}
	return 0;
}

static fck_sprite_stable_batch fck_sprite_stable_batch_create(kll_allocator *a, const fck_db_asset *asset, float sw, float sh)
{
	fck_sprite_stable_batch batch = {0};
	batch.sprite_width            = sw;
	batch.sprite_height           = sh;
	batch.base                    = fck_sprite_batch_create(a, asset);
	return batch;
}

static void fck_sprite_stable_batch_destroy(fck_sprite_stable_batch *batch)
{
	kll_allocator *allocator = batch->base.allocator;
	if (fck_sprite_batch_destroy(&batch->base))
	{
		kll_free(allocator, batch->dense);
		kll_free(allocator, batch->sparse);
		memset(batch, 0, sizeof(*batch));
	}
}

static fck_sprite_transform *fck_sprite_stable_batch_find(fck_sprite_stable_batch *batch, fckc_u32 at)
{
	if (at < batch->capacity)
	{
		fck_sprite_batch_index *sparse = batch->sparse + at;
		if (sparse->is_ok)
		{
			// It exists!
			return batch->base.transforms + sparse->value;
		}
	}
	return NULL;
}

static fck_sprite_transform *fck_sprite_stable_batch_resolve(fck_sprite_stable_batch *batch, fckc_u32 at)
{
	{
		fck_sprite_transform *transform = fck_sprite_stable_batch_find(batch, at);
		if (transform)
		{
			return transform;
		}
	}

	const fckc_u32 previous_base_capacity = batch->base.capacity;
	const fckc_u32 result                 = fck_sprite_batch_add(&batch->base);
	if (previous_base_capacity < batch->base.capacity)
	{
		const fckc_size_t             total = batch->base.capacity * sizeof(*batch->dense);
		fck_sprite_batch_dense_index *next  = (fck_sprite_batch_dense_index *)kll_malloc(batch->base.allocator, total);
		if (batch->dense)
		{
			memcpy(next, batch->dense, previous_base_capacity * sizeof(*batch->dense));
			kll_free(batch->base.allocator, batch->dense);
		}
		batch->dense = next;
	}

	if (batch->capacity <= at)
	{
		fckc_u32 next_capacity = at + 1;
		next_capacity--;
		next_capacity |= next_capacity >> 1;
		next_capacity |= next_capacity >> 2;
		next_capacity |= next_capacity >> 4;
		next_capacity |= next_capacity >> 8;
		next_capacity |= next_capacity >> 16;
		next_capacity++;

		const fckc_size_t       total = next_capacity * sizeof(*batch->sparse);
		fck_sprite_batch_index *next  = (fck_sprite_batch_index *)kll_malloc(batch->base.allocator, total);
		memset(next, 0, total);
		if (batch->sparse)
		{
			memcpy(next, batch->sparse, batch->capacity * sizeof(*batch->sparse));
			kll_free(batch->base.allocator, batch->sparse);
		}

		batch->sparse   = next;
		batch->capacity = next_capacity;
	}

	fck_sprite_batch_index *sparse = batch->sparse + at;
	sparse->value                  = result;
	sparse->is_ok                  = 1;

	fck_sprite_batch_dense_index *dense = batch->dense + result;
	dense->value                        = at;

	return batch->base.transforms + result;
}

static fckc_u32 fck_sprite_stable_batch_data(fck_sprite_stable_batch *batch, fck_sprite_transform **transforms)
{
	*transforms = batch->base.transforms;
	return batch->base.count;
}

static void fck_sprite_stable_batch_dimensions(fck_sprite_stable_batch *batch, float *width, float *height)
{
	*width  = batch->sprite_width;
	*height = batch->sprite_height;
}

static fckc_u32 fck_sprite_stable_batch_indexof(fck_sprite_stable_batch *batch, const fck_sprite_transform *transform)
{
	const fckc_u32 result = fck_sprite_batch_index_of(&batch->base, transform);
	if (!result)
	{
		return 0;
	}

	const fck_sprite_batch_dense_index *dense = batch->dense + result - 1;
	return dense->value + 1;
}

static fck_sprite_transform *fck_sprite_stable_batch_add(fck_sprite_stable_batch *batch)
{
	// This could become slow at some point! :-)
	for (fckc_u32 index = 0; index < batch->capacity; index++)
	{
		fck_sprite_batch_index *sparse = batch->sparse + index;
		if (!sparse->is_ok)
		{
			return fck_sprite_stable_batch_resolve(batch, index);
		}
	}

	return fck_sprite_stable_batch_resolve(batch, batch->capacity);
}

static int fck_sprite_stable_batch_remove(fck_sprite_stable_batch *batch, fckc_u32 index)
{
	if (index >= batch->capacity)
	{
		return 0;
	}

	fck_sprite_batch_index *sparse = batch->sparse + index;
	if (!sparse->is_ok)
	{
		return 0;
	}

	fck_sprite_batch_dense_index *dense = batch->dense + sparse->value;

	// Pop last!
	const fckc_u32 last = batch->base.count - 1;

	// Since base is the pivot for dense, it cuts off the range of batch->dense too!
	batch->base.transforms[sparse->value] = batch->base.transforms[last];

	fck_sprite_batch_dense_index *last_dense = batch->dense + last;
	*dense                                   = *last_dense;

	fck_sprite_batch_index *last_sparse = batch->sparse + last_dense->value;
	*last_sparse                        = *sparse;

	const int removal_result = fck_sprite_batch_remove(&batch->base, last);
	fck_assert(removal_result);
	(void)removal_result;

	sparse->is_ok = 0;
	return 1;
}

static fck_sprite_stable_batch *fck_sprites_get_batch(fck_sprites_internal *sprites, fckc_u32 index)
{
	if (index < sprites->count)
	{
		fck_sprite_stable_batch *batch = sprites->batches + index;
		return batch;
	}
	return NULL;
}

static fckc_u32 fck_sprite_batches(fck_sprites_internal *sprites, fck_sprite_stable_batch **batches)
{
	*batches = sprites->batches;
	return sprites->count;
}

static fckc_u32 fck_sprites_transforms(fck_sprites_internal *sprites, fckc_u32 batch_index, fck_sprite_transform **transforms)
{
	fck_sprite_stable_batch *batch = fck_sprites_get_batch(sprites, batch_index);
	if (batch)
	{
		return fck_sprite_stable_batch_data(batch, transforms);
	}
	return 0;
}

static fckc_u32 fck_sprites_names(fck_sprites_internal *sprites, const char ***names)
{
	*names = sprites->names;
	return sprites->count;
}

static const char *fck_sprites_nameof_batch(fck_sprites_internal *sprites, fckc_u32 index)
{
	if (index < sprites->count)
	{
		const char *batch_name = sprites->names[index];
		return batch_name;
	}
	return NULL;
}

static fckc_u32 fck_sprites_find_batch(fck_sprites_internal *sprites, const char *name)
{
	for (fckc_u32 index = 0; index < sprites->count; index++)
	{
		const char *batch_name = sprites->names[index];
		if (strcmp(name, batch_name) == 0)
		{
			return index + 1;
		}
	}
	return 0;
}

static fckc_u32 fck_sprites_register_batch(fck_sprites_internal *sprites, const char *name, const fck_db_asset *asset, float sw, float sh)
{
	{
		const fckc_u32 result = fck_sprites_find_batch(sprites, name);
		if (result)
		{
			fck_sprite_stable_batch *batch = sprites->batches + result - 1;
			if (asset == batch->base.asset)
			{
				// TODO: Float comparison, fix it :)
				if (batch->sprite_width == sw && batch->sprite_height == sh)
				{
					return result - 1;
				}
			}
		}
	}

	{
		if (sprites->count == sprites->capacity)
		{
			const fckc_u32    next_capacity   = sprites->capacity ? sprites->capacity * 2 : 4;
			const fckc_size_t total           = next_capacity * sizeof(*sprites->batches);
			const fckc_size_t total_name_size = next_capacity * sizeof(*sprites->names);

			fck_sprite_stable_batch *next  = (fck_sprite_stable_batch *)kll_malloc(sprites->allocator, total);
			const char             **names = (const char **)kll_malloc(sprites->allocator, total_name_size);
			if (sprites->batches)
			{
				memcpy(next, sprites->batches, sprites->count * sizeof(*sprites->batches));
				memcpy(names, sprites->names, sprites->count * sizeof(*sprites->names));
				kll_free(sprites->allocator, sprites->batches);
			}

			sprites->names    = names;
			sprites->batches  = next;
			sprites->capacity = next_capacity;
		}

		{
			fck_sprite_stable_batch *batch      = sprites->batches + sprites->count;
			const char             **batch_name = sprites->names + sprites->count;
			*batch_name                         = kll_format(sprites->strings, "%s", name);

			*batch         = fck_sprite_stable_batch_create(sprites->allocator, asset, sw, sh);
			sprites->count = sprites->count + 1;
			return sprites->count - 1;
		}
	}
}

static fck_sprites_internal fck_sprites_create(kll_allocator *allocator)
{
	const fck_sprites_internal sprites = {
		.allocator = allocator,
		.strings   = kll->arena->create(allocator, 256),
	};

	return sprites;
}

static void fck_sprites_destroy(fck_sprites_internal *sprites)
{
	if (sprites->names)
	{
		kll_free(sprites->allocator, sprites->names);
		for (fckc_u32 index = 0; index < sprites->count; index++)
		{
			fck_sprite_stable_batch_destroy(sprites->batches + index);
		}
		memset(sprites, 0, sizeof(*sprites));
	}
	kll->arena->destroy(sprites->strings);
}

// Yes
static fck_sprite_transform *fck_sprites_add(fck_sprites_internal *sprites, fckc_u32 index)
{
	fck_sprite_stable_batch *batch = sprites->batches + index;
	return fck_sprite_stable_batch_add(batch);
}

// No
static fck_sprite_transform *fck_sprites_add_by_name(fck_sprites_internal *sprites, const char *name)
{
	const fckc_u32 result = fck_sprites_find_batch(sprites, name);
	if (result)
	{
		return fck_sprites_add(sprites, result - 1);
	}
	return NULL;
}

static fckc_u32 fck_sprites_index_of(fck_sprites_internal *sprites, fckc_u32 batch_index, const fck_sprite_transform *transform)
{
	fck_sprite_stable_batch *batch = fck_sprites_get_batch(sprites, batch_index);
	if (batch)
	{
		return fck_sprite_stable_batch_indexof(batch, transform);
	}
	return 0;
}

static int fck_sprites_remove(fck_sprites_internal *sprites, fckc_u32 batch_index, fckc_u32 entry_index)
{
	fck_sprite_stable_batch *batch = sprites->batches + batch_index;

	return fck_sprite_stable_batch_remove(batch, entry_index);
}

static int fck_sprites_remove_by_name(fck_sprites_internal *sprites, fckc_u32 index, const char *name)
{
	const fckc_u32 result = fck_sprites_find_batch(sprites, name);
	if (result)
	{
		return fck_sprites_remove(sprites, result - 1, index);
	}
	return 0;
}

static fck_sprite_batch_id fck_sprite_batch_api_add(fck_sprites *external, const char *name, const fck_db_asset *asset, float sw, float sh)
{
	fck_sprites_internal sprites = {0};
	fck_sprites_to_internal(external, &sprites);

	fck_sprite_batch_id id;
	id.value = fck_sprites_register_batch(&sprites, name, asset, sw, sh);

	fck_sprites_to_external(&sprites, external);
	return id;
}

static int fck_sprite_batch_api_remove(fck_sprites *external, fck_sprite_batch_id index)
{
	(void)external;
	(void)index;
	fck_assert(0 && "Not supported to remove yet, I am too lazy to fix the stable indexing of batches!");
	return 0;
}

static fck_sprite_batch_id fck_sprite_batch_api_find_by_name(fck_sprites *external, const char *name)
{
	fck_sprites_internal sprites = {0};
	fck_sprites_to_internal(external, &sprites);
	const fckc_u32            result = fck_sprites_find_batch(&sprites, name);
	const fck_sprite_batch_id id     = {.value = result - 1};
	return id;
}

static fck_sprite_batch_id fck_sprite_batch_api_index(fck_sprites *external, fckc_u32 index)
{
	fck_sprites_internal sprites = {0};
	fck_sprites_to_internal(external, &sprites);

	const fck_sprite_stable_batch *batch = fck_sprites_get_batch(&sprites, index);
	if (batch)
	{
		const fck_sprite_batch_id id = {.value = index};
		return id;
	}
	{
		const fck_sprite_batch_id invalid = {.value = to_u32(~0LLU)};
		return invalid;
	}
}

static int fck_sprite_batch_api_is_ok(fck_sprites *external, fck_sprite_batch_id index)
{
	fck_sprites_internal sprites = {0};
	fck_sprites_to_internal(external, &sprites);

	const fck_sprite_stable_batch *batch = fck_sprites_get_batch(&sprites, index.value);
	if (batch)
	{
		return 1;
	}
	return 0;
}

static const struct fck_db_asset *fck_sprite_batch_api_asset(fck_sprites *external, fck_sprite_batch_id index)
{
	fck_sprites_internal sprites = {0};
	fck_sprites_to_internal(external, &sprites);

	fck_sprite_stable_batch *batch = fck_sprites_get_batch(&sprites, index.value);
	if (batch)
	{
		return batch->base.asset;
	}
	return NULL;
}

static struct sht_image_view *fck_sprite_batch_api_image_view(fck_sprites *external, fck_sprite_batch_id index)
{
	fck_sprites_internal sprites = {0};
	fck_sprites_to_internal(external, &sprites);

	fck_sprite_stable_batch *batch = fck_sprites_get_batch(&sprites, index.value);
	if (batch)
	{
		fck_texture_api *png = (fck_texture_api *)apis->find(fck_texture_api_name);
		return png->asset->gpu(batch->base.asset);
	}
	return NULL;
}

static int fck_sprite_batch_api_dimensions(fck_sprites *external, fck_sprite_batch_id index, float *sprite_width, float *sprite_height)
{
	fck_sprites_internal sprites = {0};
	fck_sprites_to_internal(external, &sprites);

	fck_sprite_stable_batch *batch = fck_sprites_get_batch(&sprites, index.value);
	if (batch)
	{
		*sprite_width  = batch->sprite_width;
		*sprite_height = batch->sprite_height;
		return 1;
	}
	return 0;
}

static const char *fck_sprite_batch_api_nameof(fck_sprites *external, fck_sprite_batch_id index)
{
	fck_sprites_internal sprites = {0};
	fck_sprites_to_internal(external, &sprites);
	return fck_sprites_nameof_batch(&sprites, index.value);
}

static fckc_u32 fck_sprite_batch_api_names(fck_sprites *external, const char ***out_names)
{
	fck_sprites_internal sprites = {0};
	fck_sprites_to_internal(external, &sprites);
	return fck_sprites_names(&sprites, out_names);
}

static fckc_u32 fck_sprite_batch_api_count(fck_sprites *external)
{
	fck_sprites_internal sprites = {0};
	fck_sprites_to_internal(external, &sprites);
	return sprites.count;
}

static void fck_sprite_api_destroy(struct fck_sprites *external)
{
	fck_sprites_internal sprites = {0};
	fck_sprites_to_internal(external, &sprites);

	fck_sprites_destroy(&sprites);

	fck_sprites_to_external(&sprites, external);
}

static fckc_u32 fck_sprite_api_transforms(struct fck_sprites *external, fck_sprite_batch_id index, fck_sprite_transform **out_transforms)
{
	fck_sprites_internal sprites = {0};
	fck_sprites_to_internal(external, &sprites);

	const fckc_u32 result = fck_sprites_transforms(&sprites, index.value, out_transforms);
	return result;
}

static fck_sprite_transform *fck_sprite_api_get(struct fck_sprites *external, fck_sprite_id index)
{
	fck_sprites_internal sprites = {0};
	fck_sprites_to_internal(external, &sprites);

	fck_sprite_stable_batch *batch = fck_sprites_get_batch(&sprites, index.batch.value);
	if (batch)
	{
		fck_sprite_transform *transform = fck_sprite_stable_batch_find(batch, index.entry.value);
		return transform;
	}
	return NULL;
}

static fck_sprite_transform *fck_sprite_api_set(struct fck_sprites *external, fck_sprite_id index)
{
	fck_sprites_internal sprites = {0};
	fck_sprites_to_internal(external, &sprites);

	fck_sprite_stable_batch *batch = fck_sprites_get_batch(&sprites, index.batch.value);
	if (batch)
	{
		fck_sprite_transform *transform = fck_sprite_stable_batch_resolve(batch, index.entry.value);
		fck_sprites_to_external(&sprites, external);
		return transform;
	}
	return NULL;
}

static fck_sprite_id fck_sprite_api_indexof(struct fck_sprites *external, fck_sprite_batch_id index, const fck_sprite_transform *transform)
{
	fck_sprites_internal sprites = {0};
	fck_sprites_to_internal(external, &sprites);
	fck_sprite_id id;
	id.batch       = index;
	// Flip to uintmax when invalid :-(
	id.entry.value = fck_sprites_index_of(&sprites, index.value, transform) - 1;
	return id;
}

static fck_sprite_transform *fck_sprite_api_add(struct fck_sprites *external, fck_sprite_batch_id index)
{
	fck_sprites_internal sprites = {0};
	fck_sprites_to_internal(external, &sprites);
	fck_sprite_transform *transform = fck_sprites_add(&sprites, index.value);
	if (transform)
	{
		fck_sprites_to_external(&sprites, external);
		return transform;
	}
	return NULL;
}

static int fck_sprite_api_remove(struct fck_sprites *external, fck_sprite_id index)
{
	fck_sprites_internal sprites = {0};
	fck_sprites_to_internal(external, &sprites);
	if (fck_sprites_remove(&sprites, index.batch.value, index.entry.value))
	{
		fck_sprites_to_external(&sprites, external);
		return 1;
	}
	return 0;
}

static int fck_sprite_api_is_ok(fck_sprites *external, fck_sprite_id index)
{
	fck_sprites_internal sprites = {0};
	fck_sprites_to_internal(external, &sprites);

	fck_sprite_stable_batch *batch = fck_sprites_get_batch(&sprites, index.batch.value);
	if (batch == NULL)
	{
		return 0;
	}
	if (fck_sprite_stable_batch_find(batch, index.entry.value))
	{
		return 1;
	}
	return 0;
}

static fck_sprite_id fck_sprite_api_invalid(void)
{
	const fck_sprite_id index = {
		.batch = {.value = to_u32(~0LLU)},
		.entry = {.value = to_u32(~0LLU)},
	};
	return index;
}

static struct fck_sprites fck_sprite_api_create(struct kll_allocator *allocator, const fck_sprite_create_args *args)
{
	fck_gfx_api *gfx = (fck_gfx_api *)apis->find(fck_gfx_api_name);

	fck_db_api *db = (fck_db_api *)apis->find(fck_db_api_name);

	db->asset->setup(*args->db, "sprite", fck_sprite_resource_path);
	const fck_db_asset *sprite_vs = db->asset->find(*args->db, "sprite/sprite.vert");
	const fck_db_asset *sprite_fs = db->asset->find(*args->db, "sprite/textured.frag");
	const fck_db_asset *debug_png = db->asset->find(*args->db, "sprite/debug.png");

	fck_sprites_internal sprites  = fck_sprites_create(allocator);
	fck_sprites          external = {0};
	sprites.driver                = args->driver;

	sht_memory *memory = args->driver->vt->memory(*args->driver);

	const fck_gfx_create_info create_info = {.flags = fck_gfx_target_all, .vertex = sprite_vs, .fragment = sprite_fs};
	sprites.gfx                           = gfx->create(kll->system, args->driver, &create_info);

	{
		fckc_u32 index_data[] = {0, 1, 2, 1, 3, 2};
		sprites.indices.count = fck_arraysize(index_data);
		sprites.indices.buffer =
			memory->malloc(memory->bump, &sht_buffer_target(sht_buffer_usage_index, sizeof(index_data)), sht_memory_gpu);
		args->driver->vt->upload_buffer(*args->driver, &sprites.indices.buffer, index_data, sizeof(index_data));
	}

	fck_sprites *external_sprites = fck_sprites_to_external(&sprites, &external);
	// TODO: Fix up a default thing again - Maybe not making it part of the batch? Reserve 0 to be NULL? Idk! :D
	// const fck_sprite_batch_id empty_batch = fck_sprite_batch_api_add(external_sprites, "Empty", &sprites.white_view, 32.0f, 32.0f);
	// fck_assert(empty_batch.value == 0);

	const fck_sprite_batch_id debug_batch = fck_sprite_batch_api_add(external_sprites, "null", debug_png, 8.0f, 8.0f);
	return *external_sprites;
}

typedef struct fck_sprite_screen
{
	float width;
	float height;
	float sprite_width;
	float sprite_height;
} fck_sprite_screen;

static int fck_sprite_api_present(void *userdata, const struct fck_gfx_args *args)
{
	fck_gfx_api     *gfx = (fck_gfx_api *)apis->find(fck_gfx_api_name);
	fck_texture_api *png = (fck_texture_api *)apis->find(fck_texture_api_name);

	fck_sprites         *external = (fck_sprites *)userdata;
	fck_sprites_internal sprites  = {0};
	fck_sprites_to_internal((fck_sprites *)external, &sprites);

	sht_driver *driver = sprites.driver;
	fck_assert(driver == args->driver);

	sht_command_buffer_vt *command     = driver->vt->command_buffer;
	const sht_render_pass  render_pass = command->render_pass->begin(*args->commands, args->suggestion);
	if (!command->render_pass->is_ok(render_pass))
	{
		return 0;
	}

	const sht_swapchain swapchain = driver->vt->swapchain(*driver);
	const sht_extent    extent    = swapchain.vt->extent(swapchain);
	sht_viewport        viewport;
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

	const fckc_size_t batch_count = fck_sprite_batch_api_count(external);
	for (fckc_size_t batch_index = 0; batch_index < batch_count; batch_index++)
	{
		const fck_sprite_batch_id id         = fck_sprite_batch_api_index(external, batch_index);
		fck_sprite_transform     *transforms = NULL;
		const fckc_u32            count      = fck_sprite_api_transforms(external, id, &transforms);

		if (count > 0)
		{
			float sprite_width  = 0;
			float sprite_height = 0;
			fck_sprite_batch_api_dimensions(external, id, &sprite_width, &sprite_height);
			command->index_buffer(*args->commands, &sprites.indices.buffer, 0);

			sht_bss               *bss      = gfx->bss(sprites.gfx);
			sht_graphics_pipeline *pipeline = gfx->pipeline(sprites.gfx);

			const fck_sprite_screen screen = {
				.width         = (float)extent.width,
				.height        = (float)extent.height,
				.sprite_width  = sprite_width,
				.sprite_height = sprite_height,
			};

			const sht_buffer_upload_desc screen_upload = {.data = &screen, .size = sizeof(screen), .count = 1};

			const sht_buffer_upload_desc transform_upload = {
				.data  = transforms,
				.size  = sizeof(*transforms),
				.count = count,
			};

			const sht_image_view       *view         = fck_sprite_batch_api_image_view(external, id);
			const sht_image_upload_desc image_upload = {.view = view};

			driver->vt->bss->upload_buffer(*bss, 0, &screen_upload);
			driver->vt->bss->upload_buffer(*bss, 1, &transform_upload);
			driver->vt->bss->upload_image(*bss, 3, &image_upload);
			command->bss(*args->commands, *bss);

			command->graphics_pipeline(*args->commands, *pipeline);

			const sht_draw_indexed_desc desc = {
				.first_index    = 0,
				.index_count    = to_u32(sprites.indices.count),
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

static fck_sprite_batch_api sprite_batch_api = {
	.add          = fck_sprite_batch_api_add,
	.dimensions   = fck_sprite_batch_api_dimensions,
	.image_view   = fck_sprite_batch_api_image_view,
	.asset        = fck_sprite_batch_api_asset,
	.nameof       = fck_sprite_batch_api_nameof,
	.count        = fck_sprite_batch_api_count,
	.names        = fck_sprite_batch_api_names,
	.remove       = fck_sprite_batch_api_remove,
	.find_by_name = fck_sprite_batch_api_find_by_name,
	.index        = fck_sprite_batch_api_index,
	.is_ok        = fck_sprite_batch_api_is_ok,
};

static fck_sprite_api sprite_api = {
	.batches    = &sprite_batch_api,
	.add        = fck_sprite_api_add,
	.create     = fck_sprite_api_create,
	.destroy    = fck_sprite_api_destroy,
	.indexof    = fck_sprite_api_indexof,
	.remove     = fck_sprite_api_remove,
	.get        = fck_sprite_api_get,
	.set        = fck_sprite_api_set,
	.transforms = fck_sprite_api_transforms,
	.is_ok      = fck_sprite_api_is_ok,
	.present    = fck_sprite_api_present,
	.invalid    = fck_sprite_api_invalid,
};

FCK_EXPORT_API fck_sprite_api *fck_sprite_load(fck_api_registry *registry, fck_sprite_api *old)
{
	(void)old;
	apis = registry;
	registry->add(fck_sprite_api_name, &sprite_api);
	return &sprite_api;
}
