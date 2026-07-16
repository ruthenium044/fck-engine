
#include "fck_ec.h"

#include <fckc_assert.h>
#include <fckc_inttypes.h>

#include <kll.h>
#include <kll_malloc.h>

#include <fck_hash.h>

#include <fck_apis.h>
#include <fckc_apidef.h>

#include <fck_os.h>

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

	void *opaque;
	const char *name;

	fck_component_definition definition;
	fckc_u32 size;
} fck_entity_components;

typedef struct fck_entity_buffer
{
	fck_entity *values;
	fckc_u32 capacity;
	fckc_u32 count;
} fck_entity_buffer;

typedef struct fck_query
{
	fck_hash_int hash;
	const char *name;
	fck_query_component *components;
	fckc_u32 size;
	fckc_u32 count;
} fck_query;

struct fck_query_chunk;
typedef struct fck_query_chunk
{
	fckc_u32 index;
	struct fck_query_chunk *next;

	fckc_u32 capacity;
	fckc_u32 count;
	fck_query values[1];
} fck_query_chunk;

typedef struct fck_queries
{
	// kll_allocator *allocator;
	kll_arena *arena;

	fck_query_chunk *last;
	fck_query_chunk *first;
} fck_queries;

typedef struct fck_ec_private
{
	fck_entity_storage all;
	fck_entity_buffer free_list;

	fck_queries queries;
	fck_entity_components *components;

	fck_entity_components *first;
	fck_entity_components *last;

	fckc_u32 count;
	fckc_u32 capacity;
} fck_ec_private;

static fck_hash_int fck_components_hash(const fck_query_component *components, fckc_u32 count)
{
	fck_hash_int hash = 0;
	for (fckc_u32 index = 0; index < count; index++)
	{
		const fck_query_component *component = components + index;
		hash = fck_hash_combine(hash, component->id.value);
	}
	return hash;
}

static int fck_components_same(const fck_query_component *lhs, fckc_u32 lhs_count, const fck_query_component *rhs, fckc_u32 rhs_count)
{
	if (lhs_count != rhs_count)
	{
		return 0;
	}

	for (fckc_u32 index = 0; index < lhs_count; index++)
	{
		const fck_query_component *l = lhs + index;
		const fck_query_component *r = rhs + index;
		if (l->id.value != r->id.value)
		{
			return 0;
		}
	}
	return 1;
}

static fck_queries fck_queries_create(kll_allocator *allocator)
{
	fck_queries queries = {0};
	queries.arena = kll->arena->create(allocator, 2048);
	return queries;
}

static void fck_queries_destroy(fck_queries *queries)
{
	kll->arena->destroy(queries->arena);
	memset(queries, 0, sizeof(*queries));
}

static fck_query *fck_queries_resolve(fck_query_id id)
{
	return (fck_query *)id.opaque;
}

static fck_query_id fck_queries_get(fck_queries *queries, const char *name, fckc_u32 size, const fck_query_component *components,
                                    fckc_u32 count)
{
	const fck_hash_int hash = fck_components_hash(components, count);

	{
		fck_query_chunk *current = queries->first;
		while (current)
		{
			fckc_u32 slot = hash % current->capacity;
			for (;;)
			{
				fck_query *query = current->values + slot;
				if (query->components == NULL)
				{
					break;
				}
				if (query->hash == hash)
				{
					if (fck_components_same(components, count, query->components, query->count))
					{
						const fck_query_id id = {.opaque = query};
						return id;
					}
				}
				slot = (slot + 1) % current->capacity;
			}
			current = current->next;
		}
	}

	if (queries->last == NULL)
	{
		const fckc_u32 capacity = 32;
		const fckc_size_t total = offsetof(fck_query_chunk, values[capacity]);
		fck_query_chunk *last = (fck_query_chunk *)kll_malloc(queries->arena, total);
		memset(last, 0, total);
		last->capacity = capacity;
		last->count = 0;
		last->index = 0;
		last->next = NULL;
		queries->last = last;
		queries->first = last;
	}
	else if (queries->last->count >= (queries->last->capacity / 2))
	{
		const fckc_u32 capacity = queries->last->capacity * 2;
		const fckc_size_t total = offsetof(fck_query_chunk, values[capacity]);
		fck_query_chunk *last = (fck_query_chunk *)kll_malloc(queries->arena, total);
		memset(last, 0, total);
		last->capacity = capacity;
		last->count = 0;
		last->index = 0;
		last->next = NULL;
		queries->last->next = last;
		queries->last = last;
	}

	fckc_u32 slot = hash % queries->last->capacity;
	for (;;)
	{
		fck_query *query = queries->last->values + slot;
		if (query->components == NULL)
		{
			const fckc_size_t total = sizeof(*query->components) * count;
			fck_query_component *result = (fck_query_component *)kll_malloc(queries->arena, total);
			memcpy(result, components, total);
			query->components = result;
			query->count = count;
			query->hash = hash;
			query->size = size;
			query->name = name;
			const fck_query_id id = {.opaque = query};
			return id;
		}
		slot = (slot + 1) % queries->last->capacity;
	}
}

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

