#ifndef FCK_ECS_INCLUDED
#define FCK_ECS_INCLUDED

#include <SDL3/SDL_stdinc.h>

struct fck_entity_definition
{
	// Fill out definition info
};

struct fck_entity
{
	size_t index;
};

struct fck_component_handle
{
	size_t index;
};

struct fck_component_definition
{
	fck_component_handle handle;
	size_t byte_count;
};

struct fck_component_collection
{
	// void* when we can apply type-erasure - C# version object
	// In other words, when we can cast it back to an explicit type use void*
	// If we cannot cast it back, we need to use byte*/char*
	uint8_t *components;

	size_t capcity;
};

struct fck_entity_registry
{
	fck_entity *entities;
	fck_entity_definition *definitions;

	size_t count;
	size_t capacity;
};

// Rows and columns - Essentialy a 2D matrix
struct fck_component_registry
{
	fck_component_collection *collections;
	fck_component_definition *definitions;

	size_t capacity;
};

struct fck_ecs_allocate_configuration
{
	size_t initial_entities_count;
	size_t initial_components_count;
};

struct fck_ecs
{
	fck_entity_registry entities;
	fck_component_registry components;
};

inline void fck_entity_registry_allocate(fck_entity_registry *registry, size_t initial_count)
{
	SDL_assert(registry != nullptr && "Entity registry cannot be NULL");
	SDL_assert(registry->entities == nullptr && "Entity registry entities already allocated");
	SDL_assert(registry->definitions == nullptr && "Entity registry definitions already allocated");

	registry->entities = (fck_entity *)SDL_calloc(sizeof(*registry->entities), initial_count);
	registry->definitions = (fck_entity_definition *)SDL_calloc(sizeof(*registry->definitions), initial_count);
	registry->count = 0;
	registry->capacity = initial_count;
}

inline void fck_entity_registry_free(fck_entity_registry *registry)
{
	SDL_assert(registry != nullptr && "Entity registry cannot be NULL");
	SDL_assert(registry->entities != nullptr && "Entity registry entities is not allocated");
	SDL_assert(registry->definitions != nullptr && "Entity registry definitions is not allocated");

	SDL_free(registry->entities);
	SDL_free(registry->definitions);

	registry->entities = nullptr;
	registry->definitions = nullptr;
}

inline void fck_component_registry_allocate(fck_component_registry *registry, size_t initial_count)
{
	SDL_assert(registry != nullptr && "Entity registry cannot be NULL");
	SDL_assert(registry->collections == nullptr && "Entity registry entities already allocated");
	SDL_assert(registry->definitions == nullptr && "Entity registry definitions already allocated");

	// calloc zeroes the memory. Counts are automatically 0!
	registry->collections = (fck_component_collection *)SDL_calloc(sizeof(*registry->collections), initial_count);
	registry->definitions = (fck_component_definition *)SDL_calloc(sizeof(*registry->definitions), initial_count);
	registry->capacity = initial_count;
}

inline void fck_component_registry_free(fck_component_registry *registry)
{
	SDL_assert(registry != nullptr && "Entity registry cannot be NULL");
	SDL_assert(registry->collections != nullptr && "Entity registry entities is not allocated");
	SDL_assert(registry->definitions != nullptr && "Entity registry definitions is not allocated");

	for (size_t index = 0; index < registry->capacity; index++)
	{
		// TODO: Free error - I am tired. We can fix it later. OS takes care of it
		// anyway
		fck_component_collection *component_collection = &registry->collections[index];
		if (component_collection->capcity != 0)
		{
			SDL_free(component_collection->components);
		}
	}

	SDL_free(registry->collections);
	SDL_free(registry->definitions);

	registry->collections = nullptr;
	registry->definitions = nullptr;
}

inline void fck_ecs_allocate(fck_ecs *ecs, fck_ecs_allocate_configuration const *configuration)
{
	SDL_assert(ecs != nullptr);
	fck_entity_registry_allocate(&ecs->entities, configuration->initial_entities_count);
	fck_component_registry_allocate(&ecs->components, configuration->initial_components_count);
}

inline void fck_ecs_free(fck_ecs *ecs)
{
	SDL_assert(ecs != nullptr);
	fck_entity_registry_free(&ecs->entities);
	fck_component_registry_free(&ecs->components);
}

inline fck_entity fck_entity_create(fck_ecs *ecs)
{
	SDL_assert(ecs != nullptr);
	SDL_assert(ecs->entities.count < ecs->entities.capacity);

	fck_entity_registry *entity_registry = &ecs->entities;

	size_t index = entity_registry->count;
	entity_registry->count = entity_registry->count + 1; // Advance

	fck_entity_definition definition;

	fck_entity entity;
	entity.index = index;

	// TODO:
	// zero out components
	// zero out definition

	return entity;
}

inline fck_component_handle fck_component_register(fck_ecs *ecs, size_t unique_id, size_t component_byte_size)
{
	SDL_assert(ecs != nullptr);

	fck_component_handle handle;
	handle.index = unique_id;

	fck_component_definition definition;
	definition.handle = handle;
	definition.byte_count = component_byte_size;

	// Allocate in ecs!
	fck_component_collection *component_collection = &ecs->components.collections[handle.index];

	// Handle double register
	SDL_assert(component_collection->capcity == 0 && "Component slot already used by another type");

	ecs->components.definitions[handle.index] = definition;

	component_collection->components = (uint8_t *)SDL_calloc(ecs->entities.capacity, component_byte_size);
	component_collection->capcity = ecs->entities.capacity;
	return handle;
}

inline uint8_t *fck_component_get(fck_ecs *ecs, fck_entity const *entity, fck_component_handle const *handle)
{
	SDL_assert(handle != nullptr);

	size_t index = handle->index;

	fck_component_definition *definition = &ecs->components.definitions[index];
	fck_component_collection *collection = &ecs->components.collections[index];
	uint8_t *component_data = &collection->components[entity->index * definition->byte_count];

	return component_data;
}

inline void fck_component_set(fck_ecs *ecs, fck_entity const *entity, fck_component_handle const *handle, void *data)
{
	SDL_assert(handle != nullptr);

	size_t index = handle->index;

	fck_component_definition *definition = &ecs->components.definitions[index];
	fck_component_collection *collection = &ecs->components.collections[index];
	uint8_t *component_data = &collection->components[entity->index * definition->byte_count];

	SDL_memcpy(component_data, data, definition->byte_count);
}

#endif // !FCK_ECS_INCLUDED