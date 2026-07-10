
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

#include <fck_sprite.h>

// #pragma optimize("", off)

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
	float sprite_width;
	float sprite_height;
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

typedef struct fck_gameloop
{
	void *handle;
} fck_gameloop;

typedef struct fck_gameloop_interface
{
	fck_gameloop (*create)(void);
	void (*destroy)(fck_gameloop loop);

	int (*edit)(fck_gameloop);
	int (*tick)(fck_gameloop);
} fck_gameloop_interface;

typedef struct app_sprite_pie_items
{
	fck_nk_pie_item remove;

	fck_nk_pie_item duplicate;
	fck_nk_pie_item duplicate_left;
	fck_nk_pie_item duplicate_right;
	fck_nk_pie_item duplicate_up;
	fck_nk_pie_item duplicate_down;

	fck_nk_pie_item add;
	fck_nk_pie_item add_bird;
	fck_nk_pie_item add_item;
} app_sprite_pie_items;

static void app_sprite_pie_items_init(fck_nuklear_api *nk, app_sprite_pie_items *items, fck_nk_pie_item *root)
{
	memset(items, 0, sizeof(*items));

	const app_sprite_pie_items initial = {
		.remove.name = "Delete",
		.duplicate.name = "Duplicate",
		.duplicate_left.name = "<",
		.duplicate_right.name = ">",
		.duplicate_up.name = "^",
		.duplicate_down.name = "V",
		.add.name = "Add",
		.add_bird.name = "Bird",
		.add_item.name = "Item",
	};
	*items = initial;

	nk->pie->push(root, &items->duplicate);
	nk->pie->push(root, &items->remove);
	nk->pie->push(root, &items->add);

	nk->pie->push(&items->duplicate, &items->duplicate_left);
	nk->pie->push(&items->duplicate, &items->duplicate_right);
	nk->pie->push(&items->duplicate, &items->duplicate_up);
	nk->pie->push(&items->duplicate, &items->duplicate_down);

	nk->pie->push(&items->add, &items->add_bird);
	nk->pie->push(&items->add, &items->add_item);
}

static void fck_sprite_transform_property(fck_nuklear_api *nk, fck_nk view, fck_sprite_transform *transform)
{
	transform->x = nk->elements->f32(view, "x", -1280.0f, transform->x, 1280.0f, 1.0f);
	transform->y = nk->elements->f32(view, "y", -720.0f, transform->y, 720.0f, 1.0f);
	transform->z = nk->elements->f32(view, "z", 0.0f, transform->z, 1.0f, 0.1f);
	transform->rotation = nk->elements->f32(view, "rotation", 0.0f, transform->rotation, 360.0f, 1.0f);
	transform->scale = nk->elements->f32(view, "scale", 1.0f, transform->scale, 100.0f, 1.0f);
	transform->horizontal_index = nk->elements->i32(view, "horizontal index", 0, transform->horizontal_index, 10, 1);
	transform->vertical_index = nk->elements->i32(view, "vertical index", 0, transform->vertical_index, 10, 1);
}

