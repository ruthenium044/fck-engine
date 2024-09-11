// fck main
// TODO:
// - Graphics
// - Input handling (x)
// - Draw some images
// - Systems and data model
// - Data serialisation
// - Frame independence
// - Networking!! <- implies multiplayer

// SDL core - functionality such as creating a window and getting events
#include <SDL3/SDL.h>

// SDL image - Loads images... Many kinds. We only care about PNG
#include <SDL3_image/SDL_image.h>

// SDL net - networking... More later
#include <SDL3_net/SDL_net.h>

#include "fck_keyboard.h"
#include "fck_mouse.h"

#define CHECK_INFO(condition, message)                                                                                 \
	if ((condition))                                                                                                   \
	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "%s:%d - %s", __FILE__, __LINE__, message)
#define CHECK_WARNING(condition, message)                                                                              \
	if ((condition))                                                                                                   \
	SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "%s:%d - %s", __FILE__, __LINE__, message)
#define CHECK_ERROR(condition, message)                                                                                \
	if ((condition))                                                                                                   \
	SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s:%d - %s", __FILE__, __LINE__, message)
#define CHECK_CRITICAL(condition, message)                                                                             \
	if ((condition))                                                                                                   \
	SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "%s:%d - %s", __FILE__, __LINE__, message)

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
	// Food for throught: should the definition be part of the fck_entity type itself?
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

struct fck_font_editor
{
	SDL_Texture *selected_font_texture;

	float editor_pivot_x;
	float editor_pivot_y;
	float editor_scale;

	int pixel_per_glyph_w;
	int pixel_per_glyph_h;
};

struct fck_engine
{
	SDL_Window *window;
	SDL_Renderer *renderer;

	fck_mouse_state mouse;
	fck_keyboard_state keyboard;

	fck_font_editor font_editor;
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
	registry->capacity = initial_count;
}

