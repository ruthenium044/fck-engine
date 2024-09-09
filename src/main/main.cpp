// fck main
// TODO:
// - Graphics
// - Input handling
// - Draw some images
// - Systems and data model
// - Data serialisation
// - Networking!! <- implies multiplayer

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_net/SDL_net.h>

#include "fck_keyboard.h"
#include "fck_mouse.h"

#define LOG_LAST_CRITICAL_SDL_ERROR() SDL_LogCritical(0, "%s:%d - %s", __FILE__, __LINE__, SDL_GetError());

typedef unsigned char fck_byte;

struct fck_entity_storage_header
{
	size_t byte_index;
	size_t byte_count;
	size_t definition_index;
	int unsused;
};

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
	// void* when we can apply type-erasure
	// In other words, when we can cast it back to an explicit type use void*
	// If we cannot cast it back, we need to use byte*/char*
	void *components;

	size_t count;
	size_t capcity;
};

struct fck_entity_registry
{
	// Food for throught: should the definition be part of the fck_entity type itself?
	fck_entity *entities;
	fck_entity_definition *definitions;

	size_t count;
	size_t capacity;
};

// Rows and columns
struct fck_component_registry
{
	fck_component_definition *definitions;
	fck_component_collection *collections;

	size_t count;
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

void fck_entity_registry_allocate(fck_entity_registry *registry, size_t initial_count)
{
	SDL_assert(registry != nullptr && "Entity registry cannot be NULL");
	SDL_assert(registry->entities == nullptr && "Entity registry entities already allocated");
	SDL_assert(registry->definitions == nullptr && "Entity registry definitions already allocated");

	registry->entities = (fck_entity *)SDL_calloc(sizeof(*registry->entities), initial_count);
	registry->definitions = (fck_entity_definition *)SDL_calloc(sizeof(*registry->definitions), initial_count);
	registry->count = 0;
	registry->capacity = initial_count;
}

void fck_entity_registry_free(fck_entity_registry *registry)
{
	SDL_assert(registry != nullptr && "Entity registry cannot be NULL");
	SDL_assert(registry->entities != nullptr && "Entity registry entities is not allocated");
	SDL_assert(registry->definitions != nullptr && "Entity registry definitions is not allocated");

	SDL_free(registry->entities);
	SDL_free(registry->definitions);

	registry->entities = nullptr;
	registry->definitions = nullptr;
}

void fck_component_registry_allocate(fck_component_registry *registry, size_t initial_count)
{
	SDL_assert(registry != nullptr && "Entity registry cannot be NULL");
	SDL_assert(registry->collections == nullptr && "Entity registry entities already allocated");
	SDL_assert(registry->definitions == nullptr && "Entity registry definitions already allocated");

	// calloc zeroes the memory. Counts are automatically 0!
	registry->collections = (fck_component_collection *)SDL_calloc(sizeof(*registry->collections), initial_count);
	registry->definitions = (fck_component_definition *)SDL_calloc(sizeof(*registry->definitions), initial_count);
	registry->count = 0;
	registry->capacity = initial_count;
}

void fck_component_registry_free(fck_component_registry *registry)
{
	SDL_assert(registry != nullptr && "Entity registry cannot be NULL");
	SDL_assert(registry->collections != nullptr && "Entity registry entities is not allocated");
	SDL_assert(registry->definitions != nullptr && "Entity registry definitions is not allocated");

	for (size_t index = 0; index < registry->capacity; index++)
	{
		fck_component_collection *component_collection = &registry->collections[index];
		SDL_free(component_collection);
	}

	SDL_free(registry->collections);
	SDL_free(registry->definitions);

	registry->collections = nullptr;
	registry->definitions = nullptr;
}

void fck_ecs_allocate(fck_ecs *ecs, fck_ecs_allocate_configuration const *configuration)
{
	SDL_assert(ecs != nullptr);
	fck_entity_registry_allocate(&ecs->entities, configuration->initial_entities_count);
	fck_component_registry_allocate(&ecs->components, configuration->initial_components_count);
}

void fck_ecs_free(fck_ecs *ecs)
{
	SDL_assert(ecs != nullptr);
	fck_entity_registry_free(&ecs->entities);
	fck_component_registry_free(&ecs->components);
}

fck_entity fck_entity_create(fck_ecs *ecs)
{
	SDL_assert(ecs != nullptr);

	fck_entity_registry *entity_registry = &ecs->entities;

	size_t index = entity_registry->count;
	entity_registry->count = entity_registry->count + 1; // Advance

	fck_entity_definition definition;

	fck_entity entity;
	entity.index = index;

	return entity;
}

fck_component_handle fck_component_register(fck_ecs *ecs, size_t unique_id, size_t component_byte_size)
{
	SDL_assert(ecs != nullptr);

	fck_component_handle handle;
	handle.index = unique_id;

	fck_component_definition definition;
	definition.handle = handle;
	definition.byte_count = component_byte_size;

	// Allocate in ecs!
	ecs->components.definitions[handle.index] = definition;
	fck_component_collection *component_collection = &ecs->components.collections[handle.index];
	component_collection->components = SDL_calloc(ecs->entities.capacity, component_byte_size);
	component_collection->count = 0;
	component_collection->capcity = component_byte_size;
	return handle;
}

int main(int, char **)
{
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
	{
		LOG_LAST_CRITICAL_SDL_ERROR();
		return -1;
	}

	SDL_Window *window = SDL_CreateWindow("fck - engine", 640, 640, 0);
	if (window == nullptr)
	{
		LOG_LAST_CRITICAL_SDL_ERROR();
		return -1;
	}

	SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);
	if (renderer == nullptr)
	{
		LOG_LAST_CRITICAL_SDL_ERROR();
		return -1;
	}
	if (!SDL_SetRenderVSync(renderer, true))
	{
		LOG_LAST_CRITICAL_SDL_ERROR();
		return -1;
	}