static void fck_sprite_transform_editor(fck_plugins_api *plugins, fck_sprite_api *sprite, fck_nuklear_api *nk, fck_nk view,
                                        app_sprite_pie_items *pie, fck_sprites *sprites, fck_sprite_id *selected_sprite_id)
{
	nk->panel->begin(view, "Core Panel", 300.0f);
	{
		if (nk->panel->push(view, "Plugins"))
		{
			if (nk->panel->push(view, "Loaded Plugins"))
			{
				const char *current = NULL;
				while ((current = plugins->loaded(current)))
				{
					if (nk->elements->button(view, current))
					{
						plugins->unload(current);
						break;
					}
				}
				nk->panel->pop(view);
			}

			if (nk->panel->push(view, "Unloaded Plugins"))
			{
				const char *current = NULL;
				while ((current = plugins->unloaded(current)))
				{
					if (nk->elements->button(view, current))
					{
						plugins->load(current);
						break;
					}
				}
				nk->panel->pop(view);
			}
			nk->panel->pop(view);
		}

		if (nk->panel->push(view, "Sprite Transforms"))
		{
			fck_sprite_transform *selected_transform = sprite->get(sprites, *selected_sprite_id);
			if (selected_transform)
			{
				fck_sprite_transform *transform = selected_transform;
				if (nk->panel->push(view, "Selection"))
				{
					fck_sprite_transform_property(nk, view, transform);
					nk->panel->pop(view);
				}
			}

			const fckc_u32 batch_count = sprite->batches->count(sprites);
			for (fckc_u32 batch_index = 0; batch_index < batch_count; batch_index++)
			{
				const fck_sprite_batch_id id = sprite->batches->index(sprites, batch_index);
				fck_assert(sprite->batches->is_ok(sprites, id));

				fck_sprite_transform *transforms = NULL;
				const fckc_u32 count = sprite->transforms(sprites, id, &transforms);
				const char *batch_name = sprite->batches->nameof(sprites, id);
				if (nk->panel->push(view, batch_name, count))
				{
					for (fckc_u32 index = 0; index < count; index++)
					{
						if (nk->panel->push(view, "%s[%d]", batch_name, index))
						{
							const char **names;
							const fckc_u32 names_count = sprite->batches->names(sprites, &names);
							const int new_index = nk->elements->dropdown(view, (int)batch_index, names, names_count);
							if (new_index != batch_index)
							{
								const fck_sprite_transform copy = transforms[index];
								const fck_sprite_id sprite_id = sprite->indexof(sprites, id, transforms + index);
								if (sprite->is_ok(sprites, sprite_id))
								{
									if (sprite->remove(sprites, sprite_id))
									{
										const fck_sprite_batch_id new_id = sprite->batches->index(sprites, new_index);
										fck_assert(sprite->batches->is_ok(sprites, new_id));
										fck_sprite_transform *transform = sprite->add(sprites, new_id);
										*transform = copy;
										transform->horizontal_index = transform->vertical_index = 0;
									}
								}
							}
							else
							{
								fck_sprite_transform *transform = transforms + index;
								fck_sprite_transform_property(nk, view, transform);
							}
							nk->panel->pop(view);
						}
					}
					nk->panel->pop(view);
				}
			}
			nk->panel->pop(view);
		}
	}
	nk->panel->end(view);

	{
		*selected_sprite_id = sprite->invalid();
		const fck_nk_colour on = {0, 255, 0, 255};
		const fck_nk_colour off = {255, 0, 0, 255};

		const fckc_u32 batch_count = sprite->batches->count(sprites);
		for (fckc_u32 batch_index = 0; batch_index < batch_count; batch_index++)
		{
			const fck_sprite_batch_id id = {.value = batch_index};
			fck_sprite_transform *transforms = NULL;
			const fckc_u32 count = sprite->transforms(sprites, id, &transforms);

			float sprite_width = 0;
			float sprite_height = 0;
			sprite->batches->dimensions(sprites, id, &sprite_width, &sprite_height);

			for (fckc_size_t index = 0; index < count; index++)
			{
				fck_sprite_transform *transform = transforms + index;
				if (nk->select(view, transform, transform->x, transform->y, sprite_width * transform->scale,
				               sprite_height * transform->scale, on))
				{
					*selected_sprite_id = sprite->indexof(sprites, id, transforms + index);
				}
				if (nk->control_point(view, transform, &transform->x, &transform->y, 16.0f, on, off))
				{
					// break;
				}
			}
		}
	}

	{
		if (nk->pie->happened(&pie->add_bird))
		{
			const fck_sprite_batch_id id = sprite->batches->find_by_name(sprites, "Birds");
			fck_sprite_transform *transform = sprite->add(sprites, id);
			// Pie api is a bit clunky
			const fck_sprite_transform baseline = {
				.scale = 10.0f,
			};
			*transform = baseline;
			nk->pie->apply_position(view, &transform->x, &transform->y);
			nk->set_selection(view, transform);
		}

		if (nk->pie->happened(&pie->add_item))
		{
			const fck_sprite_batch_id id = sprite->batches->find_by_name(sprites, "Items");
			fck_sprite_transform *transform = sprite->add(sprites, id);
			// Pie api is a bit clunky
			const fck_sprite_transform baseline = {
				.scale = 10.0f,
			};
			*transform = baseline;
			nk->pie->apply_position(view, &transform->x, &transform->y);
			nk->set_selection(view, transform);
		}

		{
			fck_sprite_transform *selected_transform = sprite->get(sprites, *selected_sprite_id);
			if (nk->pie->happened(&pie->remove) && selected_transform)
			{
				sprite->remove(sprites, *selected_sprite_id);
				nk->set_selection(view, NULL);
			}

			if (nk->pie->happened(&pie->duplicate) && selected_transform)
			{
				const fck_sprite_transform copy = *selected_transform;
				fck_sprite_transform *transform = sprite->add(sprites, selected_sprite_id->batch);
				*transform = copy;
				*selected_sprite_id = sprite->indexof(sprites, selected_sprite_id->batch, transform);

				nk->pie->apply_position(view, &transform->x, &transform->y);
				nk->set_selection(view, transform);
			}
			if (nk->pie->happened(&pie->duplicate_left) && selected_transform)
			{
				const fck_sprite_transform copy = *selected_transform;
				fck_sprite_transform *transform = sprite->add(sprites, selected_sprite_id->batch);
				*transform = copy;
				*selected_sprite_id = sprite->indexof(sprites, selected_sprite_id->batch, transform);

				float sprite_width = 0;
				float sprite_height = 0;
				sprite->batches->dimensions(sprites, selected_sprite_id->batch, &sprite_width, &sprite_height);

				transform->x = transform->x - sprite_width;
				nk->set_selection(view, transform);
			}
			if (nk->pie->happened(&pie->duplicate_right) && selected_transform)
			{
				const fck_sprite_transform copy = *selected_transform;
				fck_sprite_transform *transform = sprite->add(sprites, selected_sprite_id->batch);
				*transform = copy;
				*selected_sprite_id = sprite->indexof(sprites, selected_sprite_id->batch, transform);

				float sprite_width = 0;
				float sprite_height = 0;
				sprite->batches->dimensions(sprites, selected_sprite_id->batch, &sprite_width, &sprite_height);

				transform->x = transform->x + sprite_width;
				nk->set_selection(view, transform);
			}
			if (nk->pie->happened(&pie->duplicate_up) && selected_transform)
			{
				const fck_sprite_transform copy = *selected_transform;
				fck_sprite_transform *transform = sprite->add(sprites, selected_sprite_id->batch);
				*transform = copy;
				*selected_sprite_id = sprite->indexof(sprites, selected_sprite_id->batch, transform);

				float sprite_width = 0;
				float sprite_height = 0;
				sprite->batches->dimensions(sprites, selected_sprite_id->batch, &sprite_width, &sprite_height);

				transform->y = transform->y - sprite_width;
				nk->set_selection(view, transform);
			}
			if (nk->pie->happened(&pie->duplicate_down) && selected_transform)
			{
				const fck_sprite_transform copy = *selected_transform;
				fck_sprite_transform *transform = sprite->add(sprites, selected_sprite_id->batch);
				*transform = copy;
				*selected_sprite_id = sprite->indexof(sprites, selected_sprite_id->batch, transform);

				float sprite_width = 0;
				float sprite_height = 0;
				sprite->batches->dimensions(sprites, selected_sprite_id->batch, &sprite_width, &sprite_height);

				transform->y = transform->y + sprite_width;
				nk->set_selection(view, transform);
			}
		}
	}
}

typedef enum app_entity_bits
{
	app_entity_alive_bit_count = 1,

	app_entity_index_bit_count = 23,
	app_entity_index_invalid = (1 << app_entity_index_bit_count) - 1,

	app_entity_generation_bit_count = 8,
	app_entity_generation_max = (1 << app_entity_generation_bit_count) - 1,
} app_entity_bits;

typedef struct app_entity
{
	fckc_u32 index : app_entity_index_bit_count;
	fckc_u32 alive : app_entity_alive_bit_count;
	fckc_u32 generation : app_entity_generation_bit_count;
} app_entity;

typedef struct app_entity_lookup
{
	kll_allocator *allocator;
	app_entity *sparse;

	fckc_u32 capacity;
} app_entity_lookup;

typedef struct app_entity_storage
{
	app_entity_lookup lookup;

	app_entity *dense;

	fckc_u32 count;
	fckc_u32 capacity;
} app_entity_storage;

typedef struct app_entity_storage_removal
{
	fckc_u32 from;
	fckc_u32 to;
} app_entity_storage_removal;

typedef struct app_entity_components
{
	app_entity_storage storage;
	struct app_entity_components *next;

	const char *name;
	void *opaque;
	fckc_u32 size;
} app_entity_components;

