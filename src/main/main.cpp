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

#include "fck_checks.h"
#include "fck_drop_file.h"
#include "fck_ecs.h"
#include "fck_keyboard.h"
#include "fck_mouse.h"

struct fck_font_config
{
	char relative_path[256];
	int pixel_per_glyph_w;
	int pixel_per_glyph_h;
	int columns;
	int rows;
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

struct fck_pig
{
	// :((
};

struct fck_wolf
{
	double itsadouble;
	// :((
};

struct fck_file_memory
{
	Uint8 *data;
	size_t size;
};

bool fck_file_write(const char *path, const char *name, const char *extension, const void *source, size_t size)
{
	SDL_assert(path != nullptr && "NULL is not a path");
	SDL_assert(name != nullptr && "NULL is not a name");
	SDL_assert(extension != nullptr && "NULL is not an extension");
	SDL_assert(source != nullptr && size != 0);

	char path_buffer[512];
	SDL_zero(path_buffer);

	size_t added_length = SDL_strlcat(path_buffer, FCK_RESOURCE_DIRECTORY_PATH, sizeof(path_buffer));
	added_length = added_length + SDL_strlcat(path_buffer + added_length, path, sizeof(path_buffer));
	added_length = added_length + SDL_strlcat(path_buffer + added_length, name, sizeof(path_buffer));
	SDL_strlcat(path_buffer + added_length, extension, sizeof(path_buffer));

	SDL_IOStream *stream = SDL_IOFromFile(path_buffer, "wb");
	CHECK_WARNING(stream == nullptr, SDL_GetError(), return false);

	size_t written_size = SDL_WriteIO(stream, source, size);
	CHECK_WARNING(written_size < size, SDL_GetError());

	SDL_bool result = SDL_CloseIO(stream);
	CHECK_WARNING(!result, SDL_GetError(), return false);

	return true;
}

bool fck_file_read(const char *path, const char *name, const char *extension, fck_file_memory *output)
{
	SDL_assert(path != nullptr && "NULL is not a path");
	SDL_assert(name != nullptr && "NULL is not a name");
	SDL_assert(extension != nullptr && "NULL is not an extension");
	SDL_assert(output != nullptr);

	char path_buffer[512];
	SDL_zero(path_buffer);

	size_t added_length = SDL_strlcat(path_buffer, FCK_RESOURCE_DIRECTORY_PATH, sizeof(path_buffer));
	added_length = added_length + SDL_strlcat(path_buffer + added_length, path, sizeof(path_buffer));
	added_length = added_length + SDL_strlcat(path_buffer + added_length, name, sizeof(path_buffer));
	SDL_strlcat(path_buffer + added_length, extension, sizeof(path_buffer));

	SDL_IOStream *stream = SDL_IOFromFile(path_buffer, "rb");
	CHECK_ERROR(stream == nullptr, SDL_GetError(), return false);

	Sint64 stream_size = SDL_GetIOSize(stream);
	CHECK_ERROR(stream_size < 0, SDL_GetError());

	Uint8 *data = (Uint8 *)SDL_malloc(stream_size);

	size_t read_size = SDL_ReadIO(stream, data, stream_size);
	SDL_bool has_error_in_reading = read_size < stream_size;
	CHECK_ERROR(has_error_in_reading, SDL_GetError());

	SDL_bool result = SDL_CloseIO(stream);
	CHECK_ERROR(!result, SDL_GetError());

	if (has_error_in_reading)
	{
		SDL_free(data);
		output->data = nullptr;
		output->size = 0;
		return false;
	}

	output->data = data;
	output->size = read_size;

	return true;
}

void fck_file_free(fck_file_memory *file_memory)
{
	SDL_free(file_memory->data);
	file_memory->data = nullptr;
	file_memory->size = 0;
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

	// There is actually no possible way the path is longer than 2024... Let's
	// just pray
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

		if (fck_button_down(mouse_state, SDL_BUTTON_LEFT))
		{
			font_editor->editor_pivot_x -= mouse_point.x - mouse_state->previous.cursor_position_x;
			font_editor->editor_pivot_y -= mouse_point.y - mouse_state->previous.cursor_position_y;
		}

		SDL_RenderTexture(renderer, font_editor->selected_font_texture, &texture_rect_src, &texture_rect_dst);
		SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
		SDL_RenderRect(renderer, &texture_rect_dst);

		SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
		float scaled_glyph_w = font_editor->pixel_per_glyph_w * scale;
		float scaled_glyph_h = font_editor->pixel_per_glyph_h * scale;

		int glyph_cols = scaled_w / scaled_glyph_w;
		int glyph_rows = scaled_h / scaled_glyph_h;

		for (int glyph_index_y = 0; glyph_index_y < glyph_rows; glyph_index_y++)
		{
			for (int glyph_index_x = 0; glyph_index_x < glyph_cols; glyph_index_x++)
			{
				float glyph_x = dst_x + (scaled_glyph_w * glyph_index_x);
				float glyph_y = dst_y + (scaled_glyph_h * glyph_index_y);
				SDL_FRect glyph_rect = {glyph_x, glyph_y, scaled_glyph_w, scaled_glyph_h};
				SDL_RenderRect(renderer, &glyph_rect);
			}
		}

		int window_width;
		int window_height;
		if (SDL_GetWindowSize(engine->window, &window_width, &window_height))
		{
			const float save_button_padding = 4.0f;
			const float save_button_x = 0.0f + save_button_padding;
			const float save_button_width = window_width - (save_button_padding * 2.0f);
			const float save_button_height = 64.0f - (save_button_padding * 2.0f);
			const float save_button_y = window_height - save_button_height - save_button_padding;
			SDL_FRect save_button_rect = {save_button_x, save_button_y, save_button_width, save_button_height};

			if (fck_rect_point_intersection(&save_button_rect, &mouse_point))
			{
				SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
				if (fck_button_just_down(mouse_state, SDL_BUTTON_LEFT))
				{
					fck_font_config config;
					SDL_zero(config);
					SDL_strlcpy(config.relative_path, "Test path", sizeof(config.relative_path));
					config.pixel_per_glyph_w = font_editor->pixel_per_glyph_w;
					config.pixel_per_glyph_h = font_editor->pixel_per_glyph_h;
					config.rows = glyph_rows;
					config.columns = glyph_cols;

					bool write_result = fck_file_write("", "special", ".font", &config, sizeof(config));
					// TODO: Finish saving!! Make it nice
				}
			}
			else
			{

				SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
			}

			SDL_RenderRect(renderer, &save_button_rect);
		}
	}

