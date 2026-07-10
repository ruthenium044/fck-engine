
#include "fck_ec.h"

#include <fckc_assert.h>
#include <fckc_inttypes.h>

#include <kll.h>
#include <kll_malloc.h>

#include <fck_hash.h>

#include <fck_apis.h>
#include <fckc_apidef.h>

#include <string.h>

typedef struct fck_entity_lookup
{
	kll_allocator *allocator;
	fck_entity *sparse;

	fckc_u32 capacity;
} fck_entity_lookup;

typedef struct fck_entity_storage
{
	fck_entity_lookup lookup;

	fck_entity *dense;

	fckc_u32 count;
	fckc_u32 capacity;
} fck_entity_storage;

typedef struct fck_entity_storage_removal
{
	fckc_u32 from;
	fckc_u32 to;
} fck_entity_storage_removal;

typedef struct fck_entity_components
{
	fck_entity_storage storage;
	struct fck_entity_components *next;

	const char *name;
	void *opaque;
	fckc_u32 size;
} fck_entity_components;

typedef struct fck_entity_buffer
{
	fck_entity *values;
	fckc_u32 capacity;
	fckc_u32 count;
} fck_entity_buffer;

typedef struct fck_ec_private
{
	fck_entity_storage all;
	fck_entity_buffer free_list;

	fck_entity_components *components;

	fck_entity_components *first;
	fck_entity_components *last;

	fckc_u32 count;
	fckc_u32 capacity;
} fck_ec_private;

static void fck_entity_buffer_destroy(kll_allocator *allocator, fck_entity_buffer *buffer)
{
	if (buffer->values)
	{
		kll_free(allocator, buffer);
		memset(buffer, 0, sizeof(*buffer));
	}
}

static void fck_entity_buffer_push(kll_allocator *allocator, fck_entity_buffer *buffer, const fck_entity *entities, fckc_u32 count)
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
		fck_entity *values = (fck_entity *)kll_malloc(allocator, total);
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

static int fck_entity_buffer_try_pop(fck_entity_buffer *buffer, fck_entity *entity)
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

static fck_entity_lookup fck_entity_lookup_api_create(kll_allocator *allocator)
{
	fck_entity_lookup entities = {0};
	entities.allocator = allocator;
	// entities.free_list = fck_entity_index_invalid;
	return entities;
}

static void fck_entity_lookup_api_destroy(fck_entity_lookup *lookup)
{
	if (lookup->sparse)
	{
		kll_free(lookup->allocator, lookup->sparse);
	}
	memset(lookup, 0, sizeof(*lookup));
}

static void fck_entity_lookup_ensure_capacity(fck_entity_lookup *entities, fckc_u32 target)
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
		fck_entity *next_values = (fck_entity *)kll_malloc(entities->allocator, entity_total);
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

static int fck_entity_lookup_api_alive(fck_entity_lookup *entities, fckc_u32 index)
{
	if (index >= entities->capacity)
	{
		return 0;
	}

	fck_entity *result = entities->sparse + index;
	if (result->alive == 0)
	{
		return 0;
	}
	return 1;
}