typedef struct app_entity_buffer
{
	app_entity *values;
	fckc_u32 capacity;
	fckc_u32 count;
} app_entity_buffer;

typedef struct app_ec
{
	app_entity_storage all;
	app_entity_buffer free_list;

	app_entity_components *components;

	app_entity_components *first;
	app_entity_components *last;

	fckc_u32 count;
	fckc_u32 capacity;
} app_ec;

static void app_entity_buffer_destroy(kll_allocator *allocator, app_entity_buffer *buffer)
{
	if (buffer->values)
	{
		kll_free(allocator, buffer);
		memset(buffer, 0, sizeof(*buffer));
	}
}

static void app_entity_buffer_push(kll_allocator *allocator, app_entity_buffer *buffer, const app_entity *entities, fckc_u32 count)
{
	if (buffer->count + count >= buffer->capacity)
	{
		fckc_u32 next_capacity;
		if (buffer->capacity == 0)
		{
			next_capacity = 8;
		}
		else
		{
			next_capacity = buffer->count + count + 1;
			next_capacity--;
			next_capacity |= next_capacity >> 1;
			next_capacity |= next_capacity >> 2;
			next_capacity |= next_capacity >> 4;
			next_capacity |= next_capacity >> 8;
			next_capacity |= next_capacity >> 16;
			next_capacity++;
		}
		const fckc_size_t total = next_capacity * sizeof(*buffer->values);
		app_entity *values = (app_entity *)kll_malloc(allocator, total);
		if (buffer->values)
		{
			const fckc_size_t prev_entity_total = buffer->count * sizeof(*buffer->values);
			memcpy(values, buffer->values, prev_entity_total);
			kll_free(allocator, buffer->values);
		}
		buffer->values = values;
		buffer->capacity = next_capacity;
	}

	memcpy(buffer->values + buffer->count, entities, count * sizeof(*buffer->values));
	buffer->count = buffer->count + count;
}

static int app_entity_buffer_try_pop(app_entity_buffer *buffer, app_entity *entity)
{
	if (buffer->count > 0)
	{
		const fckc_u32 last = buffer->count - 1;
		*entity = buffer->values[last];
		buffer->count = last;
		return 1;
	}
	return 0;
}

static app_entity_lookup app_entity_lookup_api_create(kll_allocator *allocator)
{
	app_entity_lookup entities = {0};
	entities.allocator = allocator;
	// entities.free_list = app_entity_index_invalid;
	return entities;
}

static void app_entity_lookup_api_destroy(app_entity_lookup *lookup)
{
	if (lookup->sparse)
	{
		kll_free(lookup->allocator, lookup->sparse);
	}
	memset(lookup, 0, sizeof(*lookup));
}

static void app_entity_lookup_ensure_capacity(app_entity_lookup *entities, fckc_u32 target)
{
	if (entities->capacity <= target)
	{
		fckc_u32 next_capacity;
		if (entities->capacity == 0)
		{
			next_capacity = 8;
		}
		else
		{
			next_capacity = target + 1;
			next_capacity--;
			next_capacity |= next_capacity >> 1;
			next_capacity |= next_capacity >> 2;
			next_capacity |= next_capacity >> 4;
			next_capacity |= next_capacity >> 8;
			next_capacity |= next_capacity >> 16;
			next_capacity++;
		}

		const fckc_size_t entity_total = next_capacity * sizeof(*entities->sparse);
		app_entity *next_values = (app_entity *)kll_malloc(entities->allocator, entity_total);
		memset(next_values, 0, entity_total);
		if (entities->sparse)
		{
			const fckc_size_t prev_entity_total = entities->capacity * sizeof(*entities->sparse);
			memcpy(next_values, entities->sparse, prev_entity_total);
			kll_free(entities->allocator, entities->sparse);
		}
		entities->sparse = next_values;
		entities->capacity = next_capacity;
	}
}

static int app_entity_lookup_api_alive(app_entity_lookup *entities, fckc_u32 index)
{
	if (index >= entities->capacity)
	{
		return 0;
	}

	app_entity *result = entities->sparse + index;
	if (result->alive == 0)
	{
		return 0;
	}
	return 1;
}

static int app_entity_lookup_out_of_date(app_entity_lookup *entities, app_entity entity)
{
	if (entity.index >= entities->capacity)
	{
		return 0;
	}

	app_entity *result = entities->sparse + entity.index;
	if (!result->alive)
	{
		return 0;
	}
	if (result->generation != entity.generation)
	{
		return 1;
	}
	return 0;
}

static const app_entity *app_entity_lookup_api_add(app_entity_lookup *entities, app_entity entity)
{
	if (app_entity_lookup_api_alive(entities, entity.index))
	{
		return NULL;
	}

	app_entity_lookup_ensure_capacity(entities, entity.index + 1);

	// We have enough space!
	app_entity *stored = entities->sparse + entity.index;
	stored->index = entity.index;
	stored->alive = 1;
	stored->generation = entity.generation;
	return stored;
}

static const app_entity *app_entity_lookup_api_set(app_entity_lookup *entities, app_entity entity, fckc_u32 value)
{
	if (!app_entity_lookup_api_alive(entities, entity.index))
	{
		return NULL;
	}
	if (app_entity_lookup_out_of_date(entities, entity))
	{
		return NULL;
	}

	app_entity *result = entities->sparse + entity.index;
	result->index = value;
	return result;
}

static app_entity *app_entity_lookup_api_get(app_entity_lookup *entities, app_entity entity)
{
	if (!app_entity_lookup_api_alive(entities, entity.index))
	{
		return NULL;
	}
	if (app_entity_lookup_out_of_date(entities, entity))
	{
		return NULL;
	}

	app_entity *result = entities->sparse + entity.index;
	return result;
}

static fckc_u32 app_entity_lookup_api_remove(app_entity_lookup *entities, app_entity entity)
{
	if (!app_entity_lookup_api_alive(entities, entity.index))
	{
		return 0;
	}
	if (app_entity_lookup_out_of_date(entities, entity))
	{
		return 0;
	}

	app_entity *result = entities->sparse + entity.index;
	const fckc_u32 target = result->index;
	result->alive = 0;
	return target + 1;
}

static app_entity_storage app_entity_storage_api_create(kll_allocator *allocator)
{
	app_entity_storage storage = {0};
	storage.lookup = app_entity_lookup_api_create(allocator);
	return storage;
}