void fck_component_registry_free(fck_component_registry *registry)
{
	SDL_assert(registry != nullptr && "Entity registry cannot be NULL");
	SDL_assert(registry->collections != nullptr && "Entity registry entities is not allocated");
	SDL_assert(registry->definitions != nullptr && "Entity registry definitions is not allocated");

	for (size_t index = 0; index < registry->capacity; index++)
	{
		// TODO: Free error - I am tired. We can fix it later. OS takes care of it anyway
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

fck_component_handle fck_component_register(fck_ecs *ecs, size_t unique_id, size_t component_byte_size)
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

uint8_t *fck_component_get(fck_ecs *ecs, fck_entity const *entity, fck_component_handle const *handle)
{
	SDL_assert(handle != nullptr);

	size_t index = handle->index;

	fck_component_definition *definition = &ecs->components.definitions[index];
	fck_component_collection *collection = &ecs->components.collections[index];
	uint8_t *component_data = &collection->components[entity->index * definition->byte_count];

	return component_data;
}

void fck_component_set(fck_ecs *ecs, fck_entity const *entity, fck_component_handle const *handle, void *data)
{
	SDL_assert(handle != nullptr);

	size_t index = handle->index;

	fck_component_definition *definition = &ecs->components.definitions[index];
	fck_component_collection *collection = &ecs->components.collections[index];
	uint8_t *component_data = &collection->components[entity->index * definition->byte_count];

	SDL_memcpy(component_data, data, definition->byte_count);
}

struct fck_pig
{
	// :((
};

struct fck_wolf
{
	double itsadouble;
	// :((
};

struct fck_drop_file_context;

typedef bool (*fck_try_receive_drop_file)(fck_drop_file_context const *context, SDL_DropEvent const *drop_event);

struct fck_drop_file_context
{
	fck_try_receive_drop_file *drop_events;

	size_t count;
	size_t capacity;
};

void fck_drop_file_context_allocate(fck_drop_file_context *drop_file_context, size_t initial_capacity)
{
	SDL_assert(drop_file_context != nullptr);
	SDL_assert(drop_file_context->drop_events == nullptr && "Already allocated");

	const size_t element_size = sizeof(*drop_file_context->drop_events);
	drop_file_context->drop_events = (fck_try_receive_drop_file *)SDL_calloc(initial_capacity, element_size);
	drop_file_context->capacity = initial_capacity;
	drop_file_context->count = 0;
}

bool fck_drop_file_context_push(fck_drop_file_context *drop_file_context, fck_try_receive_drop_file func)
{
	SDL_assert(drop_file_context != nullptr);
	SDL_assert(drop_file_context->drop_events != nullptr);

	if (drop_file_context->count >= drop_file_context->capacity)
	{
		return false;
	}

	drop_file_context->drop_events[drop_file_context->count] = func;
	drop_file_context->count = drop_file_context->count + 1;
	return true;
}

void fck_drop_file_context_notify(fck_drop_file_context *drop_file_context, SDL_DropEvent const *drop_event)
{
	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Trying to load file: %s ...", drop_event->data);

	SDL_assert(drop_file_context != nullptr);
	SDL_assert(drop_file_context->drop_events != nullptr);

	for (size_t index = 0; index < drop_file_context->count; index++)
	{
		fck_try_receive_drop_file try_receive = drop_file_context->drop_events[index];
		bool result = try_receive(drop_file_context, drop_event);
		if (result)
		{
			break;
		}
	}
}

void fck_drop_file_context_free(fck_drop_file_context *drop_file_context)
{
	SDL_assert(drop_file_context != nullptr);
	SDL_assert(drop_file_context->drop_events != nullptr && "Non-sensical - Nothing to deallocate");

	const size_t element_size = sizeof(*drop_file_context->drop_events);
	SDL_free(drop_file_context->drop_events);
	drop_file_context->capacity = 0;
	drop_file_context->count = 0;
}

bool fck_drop_file_receive_png(fck_drop_file_context const *context, SDL_DropEvent const *drop_event)
{
	SDL_IOStream *stream = SDL_IOFromFile(drop_event->data, "r");
	CHECK_ERROR(stream == nullptr, SDL_GetError());

	if (!IMG_isPNG(stream))
	{
		// We only allow pngs for now!
		return false;
	}
	const char resource_path_base[] = FCK_RESOURCE_DIRECTORY_PATH;

	const char *target_file_path = drop_event->data;

	// Spin till the end
	const char *target_file_name = SDL_strrchr(target_file_path, '\\');
	SDL_assert(target_file_name != nullptr && "Potential file name is directory?");
	target_file_name = target_file_name + 1;

	char path_buffer[512];
	SDL_zero(path_buffer);
	size_t added_length = SDL_strlcat(path_buffer, resource_path_base, sizeof(path_buffer));

	// There is actually no possible way the path is longer than 2024... Let's just pray
	SDL_strlcat(path_buffer + added_length, target_file_name, sizeof(path_buffer));

	SDL_bool result = SDL_CopyFile(drop_event->data, path_buffer);
	CHECK_INFO(!result, SDL_GetError());

	return true;
}

bool fck_texture_load(SDL_Renderer *renderer, const char *relative_file_path, SDL_Texture **out_texture)
{
	const char resource_path_base[] = FCK_RESOURCE_DIRECTORY_PATH;

	char path_buffer[512];
	SDL_zero(path_buffer);

	size_t added_length = SDL_strlcat(path_buffer, resource_path_base, sizeof(path_buffer));

	SDL_strlcat(path_buffer + added_length, relative_file_path, sizeof(path_buffer));

	*out_texture = IMG_LoadTexture(renderer, path_buffer);

	return out_texture != nullptr;
}

void fck_font_editor_allocate(fck_font_editor *editor)
{
	editor->editor_scale = 1.0f;
	editor->editor_pivot_x = 0.0f;
	editor->editor_pivot_y = 0.0f;

	// Rows/Cols or target glyph size?
	editor->pixel_per_glyph_w = 8;
	editor->pixel_per_glyph_h = 12;
}

void fck_font_editor_free(fck_font_editor *editor)
{
}

bool fck_rect_point_intersection(SDL_FRect const *rect, SDL_FPoint const *point)
{
	return point->x > rect->x && point->x < rect->x + rect->w && point->y > rect->y && point->y < rect->y + rect->h;
}

void fck_font_editor_update(fck_engine *engine)
{
	fck_font_editor *font_editor = &engine->font_editor;
	SDL_Renderer *renderer = engine->renderer;
	fck_mouse_state *mouse_state = &engine->mouse;

	float scale = font_editor->editor_scale;

	scale = scale + mouse_state->current.scroll_delta_y * 0.25f;
	scale = SDL_clamp(scale, 0.1f, 8.0f);

	font_editor->editor_scale = scale;

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	if (font_editor->selected_font_texture != nullptr)
	{
		float w;
		float h;
		CHECK_ERROR(!SDL_GetTextureSize(font_editor->selected_font_texture, &w, &h), SDL_GetError());

		float x = font_editor->editor_pivot_x;
		float y = font_editor->editor_pivot_y;
		float half_w = w * 0.5f;
		float half_h = h * 0.5f;

		float scaled_w = w * scale;
		float scaled_h = h * scale;
		float scaled_half_w = scaled_w * 0.5f;
		float scaled_half_h = scaled_h * 0.5f;
		float offset_x = half_w - scaled_half_w;
		float offset_y = half_h - scaled_half_h;
		float dst_x = x + offset_x;
		float dst_y = y + offset_y;
		SDL_FRect texture_rect_src = {0, 0, w, h};
		SDL_FRect texture_rect_dst = {dst_x, dst_y, scaled_w, scaled_h};

		float previous_scale_x;
		float previous_scale_y;

		SDL_FPoint mouse_point = {mouse_state->current.cursor_position_x, mouse_state->current.cursor_position_y};

		if (fck_button_down(mouse_state, SDL_BUTTON_MIDDLE))
		{
			font_editor->editor_pivot_x -= mouse_point.x - mouse_state->previous.cursor_position_x;
			font_editor->editor_pivot_y -= mouse_point.y - mouse_state->previous.cursor_position_y;
		}

		SDL_SetTextureColorMod(font_editor->selected_font_texture, 255, 0, 0);

		SDL_RenderTexture(renderer, font_editor->selected_font_texture, &texture_rect_src, &texture_rect_dst);
		SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
		SDL_RenderRect(renderer, &texture_rect_dst);

		SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
		float scaled_glyph_w = font_editor->pixel_per_glyph_w * scale;
		float scaled_glyph_h = font_editor->pixel_per_glyph_h * scale;
		SDL_FRect glyph_rect = {dst_x, dst_y, scaled_glyph_w, scaled_glyph_h};

		for (float glyph_x = dst_x; glyph_x < dst_x + scaled_w; glyph_x = glyph_x + scaled_glyph_w)
		{
			for (float glyph_y = dst_y; glyph_y < dst_y + scaled_h; glyph_y = glyph_y + scaled_glyph_h)
			{
				SDL_FRect glyph_rect = {glyph_x, glyph_y, scaled_glyph_w, scaled_glyph_h};
				SDL_RenderRect(renderer, &glyph_rect);
			}
		}
	}

	SDL_RenderPresent(renderer);
}

int main(int c, char **str)
{
	fck_engine engine;
	SDL_zero(engine);

	// Init Systems
	CHECK_CRITICAL(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS), SDL_GetError());

	engine.window = SDL_CreateWindow("fck - engine", 640, 640, 0);
	CHECK_CRITICAL(engine.window == nullptr, SDL_GetError());

	engine.renderer = SDL_CreateRenderer(engine.window, nullptr);
	CHECK_CRITICAL(engine.renderer == nullptr, SDL_GetError());

	CHECK_WARNING(!SDL_SetRenderVSync(engine.renderer, true), SDL_GetError());

	SDL_Texture *default_font_texture;
	CHECK_CRITICAL(!fck_texture_load(engine.renderer, "Font.png", &default_font_texture), SDL_GetError());
	engine.font_editor.selected_font_texture = default_font_texture;

	// Init Application
	fck_ecs ecs;
	SDL_zero(ecs);

	fck_drop_file_context drop_file_context;
	SDL_zero(drop_file_context);

	fck_drop_file_context_allocate(&drop_file_context, 16);
	fck_drop_file_context_push(&drop_file_context, fck_drop_file_receive_png);

	// Init data blob
	// Unused - Just test
	const int INITIAL_ENTITY_COUNT = 128;
	const int INITIAL_COMPONENT_COUNT = 32;

	fck_ecs_allocate_configuration allocate_configuration;
	allocate_configuration.initial_entities_count = INITIAL_ENTITY_COUNT;
	allocate_configuration.initial_components_count = INITIAL_COMPONENT_COUNT;

	fck_ecs_allocate(&ecs, &allocate_configuration);

	fck_font_editor_allocate(&engine.font_editor);

	// Register types
	fck_component_handle pig_handle = fck_component_register(&ecs, 0, sizeof(fck_pig));
	fck_component_handle wolf_handle = fck_component_register(&ecs, 1, sizeof(fck_wolf));

	// Reserve a player slot
	fck_entity player_entity = fck_entity_create(&ecs);

	// Set and get data
	fck_wolf wolf = {420.0};
	fck_component_set(&ecs, &player_entity, &wolf_handle, &wolf);
	uint8_t *raw_wolf_data = fck_component_get(&ecs, &player_entity, &wolf_handle);
	fck_wolf *wolf_data = (fck_wolf *)raw_wolf_data;
	// !Unused - Just test

	bool is_running = true;
	while (is_running)
	{
		{
			// Exhaust error buffer
			const char *error = "";
			do
			{
				error = SDL_GetError();
				SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, error);
			} while (SDL_strcmp(error, "") != 0);
		}

		// Event processing - Input, window, etc.
		SDL_Event ev;

		float scroll_delta_x = 0.0f;
		float scroll_delta_y = 0.0f;
		while (SDL_PollEvent(&ev))
		{
			switch (ev.type)
			{
			case SDL_EventType::SDL_EVENT_QUIT:
				is_running = false;
				break;
			case SDL_EventType::SDL_EVENT_DROP_FILE:
				fck_drop_file_context_notify(&drop_file_context, &ev.drop);
				break;
			case SDL_EventType::SDL_EVENT_MOUSE_WHEEL:
				scroll_delta_x = scroll_delta_x + ev.wheel.x;
				scroll_delta_y = scroll_delta_y + ev.wheel.y;
				break;
			default:
				break;
			}
		}
		fck_keyboard_state_update(&engine.keyboard);
		fck_mouse_state_update(&engine.mouse, scroll_delta_x, scroll_delta_y);

		// Render processing
		bool is_font_editor_open = true;
		if (is_font_editor_open)
		{
			fck_font_editor_update(&engine);
		}
		else
		{
			SDL_SetRenderDrawColor(engine.renderer, 0, 0, 0, 255);
			SDL_RenderClear(engine.renderer);

			SDL_RenderPresent(engine.renderer);
		}
	}

	fck_font_editor_free(&engine.font_editor);

	fck_drop_file_context_free(&drop_file_context);

	fck_ecs_free(&ecs);

	SDL_DestroyRenderer(engine.renderer);
	SDL_DestroyWindow(engine.window);

	SDL_Quit();
	return 0;
}