static int fck_entity_lookup_out_of_date(fck_entity_lookup *entities, fck_entity entity)
{
	if (entity.index >= entities->capacity)
	{
		return 0;
	}

	fck_entity *result = entities->sparse + entity.index;
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

static const fck_entity *fck_entity_lookup_api_add(fck_entity_lookup *entities, fck_entity entity)
{
	if (fck_entity_lookup_api_alive(entities, entity.index))
	{
		return NULL;
	}

	fck_entity_lookup_ensure_capacity(entities, entity.index + 1);

	// We have enough space!
	fck_entity *stored = entities->sparse + entity.index;
	stored->index = entity.index;
	stored->alive = 1;
	stored->generation = entity.generation;
	return stored;
}

static const fck_entity *fck_entity_lookup_api_set(fck_entity_lookup *entities, fck_entity entity, fckc_u32 value)
{
	if (!fck_entity_lookup_api_alive(entities, entity.index))
	{
		return NULL;
	}
	if (fck_entity_lookup_out_of_date(entities, entity))
	{
		return NULL;
	}

	fck_entity *result = entities->sparse + entity.index;
	result->index = value;
	return result;
}

static fck_entity *fck_entity_lookup_api_get(fck_entity_lookup *entities, fck_entity entity)
{
	if (!fck_entity_lookup_api_alive(entities, entity.index))
	{
		return NULL;
	}
	if (fck_entity_lookup_out_of_date(entities, entity))
	{
		return NULL;
	}

	fck_entity *result = entities->sparse + entity.index;
	return result;
}

static fckc_u32 fck_entity_lookup_api_remove(fck_entity_lookup *entities, fck_entity entity)
{
	if (!fck_entity_lookup_api_alive(entities, entity.index))
	{
		return 0;
	}
	if (fck_entity_lookup_out_of_date(entities, entity))
	{
		return 0;
	}

	fck_entity *result = entities->sparse + entity.index;
	const fckc_u32 target = result->index;
	result->alive = 0;
	return target + 1;
}

static fck_entity_storage fck_entity_storage_api_create(kll_allocator *allocator)
{
	fck_entity_storage storage = {0};
	storage.lookup = fck_entity_lookup_api_create(allocator);
	return storage;
}

static void fck_entity_storage_api_destroy(fck_entity_storage *storage)
{
	kll_allocator *allocator = storage->lookup.allocator;
	fck_entity_lookup_api_destroy(&storage->lookup);
	if (storage->dense)
	{
		kll_free(allocator, storage->dense);
	}
	memset(storage, 0, sizeof(*storage));
}

static fckc_u32 fck_entity_storage_api_set(fck_entity_storage *storage, fck_entity entity)
{
	if (fck_entity_lookup_api_alive(&storage->lookup, entity.index))
	{
		if (fck_entity_lookup_out_of_date(&storage->lookup, entity))
		{
			return 0;
		}

		fck_entity *slot = fck_entity_lookup_api_get(&storage->lookup, entity);
		fck_assert(slot);
		return slot->index + 1;
	}

	// Add new
	if (storage->count == storage->capacity)
	{
		// Ensure size
		const fckc_u32 next_capacity = storage->capacity ? storage->capacity * 2 : 8;
		const fckc_size_t total = next_capacity * sizeof(*storage->dense);
		fck_entity *next_values = (fck_entity *)kll_malloc(storage->lookup.allocator, total);
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
	const fck_entity *sparse = fck_entity_lookup_api_add(&storage->lookup, entity);
	if (sparse)
	{
		const fck_entity entity = *sparse;
		const fck_entity *result = fck_entity_lookup_api_set(&storage->lookup, entity, index);
		fck_assert(result == sparse && "add and set entity are different?");
		fck_assert(result->index == entity.index);
		storage->dense[index] = entity;
		storage->count = storage->count + 1;
		return index + 1;
	}
	return 0;
}

static fckc_u32 fck_entity_storage_api_get(fck_entity_storage *entities, fck_entity entity)
{
	const fck_entity *result = fck_entity_lookup_api_get(&entities->lookup, entity);
	if (result)
	{
		const fck_entity value = entities->dense[result->index];
		return value.index + 1;
	}
	return 0;
}

static int fck_entity_storage_api_remove(fck_entity_storage *entities, fck_entity entity, fck_entity_storage_removal *out_removal)
{
	const fckc_u32 result = fck_entity_lookup_api_remove(&entities->lookup, entity);
	if (!result)
	{
		// Just returning {0, 0} is not really cutting it, need to find a better approach!
		return 0;
	}
	const fckc_u32 current = result - 1;

	// Invariant of lookup should protect us
	fck_assert(entities->count != 0);

	const fckc_u32 last = entities->count - 1;
	fck_entity *last_dense = entities->dense + last;
	fck_entity_lookup_api_set(&entities->lookup, *last_dense, current);
	entities->dense[current] = *last_dense;

	const fck_entity invalid = {0};
	*last_dense = invalid;

	entities->count = entities->count - 1;

	if (out_removal)
	{
		out_removal->from = last;
		out_removal->to = current;
	}
	return 1;
}

static fck_entity_components fck_entity_component_api_create(kll_allocator *allocator, const char *name, fckc_u32 size)
{
	fck_entity_components components = {0};
	components.name = name;
	components.size = size;
	components.storage = fck_entity_storage_api_create(allocator);

	return components;
}

static void fck_entity_component_api_destroy(fck_entity_components *components)
{
	kll_allocator *allocator = components->storage.lookup.allocator;
	fck_entity_storage_api_destroy(&components->storage);
	if (components->opaque)
	{
		kll_free(allocator, components->opaque);
	}
	memset(components, 0, sizeof(*components));
}

static void *fck_entity_component_api_set(fck_entity_components *components, fck_entity entity, const void *data)
{
	const fckc_u32 count = components->storage.count;
	const fckc_u32 capacity = components->storage.capacity;
	const fckc_u32 result = fck_entity_storage_api_set(&components->storage, entity);

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

static void *fck_entity_component_api_get(fck_entity_components *components, fck_entity entity)
{
	const fckc_u32 result = fck_entity_storage_api_get(&components->storage, entity);
	if (result)
	{
		const fckc_size_t at = (result - 1) * components->size;
		fckc_u8 *memory = (fckc_u8 *)components->opaque;
		return (void *)(memory + at);
	}
	return NULL;
}

static fckc_u32 fck_entity_component_api_dense(fck_entity_components *components, const fck_entity **values)
{
	*values = components->storage.dense;
	return components->storage.count;
}

static void *fck_entity_component_api_buffer(fck_entity_components *components)
{
	return components->opaque;
}

static int fck_entity_component_api_remove(fck_entity_components *components, fck_entity entity)
{
	fck_entity_storage_removal removal;
	if (fck_entity_storage_api_remove(&components->storage, entity, &removal))
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

static fck_ec fck_ec_api_create(kll_allocator *allocator, fckc_u32 capacity)
{
	fck_ec_private *ec = {0};

	const fckc_size_t self_total = sizeof(*ec);
	const fckc_size_t component_total = sizeof(*ec->components) * capacity;
	const fckc_size_t total = self_total + component_total;
	void *memory = kll_malloc(allocator, total);
	memset(memory, 0, total);

	ec = (fck_ec_private *)memory;
	ec->components = (fck_entity_components *)fckc_pointer_add(memory, self_total);
	ec->all = fck_entity_storage_api_create(allocator);
	ec->capacity = capacity;
	fck_ec result = {.opaque = ec};
	return result;
}

static void fck_ec_api_destroy(fck_ec ec)
{
	fck_ec_private *ec_private = ec.opaque;
	kll_allocator *allocator = ec_private->all.lookup.allocator;
	for (fckc_u32 index = 0; index < ec_private->capacity; index++)
	{
		fck_entity_components *components = ec_private->components + index;
		fck_entity_component_api_destroy(components);
	}
	fck_entity_storage_api_destroy(&ec_private->all);

	fck_entity_buffer_destroy(allocator, &ec_private->free_list);
	kll_free(allocator, ec_private);
}

static fck_entity fck_ec_api_entity_create(fck_ec ec)
{
	fck_ec_private* ec_private = ec.opaque;
	fck_entity entity = {0};
	if (!fck_entity_buffer_try_pop(&ec_private->free_list, &entity))
	{
		entity.index = ec_private->all.count;
		entity.alive = 1;
	}

	entity.generation = entity.generation + 1;

	fck_entity_storage_api_set(&ec_private->all, entity);
	return entity;
}

// static void fck_ec_api_entity_archetype_print(fck_ec_private *ec_private, fck_entity entity)
//{
//	const fckc_u32 result = fck_entity_storage_api_get(&ec_private->all, entity);
//	if (result == 0)
//	{
//		os->io->log("Entity(index: %u - generation: %u) is dead");
//		return;
//	}
//
//	os->io->log("Entity(index: %u - generation: %u)", entity.index, entity.generation);
//	fck_entity_components *current = ec_private->first;
//	while (current)
//	{
//		void *data = fck_entity_component_api_get(current, entity);
//		if (data)
//		{
//			os->io->log("\tComponent: %s", current->name);
//		}
//		current = current->next;
//	}
//	os->io->log("================================");
// }

static int fck_ec_api_entity_destroy(fck_ec ec, fck_entity entity)
{
	fck_ec_private* ec_private = ec.opaque;
	const fckc_u32 result = fck_entity_storage_api_get(&ec_private->all, entity);
	if (result == 0)
	{
		return 0;
	}

	fck_entity_components *current = ec_private->first;
	while (current)
	{
		if (fck_entity_component_api_remove(current, entity))
		{
			// TODO:
		}
		current = current->next;
	}

	fck_entity_storage_api_remove(&ec_private->all, entity, NULL);
	fck_entity_buffer_push(ec_private->all.lookup.allocator, &ec_private->free_list, &entity, 1);
	return 1;
}

static int fck_ec_api_component_declare(fck_ec ec, const char *name, fckc_u32 size)
{
	fck_ec_private* ec_private = ec.opaque;
	fck_assert(ec_private->count < ec_private->capacity);

	const fck_hash_int hash = fck_hash(name, strlen(name));
	fckc_u32 slot = (fckc_u32)(hash % ec_private->capacity);

	for (fckc_u32 index = 0; index < ec_private->capacity; index++)
	{
		fck_entity_components *components = ec_private->components + slot;
		if (components->name == NULL)
		{
			*components = fck_entity_component_api_create(ec_private->all.lookup.allocator, name, size);

			if (ec_private->first == NULL)
			{
				ec_private->first = components;
				ec_private->last = components;
			}
			else
			{
				ec_private->last->next = components;
				ec_private->last = components;
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
		slot = (slot + 1) % ec_private->capacity;
	}
	//// Out of capacity!??
	return 0;
}

static fck_entity_components *fck_ec_api_get_components(fck_ec ec, const char *name)
{
	fck_ec_private* ec_private = ec.opaque;
	const fck_hash_int hash = fck_hash(name, strlen(name));
	fckc_u32 slot = (fckc_u32)(hash % ec_private->capacity);
	for (fckc_u32 index = 0; index < ec_private->capacity; index++)
	{
		fck_entity_components *components = ec_private->components + slot;
		if (components->name == NULL)
		{
			return NULL;
		}

		if (strcmp(components->name, name) == 0)
		{
			// Already added
			return components;
		}
		slot = (slot + 1) % ec_private->capacity;
	}
	return NULL;
}

// static void fck_ec_api_entity_component_info_print(fck_ec_private *ec, const char *name)
//{
//	fck_entity_components *components = fck_ec_api_get_components(ec, name);
//
//	if (components == NULL)
//	{
//		os->io->log("Component (name: %s) does not exist", name);
//		os->io->log("================================");
//		return;
//	}
//
//	os->io->log("Component (name: %s - size: %u)", components->name, components->size);
//	os->io->log("\tCount: %u", components->storage.count);
//	os->io->log("================================");
// }

static int fck_ec_api_component_set(fck_ec ecv, fck_entity entity, const char *name, const void *data)
{
	fck_ec_private* ec = ecv.opaque;
	const fckc_u32 result = fck_entity_storage_api_get(&ec->all, entity);
	if (result)
	{
		fck_entity_components *components = fck_ec_api_get_components(ecv, name);
		if (components)
		{
			fck_entity_component_api_set(components, entity, data);
			return 1;
		}
	}
	return 0;
}

static void *fck_ec_api_component_get(fck_ec ec, fck_entity entity, const char *name)
{
	fck_ec_private* ec_private = ec.opaque;
	const fckc_u32 result = fck_entity_storage_api_get(&ec_private->all, entity);
	if (result)
	{
		fck_entity_components *components = fck_ec_api_get_components(ec, name);
		if (components)
		{
			return fck_entity_component_api_get(components, entity);
		}
	}
	return NULL;
}

static int fck_ec_api_component_remove(fck_ec ec, fck_entity entity, const char *name)
{
	fck_ec_private* ec_private = ec.opaque;
	const fckc_u32 result = fck_entity_storage_api_get(&ec_private->all, entity);
	if (result)
	{
		fck_entity_components *components = fck_ec_api_get_components(ec, name);
		if (components)
		{
			fck_entity_component_api_remove(components, entity);
			return 1;
		}
	}
	return 0;
}

static void *fck_ec_api_get_component_buffer(fck_ec ec, const char *name)
{
	fck_entity_components *components = fck_ec_api_get_components(ec, name);
	if (components)
	{
		return fck_entity_component_api_buffer(components);
	}
	return NULL;
}

static fckc_u32 fck_ec_api_get_component_dense(fck_ec ec, const char *name, const fck_entity **values)
{
	fck_entity_components *components = fck_ec_api_get_components(ec, name);
	if (components)
	{
		return fck_entity_component_api_dense(components, values);
	}
	return 0;
}

static fck_ec_entity_api ec_entity_api = {
	.create = fck_ec_api_entity_create,
	.destroy = fck_ec_api_entity_destroy,
};
static fck_ec_component_api ec_component_api = {
	.declare = fck_ec_api_component_declare,
	.set = fck_ec_api_component_set,
	.remove = fck_ec_api_component_remove,
	.get = fck_ec_api_component_get,
	.dense = fck_ec_api_get_component_dense,
	.buffer = fck_ec_api_get_component_buffer,
};

static fck_ec_core_api ec_core_api = {
	.create = fck_ec_api_create,
	.destroy = fck_ec_api_destroy,
};

static fck_ec_api ec_api = {
	.entity = &ec_entity_api,
	.component = &ec_component_api,
	.core = &ec_core_api,
};

FCK_EXPORT_API fck_ec_api *fck_ec_load(fck_api_registry *registry, void *old)
{
	(void)old;
	registry->add(fck_ec_api_name, &ec_api);
	return &ec_api;
}