static void app_entity_storage_api_destroy(app_entity_storage *storage)
{
	kll_allocator *allocator = storage->lookup.allocator;
	app_entity_lookup_api_destroy(&storage->lookup);
	if (storage->dense)
	{
		kll_free(allocator, storage->dense);
	}
	memset(storage, 0, sizeof(*storage));
}

static fckc_u32 app_entity_storage_api_set(app_entity_storage *storage, app_entity entity)
{
	if (app_entity_lookup_api_alive(&storage->lookup, entity.index))
	{
		if (app_entity_lookup_out_of_date(&storage->lookup, entity))
		{
			return 0;
		}

		app_entity *slot = app_entity_lookup_api_get(&storage->lookup, entity);
		fck_assert(slot);
		return slot->index + 1;
	}

	// Add new
	if (storage->count == storage->capacity)
	{
		// Ensure size
		const fckc_u32 next_capacity = storage->capacity ? storage->capacity * 2 : 8;
		const fckc_size_t total = next_capacity * sizeof(*storage->dense);
		app_entity *next_values = (app_entity *)kll_malloc(storage->lookup.allocator, total);
		if (storage->dense)
		{
			const fckc_size_t prev_total = storage->count * sizeof(*storage->dense);
			memcpy(next_values, storage->dense, prev_total);
			kll_free(storage->lookup.allocator, storage->dense);
		}
		storage->dense = next_values;
		storage->capacity = next_capacity;
	}

	const fckc_u32 index = storage->count;
	const app_entity *sparse = app_entity_lookup_api_add(&storage->lookup, entity);
	if (sparse)
	{
		const app_entity lookup_index = *sparse;
		const app_entity *result = app_entity_lookup_api_set(&storage->lookup, lookup_index, index);
		storage->dense[index] = lookup_index;
		storage->count = storage->count + 1;
		return index + 1;
	}
	return 0;
}

static fckc_u32 app_entity_storage_api_get(app_entity_storage *entities, app_entity entity)
{
	const app_entity *result = app_entity_lookup_api_get(&entities->lookup, entity);
	if (result)
	{
		const app_entity value = entities->dense[result->index];
		return value.index + 1;
	}
	return 0;
}

static int app_entity_storage_api_remove(app_entity_storage *entities, app_entity entity, app_entity_storage_removal *out_removal)
{
	const fckc_u32 result = app_entity_lookup_api_remove(&entities->lookup, entity);
	if (!result)
	{
		// Just returning {0, 0} is not really cutting it, need to find a better approach!
		return 0;
	}
	const fckc_u32 current = result - 1;

	// Invariant of lookup should protect us
	fck_assert(entities->count != 0);

	const fckc_u32 last = entities->count - 1;
	app_entity *last_dense = entities->dense + last;
	app_entity_lookup_api_set(&entities->lookup, *last_dense, current);
	entities->dense[current] = *last_dense;

	const app_entity invalid = {0};
	*last_dense = invalid;

	entities->count = entities->count - 1;

	if (out_removal)
	{
		out_removal->from = last;
		out_removal->to = current;
	}
	return 1;
}

static app_entity_components app_entity_components_api_create(kll_allocator *allocator, const char *name, fckc_u32 size)
{
	app_entity_components components = {0};
	components.name = name;
	components.size = size;
	components.storage = app_entity_storage_api_create(allocator);

	return components;
}

static void app_entity_components_api_destroy(app_entity_components *components)
{
	kll_allocator *allocator = components->storage.lookup.allocator;
	app_entity_storage_api_destroy(&components->storage);
	if (components->opaque)
	{
		kll_free(allocator, components->opaque);
	}
	memset(components, 0, sizeof(*components));
}

static void *app_entity_components_api_set(app_entity_components *components, app_entity entity, const void *data)
{
	const fckc_u32 count = components->storage.count;
	const fckc_u32 capacity = components->storage.capacity;
	const fckc_u32 result = app_entity_storage_api_set(&components->storage, entity);

	if (result == 0)
	{
		return NULL;
	}

	const fckc_u32 slot = result - 1;
	if (capacity != components->storage.capacity)
	{
		// Ensure size
		const fckc_size_t total = components->storage.capacity * components->size;
		void *next_values = kll_malloc(components->storage.lookup.allocator, total);
		if (components->opaque)
		{
			const fckc_size_t prev_total = count * components->size;
			memcpy(next_values, components->opaque, prev_total);
			kll_free(components->storage.lookup.allocator, components->opaque);
		}
		components->opaque = next_values;
	}

	const fckc_size_t offset = components->size * slot;
	fckc_u8 *memory = (fckc_u8 *)components->opaque;

	if (data != NULL)
	{
		memcpy(memory + offset, data, components->size);
	}
	else
	{
		memset(memory + offset, 0, components->size);
	}
	return (void *)(memory + offset);
}

static void *app_entity_components_api_get(app_entity_components *components, app_entity entity)
{
	const fckc_u32 result = app_entity_storage_api_get(&components->storage, entity);
	if (result)
	{
		const fckc_size_t at = (result - 1) * components->size;
		fckc_u8 *memory = (fckc_u8 *)components->opaque;
		return (void *)(memory + at);
	}
	return NULL;
}

static fckc_u32 app_entity_components_api_dense(app_entity_components *components, app_entity **values)
{
	*values = components->storage.dense;
	return components->storage.count;
}

static void *app_entity_components_api_buffer(app_entity_components *components)
{
	return components->opaque;
}

static int app_entity_components_api_remove(app_entity_components *components, app_entity entity)
{
	app_entity_storage_removal removal;
	if (app_entity_storage_api_remove(&components->storage, entity, &removal))
	{
		const fckc_size_t from = removal.from * components->size;
		const fckc_size_t to = removal.to * components->size;
		fckc_u8 *memory = (fckc_u8 *)components->opaque;
		memcpy(memory + to, memory + from, components->size);
		memset(memory + from, 0, components->size);
		return 1;
	}
	return 0;
}

static app_ec *app_ec_api_create(kll_allocator *allocator, fckc_u32 capacity)
{
	app_ec *ec = {0};

	const fckc_size_t self_total = sizeof(*ec);
	const fckc_size_t components_total = sizeof(*ec->components) * capacity;
	const fckc_size_t total = self_total + components_total;
	void *memory = kll_malloc(allocator, total);
	memset(memory, 0, total);

	ec = (app_ec *)memory;
	ec->components = (app_entity_components *)fckc_pointer_add(memory, self_total);
	ec->all = app_entity_storage_api_create(allocator);
	ec->capacity = capacity;
	return ec;
}

