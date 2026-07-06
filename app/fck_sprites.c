
#include "fck_sprites.h"

#include <fckc_assert.h>
#include <fckc_inttypes.h>
#include <fckc_math.h>

#include <kll.h>
#include <kll_malloc.h>

#include <sht_render.h>

#include <string.h>

typedef struct fck_sprite_batch
{
	kll_allocator *allocator;

	sht_image_view image_view;

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

	fck_sprite_batch_index *sparse;
	fck_sprite_batch_dense_index *dense;

	fckc_u32 capacity;
	float sprite_width;
	float sprite_height;
} fck_sprite_stable_batch;

typedef struct fck_sprites_internal
{
	kll_allocator *allocator;
	const char **names;

	fck_sprite_stable_batch *batches;
	fckc_u32 count;
	fckc_u32 capacity;
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

static fck_sprite_batch fck_sprite_batch_create(kll_allocator *allocator, sht_image_view view)
{
	const fck_sprite_batch batch = {
		.allocator = allocator,
		.image_view = view,
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
	if (batch->count == batch->capacity)
	{
		const fckc_u32 initial = 8;
		const fckc_u32 next_capacity = batch->capacity ? batch->capacity * 2 : 8;
		fck_sprite_transform *next = (fck_sprite_transform *)kll_malloc(batch->allocator, next_capacity * sizeof(*batch->transforms));
		if (batch->transforms)
		{
			memcpy(next, batch->transforms, batch->capacity * sizeof(*batch->transforms));
			kll_free(batch->allocator, batch->transforms);
		}
		batch->capacity = next_capacity;
		batch->transforms = next;
	}

	const fckc_u32 at = batch->count;
	fck_sprite_transform *target = batch->transforms + at;
	memset(target, 0, sizeof(*target));
	batch->count = batch->count + 1;
	return at;
}

static fckc_u32 fck_sprite_batch_index_of(fck_sprite_batch *batch, const fck_sprite_transform *transform)
{
	const fckc_size_t root = to_size_t(batch->transforms);
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
		const fck_sprite_transform *last = batch->transforms + batch->count - 1;
		fck_sprite_transform *current = batch->transforms + index;
		*current = *last;
		batch->count = batch->count - 1;
		return 1;
	}
	return 0;
}

static fck_sprite_stable_batch fck_sprite_stable_batch_create(kll_allocator *a, const char *name, sht_image_view view, float sw, float sh)
{
	fck_sprite_stable_batch batch = {0};
	batch.sprite_width = sw;
	batch.sprite_height = sh;
	batch.base = fck_sprite_batch_create(a, view);
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
	const fckc_u32 result = fck_sprite_batch_add(&batch->base);
	if (previous_base_capacity != batch->base.capacity)
	{
		const fckc_size_t total = batch->base.capacity * sizeof(*batch->dense);
		fck_sprite_batch_dense_index *next = (fck_sprite_batch_dense_index *)kll_malloc(batch->base.allocator, total);
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

		const fckc_size_t total = next_capacity * sizeof(*batch->sparse);
		fck_sprite_batch_index *next = (fck_sprite_batch_index *)kll_malloc(batch->base.allocator, total);
		memset(next, 0, total);
		if (batch->sparse)
		{
			memcpy(next, batch->sparse, batch->capacity * sizeof(*batch->sparse));
			kll_free(batch->base.allocator, batch->sparse);
		}

		batch->sparse = next;
		batch->capacity = next_capacity;
	}

	fck_sprite_batch_index *sparse = batch->sparse + at;
	sparse->value = result;
	sparse->is_ok = 1;

	fck_sprite_batch_dense_index *dense = batch->dense + result;
	dense->value = at;

	return batch->base.transforms + result;
}

static fckc_u32 fck_sprite_stable_batch_data(fck_sprite_stable_batch *batch, fck_sprite_transform **transforms)
{
	*transforms = batch->base.transforms;
	return batch->base.count;
}

static void fck_sprite_stable_batch_dimensions(fck_sprite_stable_batch *batch, float *width, float *height)
{
	*width = batch->sprite_width;
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
	*dense = *last_dense;

	fck_sprite_batch_index *last_sparse = batch->sparse + last_dense->value;
	*last_sparse = *sparse;

	fck_assert(fck_sprite_batch_remove(&batch->base, last));

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

static fckc_u32 fck_sprites_batches(fck_sprites_internal *sprites, fck_sprite_stable_batch **batches)
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

static fckc_u32 fck_sprites_register_batch(fck_sprites_internal *sprites, const char *name, sht_image_view view, float sw, float sh)
{
	{
		const fckc_u32 result = fck_sprites_find_batch(sprites, name);
		if (result)
		{
			fck_sprite_stable_batch *batch = sprites->batches + result - 1;
			if (view.gpu == batch->base.image_view.gpu)
			{
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
			const fckc_u32 next_capacity = sprites->capacity ? sprites->capacity * 2 : 4;
			const fckc_size_t total = next_capacity * sizeof(*sprites->batches);
			const fckc_size_t total_name_size = next_capacity * sizeof(*sprites->names);

			fck_sprite_stable_batch *next = (fck_sprite_stable_batch *)kll_malloc(sprites->allocator, total);
			const char **names = (const char **)kll_malloc(sprites->allocator, total_name_size);
			if (sprites->batches)
			{
				memcpy(next, sprites->batches, sprites->count * sizeof(*sprites->batches));
				memcpy(names, sprites->names, sprites->count * sizeof(*sprites->names));
				kll_free(sprites->allocator, sprites->batches);
			}

			sprites->names = names;
			sprites->batches = next;
			sprites->capacity = next_capacity;
		}

		{
			fck_sprite_stable_batch *batch = sprites->batches + sprites->count;
			const char **batch_name = sprites->names + sprites->count;
			*batch_name = name;

			*batch = fck_sprite_stable_batch_create(sprites->allocator, name, view, sw, sh);
			sprites->count = sprites->count + 1;
			return sprites->count - 1;
		}
	}
}

static fck_sprites_internal fck_sprites_create(kll_allocator *allocator)
{
	const fck_sprites_internal sprites = {.allocator = allocator};
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

static fck_sprite_batch_id fck_sprites_batch_api_add(fck_sprites *external, const char *name, const sht_image_view *view, float sw,
                                                     float sh)
{
	fck_sprites_internal sprites = {0};
	fck_sprites_to_internal(external, &sprites);

	fck_sprite_batch_id id;
	id.value = fck_sprites_register_batch(&sprites, name, *view, sw, sh);

	fck_sprites_to_external(&sprites, external);
	return id;
}

static int fck_sprites_batch_api_remove(fck_sprites *external, fck_sprite_batch_id index)
{
	(void)external;
	(void)index;
	fck_assert(0 && "Not supported to remove yet, I am too lazy to fix the stable indexing of batches!");
	return 0;
}

static fck_sprite_batch_id fck_sprites_batch_api_index(fck_sprites *external, fckc_u32 index)
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

static int fck_sprites_batch_api_is_ok(fck_sprites *external, fck_sprite_batch_id index)
{
	fck_sprites_internal sprites = { 0 };
	fck_sprites_to_internal(external, &sprites);

	const fck_sprite_stable_batch* batch = fck_sprites_get_batch(&sprites, index.value);
	if(batch) {
		return 1;
	}
	return 0;
}

static struct sht_image_view *fck_sprites_batch_api_image_view(fck_sprites *external, fck_sprite_batch_id index)
{
	fck_sprites_internal sprites = {0};
	fck_sprites_to_internal(external, &sprites);

	fck_sprite_stable_batch *batch = fck_sprites_get_batch(&sprites, index.value);
	if (batch)
	{
		return &batch->base.image_view;
	}
	return NULL;
}

static int fck_sprites_batch_api_dimensions(fck_sprites *external, fck_sprite_batch_id index, float *sprite_width, float *sprite_height)
{
	fck_sprites_internal sprites = {0};
	fck_sprites_to_internal(external, &sprites);

	fck_sprite_stable_batch *batch = fck_sprites_get_batch(&sprites, index.value);
	if (batch)
	{
		*sprite_width = batch->sprite_width;
		*sprite_height = batch->sprite_height;
		return 1;
	}
	return 0;
}

static const char *fck_sprites_batch_api_nameof(fck_sprites *external, fck_sprite_batch_id index)
{
	fck_sprites_internal sprites = {0};
	fck_sprites_to_internal(external, &sprites);
	return fck_sprites_nameof_batch(&sprites, index.value);
}

static fckc_u32 fck_sprites_batch_api_names(fck_sprites *external, const char ***out_names)
{
	fck_sprites_internal sprites = {0};
	fck_sprites_to_internal(external, &sprites);
	return fck_sprites_names(&sprites, out_names);
}

static fckc_u32 fck_sprites_batch_api_count(fck_sprites *external)
{
	fck_sprites_internal sprites = {0};
	fck_sprites_to_internal(external, &sprites);
	return sprites.count;
}

static struct fck_sprites fck_sprites_api_create(struct kll_allocator *allocator)
{
	fck_sprites_internal sprites = fck_sprites_create(allocator);
	fck_sprites external = {0};
	return *fck_sprites_to_external(&sprites, &external);
}

static void fck_sprites_api_destroy(struct fck_sprites *external)
{
	fck_sprites_internal sprites = {0};
	fck_sprites_to_internal(external, &sprites);

	fck_sprites_destroy(&sprites);

	fck_sprites_to_external(&sprites, external);
}

static fckc_u32 fck_sprites_api_transforms(struct fck_sprites *external, fck_sprite_batch_id index, fck_sprite_transform **out_transforms)
{
	fck_sprites_internal sprites = {0};
	fck_sprites_to_internal(external, &sprites);

	const fckc_u32 result = fck_sprites_transforms(&sprites, index.value, out_transforms);
	return result;
}

static fck_sprite_transform *fck_sprites_api_get(struct fck_sprites *external, fck_sprite_id index)
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

static fck_sprite_transform *fck_sprites_api_set(struct fck_sprites *external, fck_sprite_id index)
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

static fck_sprite_transform *fck_sprites_api_add(struct fck_sprites *external, fck_sprite_batch_id index)
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

static int fck_sprites_api_remove(struct fck_sprites *external, fck_sprite_id index)
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

static fck_sprite_id fck_sprites_api_indexof(struct fck_sprites *external, fck_sprite_batch_id index, const fck_sprite_transform *transform)
{
	fck_sprites_internal sprites = {0};
	fck_sprites_to_internal(external, &sprites);
	fck_sprite_id id;
	id.batch = index;
	// Flip to uintmax when invalid :-(
	id.entry.value = fck_sprites_index_of(&sprites, index.value, transform) - 1;
	return id;
}

static int fck_sprites_api_is_ok(fck_sprites *external, fck_sprite_id index)
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

static fck_sprite_id fck_sprites_api_invalid(void)
{
	const fck_sprite_id index = {
		.batch = {.value = to_u32(~0LLU)},
		.entry = {.value = to_u32(~0LLU)},
	};
	return index;
}

static fck_sprite_batch_api sprites_batch_api = {
	.add = fck_sprites_batch_api_add,
	.dimensions = fck_sprites_batch_api_dimensions,
	.image_view = fck_sprites_batch_api_image_view,
	.nameof = fck_sprites_batch_api_nameof,
	.count = fck_sprites_batch_api_count,
	.names = fck_sprites_batch_api_names,
	.remove = fck_sprites_batch_api_remove,
	.index = fck_sprites_batch_api_index,
	.is_ok = fck_sprites_batch_api_is_ok,
};

static fck_sprite_api sprites_api = {
	.batches = &sprites_batch_api,
	.add = fck_sprites_api_add,
	.create = fck_sprites_api_create,
	.destroy = fck_sprites_api_destroy,
	.indexof = fck_sprites_api_indexof,
	.remove = fck_sprites_api_remove,
	.get = fck_sprites_api_get,
	.set = fck_sprites_api_set,
	.transforms = fck_sprites_api_transforms,
	.is_ok = fck_sprites_api_is_ok,
	.invalid = fck_sprites_api_invalid,
};

fck_sprite_api *sprites_ = &sprites_api;