static fckc_u32 fck_entity_storage_api_set(fck_entity_storage *storage, fck_entity entity, int *just_added)
{
	if (just_added)
	{
		*just_added = 0;
	}
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
		(void)result;
		storage->dense[index] = entity;
		storage->count = storage->count + 1;

		if (just_added)
		{
			*just_added = 1;
		}

		return index + 1;
	}
	return 0;
}

static fckc_u32 fck_entity_storage_api_get(fck_entity_storage *entities, fck_entity entity)
{
	const fck_entity *result = fck_entity_lookup_api_get(&entities->lookup, entity);
	if (result)
	{
		// const fck_entity value = entities->dense[result->index];
		return result->index + 1;
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

static void *fck_entity_component_api_maybe_add(fck_entity_components *components, fck_entity entity, int *just_added)
{
	const fckc_u32 count = components->storage.count;
	const fckc_u32 capacity = components->storage.capacity;
	const fckc_u32 result = fck_entity_storage_api_set(&components->storage, entity, just_added);
	if (result == 0)
	{
		return NULL;
	}

	const fckc_u32 slot = result - 1;
	if (capacity < components->storage.capacity)
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
	fckc_u8 *dst = memory + offset;
	return (void *)(dst);
}

static void *fck_entity_component_api_add(fck_entity_components *components, fck_entity entity)
{
	int just_added;
	void *dst = fck_entity_component_api_maybe_add(components, entity, &just_added);
	if (!just_added)
	{
		return NULL;
	}

	memset(dst, 0, components->size);

	fck_component_definition *definition = &components->definition;
	if (just_added && definition->constructor)
	{
		definition->constructor(dst, definition->userdata);
	}
	return (void *)(dst);
}

static void *fck_entity_component_api_set(fck_entity_components *components, fck_entity entity, const void *data)
{
	void *dst = fck_entity_component_api_maybe_add(components, entity, NULL);
	if (data != NULL)
	{
		memcpy(dst, data, components->size);
	}
	else
	{
		memset(dst, 0, components->size);
	}
	return (void *)(dst);
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

		fckc_u8 *src = memory + from;
		fckc_u8 *dst = memory + to;

		fck_component_definition *definition = &components->definition;
		if (definition->destructor)
		{
			definition->destructor(dst, definition->userdata);
		}

		memmove(dst, src, components->size);
		memset(src, 0, components->size);
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
	ec->queries = fck_queries_create(allocator);

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
	fck_queries_destroy(&ec_private->queries);
	fck_entity_buffer_destroy(allocator, &ec_private->free_list);
	kll_free(allocator, ec_private);
}

static fck_entity fck_ec_api_entity_invalid(fck_ec ec)
{
	fck_entity entity = {0};
	return entity;
}

static int fck_ec_api_entity_is_ok(fck_ec ec, fck_entity entity)
{
	fck_ec_private *ec_private = ec.opaque;
	const fckc_u32 result = fck_entity_storage_api_get(&ec_private->all, entity);
	if (result)
	{
		return 1;
	}
	return 0;
}

static fck_entity fck_ec_api_entity_create(fck_ec ec)
{
	fck_ec_private *ec_private = ec.opaque;
	fck_entity entity = {0};
	if (!fck_entity_buffer_try_pop(&ec_private->free_list, &entity))
	{
		entity.index = ec_private->all.count;
		entity.alive = 1;
	}

	entity.generation = entity.generation + 1;

	fck_entity_storage_api_set(&ec_private->all, entity, NULL);
	return entity;
}

// static void fck_ec_api_entity_query_print(fck_ec_private *ec_private, fck_entity entity)
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
	fck_ec_private *ec_private = ec.opaque;
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

static fck_entity fck_ec_api_entity_copy(fck_ec ec, fck_entity entity)
{
	fck_ec_private *ec_private = ec.opaque;
	fck_entity result = fck_ec_api_entity_create(ec);

	fck_entity_components *current = ec_private->first;
	while (current)
	{
		const void *src = fck_entity_component_api_get(current, entity);
		if (src)
		{
			void *dst = fck_entity_component_api_set(current, result, src);
			// Setting can realloc - There are probably cheaper ways of doing it
			// But this way we can also ensure we copy correctly
			src = fck_entity_component_api_get(current, entity);
			fck_component_definition *definition = &current->definition;
			if (definition->copy)
			{
				definition->copy(dst, src, definition->userdata);
			}
		}

		current = current->next;
	}

	return result;
}

static fck_component_id fck_ec_api_component_declare(fck_ec ec, const char *name, fckc_u32 size)
{
	fck_ec_private *ec_private = ec.opaque;
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

			const fck_component_id result = {.value = slot + 1};
			return result;
		}

		if (strcmp(components->name, name) == 0)
		{
			const fck_component_id result = {.value = slot + 1};
			// Already added
			return result;
		}
		const fck_hash_int other = fck_hash(components->name, strlen(components->name));
		fck_assert(hash != other);
		(void)other;
		slot = (slot + 1) % ec_private->capacity;
	}
	//// Out of capacity!??
	{
		const fck_component_id result = {0};
		return result;
	}
}