static void app_ec_api_destroy(app_ec *ecs)
{
	kll_allocator *allocator = ecs->all.lookup.allocator;
	for (fckc_u32 index = 0; index < ecs->capacity; index++)
	{
		app_entity_components *components = ecs->components + index;
		app_entity_components_api_destroy(components);
	}
	app_entity_storage_api_destroy(&ecs->all);

	app_entity_buffer_destroy(allocator, &ecs->free_list);
	kll_free(allocator, ecs);
}

static app_entity app_ec_api_entity_add(app_ec *ecs)
{
	app_entity entity = {0};
	if (!app_entity_buffer_try_pop(&ecs->free_list, &entity))
	{
		entity.index = ecs->all.count;
		entity.alive = 1;
	}

	entity.generation = entity.generation + 1;

	app_entity_storage_api_set(&ecs->all, entity);
	return entity;
}

static void app_ec_api_entity_archetype_print(app_ec *ecs, app_entity entity)
{
	const fckc_u32 result = app_entity_storage_api_get(&ecs->all, entity);
	if (result == 0)
	{
		os->io->log("Entity(index: %u - generation: %u) is dead");
		return;
	}

	os->io->log("Entity(index: %u - generation: %u)", entity.index, entity.generation);
	app_entity_components *current = ecs->first;
	while (current)
	{
		void *data = app_entity_components_api_get(current, entity);
		if (data)
		{
			os->io->log("\tComponent: %s", current->name);
		}
		current = current->next;
	}
	os->io->log("================================");
}

static void app_ec_api_entity_remove(app_ec *ecs, app_entity entity)
{
	const fckc_u32 result = app_entity_storage_api_get(&ecs->all, entity);
	if (result == 0)
	{
		return;
	}

	app_entity_components *current = ecs->first;
	while (current)
	{
		if (app_entity_components_api_remove(current, entity))
		{
			// TODO:
		}
		current = current->next;
	}

	app_entity_storage_api_remove(&ecs->all, entity, NULL);
	app_entity_buffer_push(ecs->all.lookup.allocator, &ecs->free_list, &entity, 1);
}

static int app_ec_api_register_component(app_ec *ecs, const char *name, fckc_u32 size)
{
	fck_assert(ecs->count < ecs->capacity);

	const fck_hash_int hash = fck_hash(name, strlen(name));
	fckc_u32 slot = (fckc_u32)(hash % ecs->capacity);

	for (fckc_u32 index = 0; index < ecs->capacity; index++)
	{
		app_entity_components *components = ecs->components + slot;
		if (components->name == NULL)
		{
			*components = app_entity_components_api_create(ecs->all.lookup.allocator, name, size);

			if (ecs->first == NULL)
			{
				ecs->first = components;
				ecs->last = components;
			}
			else
			{
				ecs->last->next = components;
				ecs->last = components;
			}

			return 1;
		}

		if (strcmp(components->name, name) == 0)
		{
			// Already added
			return 0;
		}
		const fck_hash_int other = fck_hash(components->name, strlen(components->name));
		fck_assert(hash != other);
		slot = (slot + 1) % ecs->capacity;
	}
	//// Out of capacity!??
	return 0;
}

static app_entity_components *app_ec_api_get_components(app_ec *ecs, const char *name)
{
	const fck_hash_int hash = fck_hash(name, strlen(name));
	fckc_u32 slot = (fckc_u32)(hash % ecs->capacity);
	for (fckc_u32 index = 0; index < ecs->capacity; index++)
	{
		app_entity_components *components = ecs->components + slot;
		if (components->name == NULL)
		{
			return NULL;
		}

		if (strcmp(components->name, name) == 0)
		{
			// Already added
			return components;
		}
		slot = (slot + 1) % ecs->capacity;
	}
	return NULL;
}

static void app_ec_api_entity_components_info_print(app_ec *ecs, const char *name)
{
	app_entity_components *components = app_ec_api_get_components(ecs, name);

	if (components == NULL)
	{
		os->io->log("Component (name: %s) does not exist", name);
		os->io->log("================================");
		return;
	}

	os->io->log("Component (name: %s - size: %u)", components->name, components->size);
	os->io->log("\tCount: %u", components->storage.count);
	os->io->log("================================");
}

static int app_ec_api_set_component(app_ec *ecs, app_entity entity, const char *name, const void *data)
{
	const fckc_u32 result = app_entity_storage_api_get(&ecs->all, entity);
	if (result)
	{
		app_entity_components *components = app_ec_api_get_components(ecs, name);
		if (components)
		{
			app_entity_components_api_set(components, entity, data);
			return 1;
		}
	}
	return 0;
}

static void *app_ec_api_get_component_buffer(app_ec *ecs, const char *name)
{
	app_entity_components *components = app_ec_api_get_components(ecs, name);
	return components->opaque;
}

typedef struct app_entity_lookup_api
{
	app_entity_lookup (*create)(kll_allocator *allocator);
	void (*destroy)(app_entity_lookup *lookup);
	const app_entity *(*add)(app_entity_lookup *entities, app_entity entity);
	const app_entity *(*set)(app_entity_lookup *entities, app_entity entity, fckc_u32 value);
	app_entity *(*get)(app_entity_lookup *entities, app_entity entity);
	fckc_u32 (*remove)(app_entity_lookup *entities, app_entity entity);
} app_entity_lookup_api;

typedef struct app_entity_storage_api
{
	app_entity_storage (*create)(kll_allocator *allocator);
	void (*destroy)(app_entity_storage *entities);
	// All return values are the actual value stored + 1
	// Need to do a - 1 on the result to get the result back! :)
	fckc_u32 (*get)(app_entity_storage *entities, app_entity entity);
	fckc_u32 (*set)(app_entity_storage *storage, app_entity entity);
	int (*remove)(app_entity_storage *entities, app_entity entity, app_entity_storage_removal *out_removal);
} app_entity_storage_api;

typedef struct app_entity_components_api
{
	app_entity_components (*create)(kll_allocator *allocator, const char *name, fckc_u32 size);
	void (*destroy)(app_entity_components *components);
	void *(*set)(app_entity_components *components, app_entity entity, const void *data);
	void *(*get)(app_entity_components *components, app_entity entity);
	int (*remove)(app_entity_components *components, app_entity entity);
	fckc_u32 (*dense)(app_entity_components *components, app_entity **values);
	void *(*buffer)(app_entity_components *components);
} app_entity_components_api;