	SDL_RenderPresent(renderer);
}

// TODO: Clean up this prototype of a function :D
void fck_render_text(fck_engine *engine, const char *text, int scale = 2)
{
	// 64 hard limit. No reason
	fck_font_editor *font_editor = &engine->font_editor;
	float glyph_w = font_editor->pixel_per_glyph_w;
	float glyph_h = font_editor->pixel_per_glyph_h;
	SDL_FRect src_rect = {0, 0, glyph_w, glyph_h};
	SDL_FRect dst_rect = {0, 0, glyph_w * scale, glyph_h * scale};

	float tw;
	float th;
	if (!SDL_GetTextureSize(font_editor->selected_font_texture, &tw, &th))
	{
		return;
	}

	int glyph_cols = tw / glyph_w;
	int glyph_rows = th / glyph_h;

	for (size_t index = 0; index < 64; index++)
	{
		const char c = text[index];
		if (c == 0)
		{
			// maybe break if cleanup needed?
			return;
		}
		int x = c % glyph_cols;
		int y = c / glyph_cols;

		src_rect.x = x * glyph_w;
		src_rect.y = y * glyph_h;

		dst_rect.x = dst_rect.w * index;

		SDL_RenderTexture(engine->renderer, engine->font_editor.selected_font_texture, &src_rect, &dst_rect);
	}
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

	// Read default font config on startup and load it somwhere. Need a place for it
	fck_file_memory file_memory;
	if (fck_file_read("", "special", ".font", &file_memory))
	{
		fck_font_config *font_config = (fck_font_config *)file_memory.data;
		fck_file_free(&file_memory);
	}

	bool is_running = true;
	while (is_running)
	{
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

			fck_render_text(&engine, "TEEEST", 4);

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