	fck_keyboard_state keyboard;
	SDL_zero(keyboard);

	fck_mouse_state mouse;
	SDL_zero(mouse);

	fck_ecs ecs;
	SDL_zero(ecs);

	// Initialise data blob
	// Unused - Just test
	const int INITIAL_ENTITY_COUNT = 128;
	const int INITIAL_COMPONENT_COUNT = 32;

	fck_ecs_allocate_configuration allocate_configuration;
	allocate_configuration.initial_entities_count = INITIAL_ENTITY_COUNT;
	allocate_configuration.initial_components_count = INITIAL_COMPONENT_COUNT;

	fck_ecs_allocate(&ecs, &allocate_configuration);

	fck_entity player_entity = fck_entity_create(&ecs);
	// !Unused - Just test

	SDL_FRect player = {0, 0, 128, 128};

	SDL_FRect enemy = {256, 256, 64, 64};

	bool is_running = true;
	while (is_running)
	{
		// Event processing - Input, window, etc.
		SDL_Event ev;
		while (SDL_PollEvent(&ev))
		{
			if (ev.type == SDL_EVENT_QUIT)
			{
				is_running = false;
			}
		}
		fck_keyboard_state_update(&keyboard);
		fck_mouse_state_update(&mouse);

		SDL_FPoint direction = {0.0f, 0.0f};
		if (fck_key_down(&keyboard, SDL_SCANCODE_A))
		{
			direction.x -= 1.0f;
		}
		if (fck_key_down(&keyboard, SDL_SCANCODE_D))
		{
			direction.x += 1.0f;
		}
		if (fck_key_down(&keyboard, SDL_SCANCODE_W))
		{
			direction.y -= 1.0f;
		}
		if (fck_key_down(&keyboard, SDL_SCANCODE_S))
		{
			direction.y += 1.0f;
		}
		player.x += direction.x;
		player.y += direction.y;

		// Render processing
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		SDL_SetRenderDrawColor(renderer, 0, 0, 255, 0);
		SDL_RenderFillRect(renderer, &player);

		SDL_SetRenderDrawColor(renderer, 255, 0, 0, 0);
		SDL_RenderFillRect(renderer, &enemy);

		SDL_RenderPresent(renderer);
	}

	fck_ecs_free(&ecs);

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();
	return 0;
}