static app_entity_lookup_api entity_lookup_api = {
	.create = app_entity_lookup_api_create,
	.destroy = app_entity_lookup_api_destroy,
	.add = app_entity_lookup_api_add,
	.set = app_entity_lookup_api_set,
	.get = app_entity_lookup_api_get,
	.remove = app_entity_lookup_api_remove,
};

static app_entity_storage_api entity_storage_api = {
	.create = app_entity_storage_api_create,
	.destroy = app_entity_storage_api_destroy,
	.set = app_entity_storage_api_set,
	.get = app_entity_storage_api_get,
	.remove = app_entity_storage_api_remove,
};

static app_entity_components_api entity_component_api = {
	.create = app_entity_components_api_create,
	.destroy = app_entity_components_api_destroy,
	.set = app_entity_components_api_set,
	.get = app_entity_components_api_get,
	.remove = app_entity_components_api_remove,
	.dense = app_entity_components_api_dense,
	.buffer = app_entity_components_api_buffer,
};

typedef struct app_ec_entities_api
{
	app_entity (*create)(app_ec *ec);
	void (*remove)(app_ec *ec, app_entity entity);
} app_ec_entities_api;

typedef struct app_ec_components_api
{
	// Find better name
	int (*reg)(app_ec *ec, const char *name, fckc_u32 size);
	app_entity (*add)(app_ec *ec, app_entity entity, const char *name, const void *data);
	void (*remove)(app_ec *ec, app_entity entity, const char *name);

	void *(*get)(app_ec *ec, app_entity entity, const char *name);
	void *(*query)(app_ec *ec, app_entity entity, const char *name, void *out_data);

	fckc_u32 (*dense)(app_ec *ec, const char *name, const app_entity **values);
	void *(*buffer)(app_ec *ec, const char *name);
} app_ec_components_api;

// Rethink this
#define app_ec_components_get_typed(api, type, ec, entity) (type *)api->components->get(ec, entity, #type)

typedef struct app_ec_api
{
	app_ec_entities_api *entities;
	app_ec_components_api *components;

	// Maybe moving this one into core or world would feel nicer
	// Maybe call world "database", that would be extra nice
	app_ec *(*create)(kll_allocator *allocator, fckc_u32 capacity);
	void (*destroy)(app_ec *ec);
} app_ec_api;

typedef struct app_entity_api
{
	app_entity_lookup_api *lookup;
	app_entity_storage_api *storage;
	app_entity_components_api *components;
} app_entity_api;

static app_entity_api entity_api = {
	.lookup = &entity_lookup_api,
	.storage = &entity_storage_api,
	.components = &entity_component_api,
};

static app_entity_api *entity = &entity_api;
static app_ec_api *ec;

typedef struct app_sprite_component
{
	fck_sprite_id id;
} app_sprite_component;