static fck_entity_components *fck_ec_api_get_components(fck_ec ec, const char *name)
{
	fck_ec_private *ec_private = ec.opaque;
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

static fck_component_id fck_ec_api_components_make_id(fck_ec ec, fck_entity_components *components)
{
	fck_ec_private *ec_private = ec.opaque;
	const fckc_uintptr offset = components - ec_private->components;
	const fck_component_id result = {.value = offset + 1};
	return result;
}

static fck_component_id fck_ec_api_components_id(fck_ec ec, const char *name)
{
	fck_entity_components *components = fck_ec_api_get_components(ec, name);
	if (components)
	{
		return fck_ec_api_components_make_id(ec, components);
	}
	{
		const fck_component_id result = {0};
		return result;
	}
}

static fck_entity_components *fck_ec_api_components_resolve(fck_ec ec, fck_component_id id)
{
	if (id.value == 0)
	{
		return NULL;
	}
	fck_ec_private *ec_private = ec.opaque;
	return ec_private->components + (id.value - 1);
}

static int fck_ec_api_component_define(fck_ec ec, fck_component_id id, const fck_component_definition *definition)
{
	fck_entity_components *components = fck_ec_api_components_resolve(ec, id);
	if (components)
	{
		components->definition = *definition;
		return 1;
	}
	return 0;
}

static const char *fck_ec_api_components_nameof(fck_ec ec, fck_component_id id)
{
	fck_entity_components *components = fck_ec_api_components_resolve(ec, id);
	if (components)
	{
		return components->name;
	}
	return NULL;
}

static void *fck_ec_api_component_add(fck_ec ec, fck_entity entity, fck_component_id id)
{
	fck_ec_private *ec_private = ec.opaque;
	const fckc_u32 result = fck_entity_storage_api_get(&ec_private->all, entity);
	if (result)
	{
		fck_entity_components *components = fck_ec_api_components_resolve(ec, id);
		if (components)
		{
			return fck_entity_component_api_add(components, entity);
		}
	}
	return NULL;
}

static int fck_ec_api_component_set(fck_ec ec, fck_entity entity, fck_component_id id, const void *data)
{
	fck_ec_private *ec_private = ec.opaque;
	const fckc_u32 result = fck_entity_storage_api_get(&ec_private->all, entity);
	if (result)
	{
		fck_entity_components *components = fck_ec_api_components_resolve(ec, id);
		if (components)
		{
			fck_entity_component_api_set(components, entity, data);

			return 1;
		}
	}
	return 0;
}

static void *fck_ec_api_component_get(fck_ec ec, fck_entity entity, fck_component_id id)
{
	fck_ec_private *ec_private = ec.opaque;
	const fckc_u32 result = fck_entity_storage_api_get(&ec_private->all, entity);
	if (result)
	{
		fck_entity_components *components = fck_ec_api_components_resolve(ec, id);
		if (components)
		{
			return fck_entity_component_api_get(components, entity);
		}
	}
	return NULL;
}

static int fck_ec_api_component_remove(fck_ec ec, fck_entity entity, fck_component_id id)
{
	fck_ec_private *ec_private = ec.opaque;
	const fckc_u32 result = fck_entity_storage_api_get(&ec_private->all, entity);
	if (result)
	{
		fck_entity_components *components = fck_ec_api_components_resolve(ec, id);
		if (components)
		{
			fck_entity_component_api_remove(components, entity);
			return 1;
		}
	}
	return 0;
}

static void *fck_ec_api_get_component_buffer(fck_ec ec, fck_component_id id)
{
	fck_entity_components *components = fck_ec_api_components_resolve(ec, id);
	if (components)
	{
		return fck_entity_component_api_buffer(components);
	}
	return NULL;
}

static fckc_u32 fck_ec_api_all(fck_ec ec, const fck_entity **values)
{
	fck_ec_private *ec_private = ec.opaque;
	*values = ec_private->all.dense;
	return ec_private->all.count;
}

static fckc_u32 fck_ec_api_get_component_dense(fck_ec ec, fck_component_id id, const fck_entity **values)
{
	fck_entity_components *components = fck_ec_api_components_resolve(ec, id);
	if (components)
	{
		return fck_entity_component_api_dense(components, values);
	}
	return 0;
}

static fck_query_id fck_ec_api_query_get(fck_ec ec, const fck_query_description *description)
{
	fck_ec_private *ec_private = ec.opaque;
	return fck_queries_get(&ec_private->queries, description->name, description->size, description->components, description->count);
}

static fck_query_iterator fck_ec_api_query_iterator(fck_ec ec, fck_query_id query)
{
	fck_query_iterator it = {0};
	it.ec = ec;
	it.query = query;
	return it;
}

static fckc_u32 fck_ec_api_query_match(fck_query_iterator *it, void *dst, fckc_u32 capacity)
{
	const fck_query_id query_id = it->query;
	fck_query *query = fck_queries_resolve(query_id);
	if (query == NULL)
	{
		return 0;
	}

	fck_assert(query->count && "Fix me if you see me :giggle:");

	const fck_ec ec = it->ec;
	const fck_entity *entities = NULL;
	fckc_u32 entities_count = 0;
	{
		// First
		const fck_query_component *query_component = query->components + 0;
		fck_entity_components *components = fck_ec_api_components_resolve(ec, query_component->id);
		if (components == NULL)
		{
			return 0;
		}
		entities_count = fck_entity_component_api_dense(components, &entities);
	}

	fckc_u32 result = 0;
	for (; it->index < entities_count; it->index++)
	{
		if (result == capacity)
		{
			break;
		}

		const fck_entity entity = entities[it->index];
		void *current = fckc_pointer_add(dst, result * query->size);

		int write = 1;
		for (fckc_size_t component_index = 0; component_index < query->count; component_index++)
		{
			const fck_query_component *query_component = query->components + component_index;
			void *component_buffer = fckc_pointer_add(current, query_component->offset);

			fck_entity_components *components = fck_ec_api_components_resolve(ec, query_component->id);
			void *data = fck_entity_component_api_get(components, entity);
			if (data == NULL)
			{
				write = 0;
				break;
			}
			memcpy(component_buffer, data, components->size);
		}
		if (write)
		{
			result = result + 1;
		}
	}
	return result;
}

static fck_archetype_iterator fck_ec_api_archetype_iterator(fck_ec ec, fck_entity entity)
{
	fck_ec_private *ec_private = ec.opaque;

	fck_archetype_iterator it = {0};
	it.ec = ec;
	it.entity = entity;
	it.opaque = ec_private->first;
	return it;
}

static fckc_u32 fck_ec_api_archetype_get(fck_archetype_iterator *it, fck_component_id *components, fckc_u32 capacity)
{
	if (it->opaque == NULL)
	{
		return 0;
	}

	fckc_u32 result = 0;
	while (it->opaque)
	{
		if (result == capacity)
		{
			break;
		}
		fck_entity_components *current = (fck_entity_components *)it->opaque;
		void *data = fck_entity_component_api_get(current, it->entity);
		if (data)
		{
			components[result] = fck_ec_api_components_make_id(it->ec, current);
			result = result + 1;
		}
		it->opaque = current->next;
	}
	return result;
}

static fck_component_names_iterator fck_ec_api_registry_names_iterator(fck_ec ec)
{
	fck_ec_private *ec_private = ec.opaque;

	fck_component_names_iterator it = {0};
	it.ec = ec;
	it.opaque = ec_private->first;
	return it;
}

static fckc_u32 fck_ec_api_registry_names(fck_component_names_iterator *it, const char **names, fckc_u32 capacity)
{
	if (names == NULL && capacity == 0)
	{
		// TODO: Do the same for the others!
		fckc_u32 result = 0;
		while (it->opaque)
		{
			fck_entity_components *current = (fck_entity_components *)it->opaque;
			result = result + 1;
			it->opaque = current->next;
		}
		return result;
	}

	{
		fck_assert(names);
		if (it->opaque == NULL)
		{
			return 0;
		}

		fckc_u32 result = 0;
		while (it->opaque)
		{
			if (result == capacity)
			{
				break;
			}

			fck_entity_components *current = (fck_entity_components *)it->opaque;
			names[result] = current->name;
			result = result + 1;
			it->opaque = current->next;
		}
		return result;
	}
}

static fck_ec_archetype_api ec_archetype_api = {
	.iterator = fck_ec_api_archetype_iterator,
	.get = fck_ec_api_archetype_get,
};

static fck_ec_entity_api ec_entity_api = {
	.all = fck_ec_api_all,
	.invalid = fck_ec_api_entity_invalid,
	.create = fck_ec_api_entity_create,
	.copy = fck_ec_api_entity_copy,
	.is_ok = fck_ec_api_entity_is_ok,
	.destroy = fck_ec_api_entity_destroy,
};

static fck_ec_component_api ec_component_api = {
	.add = fck_ec_api_component_add,
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

static fck_ec_registry_api ec_registry_api = {
	.declare = fck_ec_api_component_declare,
	.define = fck_ec_api_component_define,
	.id = fck_ec_api_components_id,
	.nameof = fck_ec_api_components_nameof,
	.iterator = fck_ec_api_registry_names_iterator,
	.names = fck_ec_api_registry_names,
};

static fck_ec_query_api ec_query_qpi = {
	.iterator = fck_ec_api_query_iterator,
	.get = fck_ec_api_query_get,
	.match = fck_ec_api_query_match,
};

static fck_ec_api ec_api = {
	.archetype = &ec_archetype_api,
	.registry = &ec_registry_api,
	.query = &ec_query_qpi,
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