int main(int argc, char **argv)
{
	app_sprite_component initial = {.id = {.batch.value = 64, .entry.value = 128}};

	//// We can create a new ec
	//app_ec *other = ec->create(kll->system, 32);

	//// We can register a component
	//ec->components->reg(other, "sprite", sizeof(app_sprite_component));
	//// We can create an entity
	//const app_entity entity = ec->entities->create(other);

	//// We can add component to entity
	//ec->components->add(other, entity, "sprite", NULL);

	//const app_entity *sprites_entities;
	//// We can query a compact array of indices
	//const fckc_u32 sprites_count = ec->components->dense(other, "sprite", &sprites_entities);
	//// We can query a component buffer holding all the state in a compact manner
	//app_sprite_component *sprites_data = (app_sprite_component *)ec->components->buffer(other, "sprites");
	//for (fckc_u32 index = 0; index < sprites_count; index++)
	//{
	//	const app_entity e = sprites_entities[index];
	//	const app_sprite_component d = sprites_data[index];

	//	// This - Still requires null check
	//	float *x = app_ec_components_get_typed(ec, float, other, entity);

	//	// vs this - Still requires null check -- Can also be implicit
	//	float *y = (float *)ec->components->get(other, e, "float");
	//	// implicit:
	//	// float *y = ec->components->get(other, e, "float");
	//	// Makes the upper option redundant 
	//	// 
	//	// or maybe!
	//	float z;
	//	if (ec->components->query(other, e, "float", &z))
	//	{
	//	}

	//	os->io->log("Entity: {index: %u - generation: %u} uses Sprite {batch: %u - index: %u}", e.index, e.generation, d.id.batch.value,
	//	            d.id.entry.value);
	//}

	//// We can remove components
	//ec->components->remove(other, entity, "sprite");

	app_ec *world = app_ec_api_create(kll->system, 32);

	app_ec_api_register_component(world, "sprites", sizeof(app_sprite_component));
	app_ec_api_register_component(world, "test", sizeof(app_sprite_component));
	app_ec_api_register_component(world, "test case", sizeof(app_sprite_component));
	app_ec_api_register_component(world, "More", sizeof(app_sprite_component));

	app_entity e0 = app_ec_api_entity_add(world);
	app_entity e1 = app_ec_api_entity_add(world);
	app_entity e2 = app_ec_api_entity_add(world);
	app_entity e3 = app_ec_api_entity_add(world);

	app_ec_api_set_component(world, e0, "sprites", &initial);
	app_ec_api_set_component(world, e0, "test", &initial);
	app_ec_api_set_component(world, e0, "test case", &initial);
	app_ec_api_set_component(world, e0, "More", &initial);
	app_ec_api_set_component(world, e0, "Garbage", &initial);

	app_ec_api_entity_components_info_print(world, "sprites");
	app_ec_api_entity_components_info_print(world, "test");
	app_ec_api_entity_components_info_print(world, "test case");
	app_ec_api_entity_components_info_print(world, "More");

	app_ec_api_set_component(world, e1, "sprites", &initial);
	app_ec_api_set_component(world, e2, "sprites", &initial);
	app_ec_api_set_component(world, e3, "sprites", &initial);

	app_ec_api_entity_archetype_print(world, e0);
	app_ec_api_entity_archetype_print(world, e1);
	app_ec_api_entity_archetype_print(world, e2);
	app_ec_api_entity_archetype_print(world, e3);

	void *sprites_buffer = app_ec_api_get_component_buffer(world, "sprites");

	app_ec_api_entity_remove(world, e0);
	app_ec_api_entity_remove(world, e1);
	app_ec_api_entity_remove(world, e2);
	app_ec_api_entity_remove(world, e3);

	app_ec_api_entity_components_info_print(world, "sprites");
	app_ec_api_entity_components_info_print(world, "test");
	app_ec_api_entity_components_info_print(world, "test case");
	app_ec_api_entity_components_info_print(world, "More");

	app_entity e01 = app_ec_api_entity_add(world);
	app_entity e11 = app_ec_api_entity_add(world);
	app_entity e21 = app_ec_api_entity_add(world);
	app_entity e31 = app_ec_api_entity_add(world);

	// app_entity_lookup lookup = entity->lookup->create(kll->system);
	// app_entity_storage strorage = entity->storage->create(kll->system);
	// app_entity_components components = entity->components->create(kll->system, "sprites", sizeof(app_sprite_component));

	// for (fckc_u32 index = 0; index < 9; index++)
	//{
	//	if (index % 2 == 0)
	//	{
	//		const app_entity handle = {.index = index};
	//		entity->lookup->add(&lookup, handle);
	//		entity->storage->set(&strorage, handle);
	//		entity->components->set(&components, handle, &initial);
	//	}
	// }

	// for (fckc_u32 index = 0; index < 9; index++)
	//{
	//	if (index % 2 == 0)
	//	{
	//		const app_entity handle = {.index = index};
	//		entity->lookup->remove(&lookup, handle);
	//		entity->storage->remove(&strorage, handle);
	//		entity->components->remove(&components, handle);
	//	}
	// }

	// for (fckc_u32 index = 0; index < 9; index++)
	//{
	//	if (index % 2 == 0)
	//	{
	//		const app_entity handle = {.index = index};
	//		entity->lookup->set(&lookup, handle, index);
	//		entity->storage->set(&strorage, handle);
	//		entity->components->set(&components, handle, &initial);
	//	}
	// }

	// for (fckc_u32 index = 0; index < 9; index++)
	//{
	//	if (index % 2 == 0)
	//	{
	//		const app_entity handle = {.index = index};
	//		entity->lookup->add(&lookup, handle);
	//		entity->storage->set(&strorage, handle);
	//		entity->components->set(&components, handle, &initial);
	//	}
	// }
	// for (fckc_u32 index = 0; index < 9; index++)
	//{
	//	if (index % 2 == 0)
	//	{
	//		const app_entity handle = {
	//			.index = index,
	//			.generation = 1,
	//		};
	//		entity->lookup->add(&lookup, handle);
	//		entity->storage->set(&strorage, handle);
	//		entity->components->set(&components, handle, &initial);
	//	}
	// }

	// app_entity_components entities = entity->components->create(kll->system, "sprite-component", sizeof(app_sprite_component));
	// entity->components->add(&entities, &initial);
	// entity->components->add(&entities, &initial);
	// entity->components->add(&entities, &initial);
	// entity->components->add(&entities, &initial);
	// entity->components->add(&entities, &initial);
	// entity->components->add(&entities, &initial);
	// entity->components->add(&entities, &initial);
	// entity->components->add(&entities, &initial);
	// entity->components->add(&entities, &initial);

	// for (fckc_u32 index = 0; index < 9; index++)
	//{
	//	if (index % 2 == 0)
	//	{
	//		const app_entity handle = {.index = index};
	//		entity->components->remove(&entities, handle);
	//	}
	// }

	//{
	//	app_entity *entries = NULL;
	//	const fckc_u32 count = entity->components->dense(&entities, &entries);
	//	const fck_sprite_id *sprites = (const fck_sprite_id *)entity->components->buffer(&entities);
	//	for (fckc_u32 index = 0; index < count; index++)
	//	{
	//		const app_entity e = entries[index];
	//		const fck_sprite_id id = sprites[index];
	//		os->io->log("Entity: %u -> Sprite: {batch: %u, entry: %u}", e.index, id.batch.value, id.entry.value);
	//	}
	//}

	// entity->components->add(&entities, &initial);
	// entity->components->add(&entities, &initial);
	// entity->components->add(&entities, &initial);
	// entity->components->add(&entities, &initial);
	// entity->components->add(&entities, &initial);
	// entity->components->add(&entities, &initial);
	// entity->components->add(&entities, &initial);
	// entity->components->add(&entities, &initial);
	// entity->components->add(&entities, &initial);

	// TODO: We need to setup stable editor entities, or something like that
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
	fck_sprite_api *sprite = (fck_sprite_api *)registry->find(fck_sprite_api_name);

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

	nk->hamburger->push(view, &help_menu_item);
	nk->hamburger->push(view, &about_menu_item);
	nk->hamburger->push(view, &setting_menu_item);

	fck_nk_pie_item *root = nk->pie->root(view);
	app_sprite_pie_items sprite_pie;
	app_sprite_pie_items_init(nk, &sprite_pie, root);

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

	const fck_png background_png = png->load(fck_resource_path "bg-mockup.png");
	const fck_png bird_png = png->load(fck_resource_path "bird-sheet.png");
	const fck_png items_png = png->load(fck_resource_path "items-sheet.png");

	const sht_image background_image =
		app_load_image(driver, background_png.data, sht_format_r8g8b8a8_unorm, background_png.width, background_png.height);
	const sht_image_view background_image_view = memory->image->view(memory->bump, background_image, sht_format_r8g8b8a8_unorm);

	const sht_image bird_image = app_load_image(driver, bird_png.data, sht_format_r8g8b8a8_unorm, bird_png.width, bird_png.height);
	const sht_image_view bird_image_view = memory->image->view(memory->bump, bird_image, sht_format_r8g8b8a8_unorm);

	const sht_image items_image = app_load_image(driver, items_png.data, sht_format_r8g8b8a8_unorm, items_png.width, items_png.height);
	const sht_image_view items_image_view = memory->image->view(memory->bump, items_image, sht_format_r8g8b8a8_unorm);

	const fck_gfx_shader vertex_shader = {.name = "vertex", .path = fck_resource_path "sprite.vert"};
	const fck_gfx_shader fragment_shader = {.name = "textured", .path = fck_resource_path "textured.frag"};
	const fck_gfx_create_info create_info = {.has_depth = 1, .vertex = &vertex_shader, .fragment = &fragment_shader};
	const fck_gfx sprite_gfx = gfx->create(kll->system, &driver, &create_info);

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

	fck_sprites sprites = sprite->create(kll->system);
	const fck_sprite_batch_id background_batch = sprite->batches->add(&sprites, "Background", &background_image_view, 132.0f, 72.0f);
	const fck_sprite_batch_id birds_batch = sprite->batches->add(&sprites, "Birds", &bird_image_view, 32.0f, 32.0f);
	const fck_sprite_batch_id items_batch = sprite->batches->add(&sprites, "Items", &items_image_view, 16.0f, 16.0f);

	const float temp_transform_scale = 10.0f;

	{
		fck_sprite_transform *transform = sprite->add(&sprites, background_batch);
		const fck_sprite_transform baseline = {
			.scale = temp_transform_scale,
			.x = 0.0f,
			.y = 0.0f,
			.z = 0.0f,
		};
		*transform = baseline;
	}

	{
		fck_sprite_transform *transform = sprite->add(&sprites, birds_batch);
		const fck_sprite_transform baseline = {
			.scale = temp_transform_scale,
			.x = -20.0f * temp_transform_scale,
			.y = 15.0f * temp_transform_scale,
		};
		*transform = baseline;
	}

	{
		fck_sprite_transform *transform = sprite->add(&sprites, items_batch);
		const fck_sprite_transform baseline = {
			.scale = temp_transform_scale,
			.x = 20.0f * temp_transform_scale,
			.y = 15.0f * temp_transform_scale,
		};
		*transform = baseline;
	}

	fckc_u64 time_point = os->chrono->ms();

	fckc_u64 accumulator = 0;

	// fckc_u32 selected_batch_index = 0;
	// fckc_u32 selected_transform_index = 0;
	fck_sprite_id selected_sprite_id = {0, 0};
	int is_running = 1;
	while (is_running)
	{
		const fck_nk_control control = nk->control(view);
		if (control.close)
		{
			is_running = 0;
		}

		if (control.minimise)
		{
			os->win->minimise(window);
		}

		const fckc_u64 now = os->chrono->ms();
		const fckc_u64 delta = now - time_point;
		time_point = now;

		accumulator = accumulator + delta;
		if (accumulator >= 160)
		{
			accumulator = accumulator - 160;
			fck_sprite_transform *bird_transforms;
			const fckc_u32 bird_count = sprite->transforms(&sprites, birds_batch, &bird_transforms);
			for (fckc_size_t index = 0; index < bird_count; index++)
			{
				bird_transforms[index].horizontal_index = (bird_transforms[index].horizontal_index + 1) % 4;
			}
		}
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
				fck_sprite_transform_editor(plugins, sprite, nk, view, &sprite_pie, &sprites, &selected_sprite_id);
			}
			nk->end(view);
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
				// Keeping them more contained?
				sht_viewport viewport;
				viewport.offset.x = 0.0f;
				viewport.offset.y = 0.0f;
				viewport.depth.min = (float)0.0f;
				viewport.depth.max = (float)1.0f;

				sht_scissor scissor;
				scissor.offset.x = 0;
				scissor.offset.y = 0;
				scissor.extent = viewport.extent = swapchain.vt->extent(swapchain);

				{
					sht_render_desc desc = {
						.colour = {.view = color_target,
					               .load_op = sht_clear,
					               .store_op = sht_store,
					               .clear_value = {0.2f, 0.0f, 0.2f, 1.0f}},
						.depth = {.view = depth_view, .load_op = sht_clear, .store_op = sht_dont_care},
					};

					const sht_render_pass render_pass = command->render_pass->begin(command_buffer, &desc);
					if (command->render_pass->is_ok(render_pass))
					{
						command->viewport(command_buffer, &viewport);
						command->scissor(command_buffer, &scissor);

						const fckc_size_t batch_count = sprite->batches->count(&sprites);
						for (fckc_size_t batch_index = 0; batch_index < batch_count; batch_index++)
						{
							const fck_sprite_batch_id id = sprite->batches->index(&sprites, batch_index);
							fck_sprite_transform *transforms = NULL;
							const fckc_u32 count = sprite->transforms(&sprites, id, &transforms);

							if (count > 0)
							{
								float sprite_width = 0;
								float sprite_height = 0;
								sprite->batches->dimensions(&sprites, id, &sprite_width, &sprite_height);
								command->index_buffer(command_buffer, &indices.buffer, 0);

								sht_bss *bss = gfx->bss(sprite_gfx);
								sht_graphics_pipeline *pipeline = gfx->pipeline(sprite_gfx); //

								const app_screen screen = {
									.width = (float)extent.width,
									.height = (float)extent.height,
									.sprite_width = sprite_width,
									.sprite_height = sprite_height,
								};

								const sht_buffer_upload_desc screen_upload = {.data = &screen, .size = sizeof(screen), .count = 1};

								const sht_buffer_upload_desc transform_upload = {
									.data = transforms,
									.size = sizeof(*transforms),
									.count = count,
								};

								const sht_image_view *view = sprite->batches->image_view(&sprites, id);
								const sht_image_upload_desc image_upload = {.samplers = sampler, .views = *view};

								driver.vt->bss->upload_buffer(*bss, 0, &screen_upload);
								driver.vt->bss->upload_buffer(*bss, 1, &transform_upload);
								driver.vt->bss->upload_image(*bss, 3, &image_upload);
								command->bss(command_buffer, *bss);

								command->graphics_pipeline(command_buffer, *pipeline);

								const sht_draw_indexed_desc desc = {
									.first_index = 0,
									.index_count = to_u32(indices.count),
									.instance_count = count,
									.first_instance = 0,
									.vertex_offset = 0,
								};

								command->draw_indexed(command_buffer, &desc);
							}
						}

						command->render_pass->end(command_buffer);
					}
				}

				{
					// Could this render pass live in nk->present directly?
					sht_render_desc desc = {.colour = {.view = color_target, .load_op = sht_load, .store_op = sht_store}};
					const sht_render_pass render_pass = command->render_pass->begin(command_buffer, &desc);
					if (command->render_pass->is_ok(render_pass))
					{
						command->viewport(command_buffer, &viewport);
						command->scissor(command_buffer, &scissor);
						nk->present(view, &command_buffer, frame_index);
						command->render_pass->end(command_buffer);
					}
				}

				command->submit(command_buffer, sht_queue_graphic);
			}
		}
		else
		{
			if (frame_index == sht_swapchain_needs_resize)
			{
				// NOTE: Maybe only resize if we get larger and never shrink?
				const sht_extent extent = swapchain.vt->extent(swapchain);
				memory->image->recreate(memory->bump, &depth_image, extent, &depth_view, 1);
			}
		}
	}

	plugins->shutdown();
	purge_files("temp-*.dll");

	os->win->destroy(window);

	return 0;
}
