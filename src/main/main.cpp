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
#include "fck_file.h"
#include "fck_keyboard.h"
#include "fck_memory_stream.h"
#include "fck_mouse.h"
#include "fck_ui.h"
#include <fck_spritesheet.h>

struct fck_font_editor
{
	SDL_Texture *selected_font_texture;
	const char *relative_texture_path;

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

	fck_font_asset default_editor_font;
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

void fck_font_editor_allocate(fck_font_editor *editor)
{
	SDL_assert(editor != nullptr);

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

void fck_font_editor_update(fck_engine *engine)
{
	fck_font_editor *font_editor = &engine->font_editor;
	SDL_Renderer *renderer = engine->renderer;
	fck_mouse_state *mouse_state = &engine->mouse;

	float scale = font_editor->editor_scale;

	scale = scale + mouse_state->current.scroll_delta_y * 0.25f;
	scale = SDL_clamp(scale, 0.1f, 8.0f);

	font_editor->editor_scale = scale;

	if (font_editor->selected_font_texture != nullptr)
	{
		float w;
		float h;
		CHECK_ERROR(SDL_GetTextureSize(font_editor->selected_font_texture, &w, &h), SDL_GetError());

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

		SDL_FPoint mouse_point = {mouse_state->current.cursor_position_x, mouse_state->current.cursor_position_y};

		if (fck_button_down(mouse_state, SDL_BUTTON_LEFT))
		{
			font_editor->editor_pivot_x -= mouse_point.x - mouse_state->previous.cursor_position_x;
			font_editor->editor_pivot_y -= mouse_point.y - mouse_state->previous.cursor_position_y;
		}
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		SDL_SetTextureColorMod(font_editor->selected_font_texture, 255, 255, 255);
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

			fck_ui_button_style button_style = fck_ui_button_style_engine(&engine->default_editor_font);
			if (fck_ui_button(engine->renderer, &engine->mouse, &save_button_rect, &button_style, "SAVE TO FILE"))
			{
				if (font_editor->relative_texture_path != nullptr)
				{
					fck_font_resource config;
					SDL_zero(config);
					SDL_strlcpy(config.texture_path, font_editor->relative_texture_path, sizeof(config.texture_path));
					config.pixel_per_glyph_w = font_editor->pixel_per_glyph_w;
					config.pixel_per_glyph_h = font_editor->pixel_per_glyph_h;
					config.rows = glyph_rows;
					config.columns = glyph_cols;

					bool write_result = fck_file_write("", "special", ".font", &config, sizeof(config));
				}
			}
		}
	}
}

SDL_bool fck_font_asset_load(SDL_Renderer *renderer, const char *file_name, fck_font_asset *font_asset)
{
	fck_file_memory file_memory;
	if (!fck_file_read("", file_name, ".font", &file_memory))
	{
		return false;
	}

	fck_font_resource *font_resource = (fck_font_resource *)file_memory.data;

	font_asset->pixel_per_glyph_h = font_resource->pixel_per_glyph_h;
	font_asset->pixel_per_glyph_w = font_resource->pixel_per_glyph_w;
	font_asset->columns = font_resource->columns;
	font_asset->rows = font_resource->rows;
	SDL_bool load_result = fck_texture_load(renderer, font_resource->texture_path, &font_asset->texture);

	fck_file_free(&file_memory);

	CHECK_ERROR(load_result, SDL_GetError(), return false);

	return true;
}

int fck_print_directory(void *userdata, const char *dirname, const char *fname)
{
	const char *extension = SDL_strrchr(fname, '.');

	SDL_Log("%s - %s - %s", dirname, fname, extension);

	return 1;
}

int main(int, char **)
{
	fck_engine engine;
	SDL_zero(engine);

	// Init Systems
	CHECK_CRITICAL(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS), SDL_GetError());

	CHECK_CRITICAL(IMG_Init(IMG_INIT_PNG), SDL_GetError());

	CHECK_CRITICAL(SDLNet_Init() != -1, SDL_GetError())

	const int window_width = 640;
	const int window_height = 640;
	engine.window = SDL_CreateWindow("fck - engine", window_width, window_height, SDL_WINDOW_RESIZABLE);
	CHECK_CRITICAL(engine.window, SDL_GetError());

	engine.renderer = SDL_CreateRenderer(engine.window, nullptr);
	CHECK_CRITICAL(engine.renderer, SDL_GetError());

	CHECK_WARNING(SDL_SetRenderVSync(engine.renderer, true), SDL_GetError());

	// Init Application
	fck_ecs ecs;
	SDL_zero(ecs);

	fck_drop_file_context drop_file_context;
	SDL_zero(drop_file_context);

	fck_font_asset_load(engine.renderer, "special", &engine.default_editor_font);

	fck_spritesheet cammy_sprites;
	SDL_zero(cammy_sprites);
	CHECK_ERROR(fck_spritesheet_load(engine.renderer, "cammy.png", &cammy_sprites), SDL_GetError());

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

	engine.font_editor.selected_font_texture = cammy_sprites.texture;
	float w, h;
	CHECK_ERROR(SDL_GetTextureSize(engine.default_editor_font.texture, &w, &h), SDL_GetError());
	int offset_x = (window_width * 0.5f) - (w * 0.5f);
	int offset_y = (window_height * 0.5f) - (h * 0.5f);
	engine.font_editor.editor_pivot_x = offset_x;
	engine.font_editor.editor_pivot_y = offset_y;

	SDL_EnumerateDirectory(FCK_RESOURCE_DIRECTORY_PATH, fck_print_directory, nullptr);

	bool is_font_editor_open = false;

	bool is_running = true;
	int sprite_rect_index = 0;

	int cammy_idle_start = 24;
	int cammy_idle_count = 8;
	int cammy_idle_current = 24;
	Uint64 cammy_animation_accumulator = 0;
	Uint64 cammy_animation_frame_time = 120;

	Uint64 tp = SDL_GetTicks();

	while (is_running)
	{
		Uint64 now = SDL_GetTicks();
		Uint64 delta = now - tp;
		tp = now;

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

		SDL_SetRenderDrawColor(engine.renderer, 0, 0, 0, 255);
		SDL_RenderClear(engine.renderer);

		fck_layout layout = {2, FCK_LAYOUT_HORIZONTAL_ALIGNMENT_CENTRE, FCK_LAYOUT_VERTICAL_ALIGNMENT_CENTRE};

		// Render processing
		if (is_font_editor_open)
		{
			fck_font_editor_update(&engine);
		}
		else
		{
			cammy_animation_accumulator += delta;
			if (cammy_animation_accumulator > cammy_animation_frame_time)
			{
				cammy_animation_accumulator -= cammy_animation_frame_time;
				cammy_idle_current = cammy_idle_current + 1;
				if (cammy_idle_current - cammy_idle_start >= cammy_idle_count)
				{
					cammy_idle_current = cammy_idle_start;
				}
			}

			SDL_FRect source = cammy_sprites.rect_list.rects[cammy_idle_current];
			SDL_FRect dst = {0.0f, 128.0f, source.w * 2.0f, source.h * 2.0f};
			SDL_RenderTextureRotated(engine.renderer, cammy_sprites.texture, &source, &dst, 0.0f, nullptr,
			                         SDL_FLIP_HORIZONTAL);
		}

		SDL_RenderPresent(engine.renderer);
	}

	fck_font_editor_free(&engine.font_editor);

	fck_drop_file_context_free(&drop_file_context);

	fck_ecs_free(&ecs);

	SDL_DestroyRenderer(engine.renderer);
	SDL_DestroyWindow(engine.window);

	SDLNet_Quit();

	IMG_Quit();

	SDL_Quit();
	return 0;
}

int student_main(int, char **)
{
	CHECK_CRITICAL(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS), SDL_GetError());

	CHECK_CRITICAL(IMG_Init(IMG_INIT_PNG), SDL_GetError());

	SDL_Window *window = SDL_CreateWindow("Fck - Engine", 640, 640, 0);
	CHECK_CRITICAL(window != nullptr, SDL_GetError());

	SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);
	CHECK_CRITICAL(renderer != nullptr, SDL_GetError());

	SDL_bool input_previous[SDL_SCANCODE_COUNT];
	SDL_zero(input_previous);

	SDL_bool input_current[SDL_SCANCODE_COUNT];
	SDL_zero(input_current);

	SDL_Texture *player_texture = IMG_LoadTexture(renderer, FCK_RESOURCE_DIRECTORY_PATH "player.png");
	CHECK_CRITICAL(player_texture != nullptr, SDL_GetError());

	float x = 0;
	float y = 640 - 64.0f;

	Uint64 tp = SDL_GetTicks();

	float dash_cooldown_duration = 1.0f;
	float dash_cooldown_timer = 0.0f;

	bool is_running = true;
	while (is_running)
	{
		Uint64 now = SDL_GetTicks();
		Uint64 delta = now - tp;
		float delta_seconds = float(delta) / 1000.0f;

		tp = now;

		SDL_memcpy(input_previous, input_current, sizeof(input_current));

		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_EVENT_QUIT: {
				is_running = false;
				break;
			}
			case SDL_EVENT_KEY_DOWN:
			case SDL_EVENT_KEY_UP: {
				input_current[event.key.scancode] = event.type == SDL_EVENT_KEY_DOWN;
				break;
			}
			default:
				break;
			}
		}

		// Update
		float direction = 0.0f;
		if (input_current[SDL_SCANCODE_D])
		{
			direction = direction + 0.5f;
		}
		if (input_current[SDL_SCANCODE_A])
		{
			direction = direction - 0.5f;
		}
		x = x + direction;

		dash_cooldown_timer -= delta_seconds;
		dash_cooldown_timer = SDL_max(dash_cooldown_timer, 0.0f);
		if (dash_cooldown_timer <= 0.0f)
		{
			if (input_current[SDL_SCANCODE_SPACE] && !input_previous[SDL_SCANCODE_SPACE])
			{
				y = y - 64.0f;
				dash_cooldown_timer = dash_cooldown_duration;
			}
		}

		// Render

		// Clear the final image
		SDL_SetRenderDrawColor(renderer, 0, 0, 20, 255);
		SDL_RenderClear(renderer);

		// Construct the final
		SDL_SetRenderDrawColor(renderer, 50, 175, 20, 255);
		SDL_FRect src{0, 0, 48.0f, 48.0f};
		SDL_FRect dst{x, y, 48.0f, 48.0f};
		CHECK_ERROR(SDL_RenderTextureRotated(renderer, player_texture, &src, &dst, 0.0, nullptr, SDL_FLIP_VERTICAL),
		            SDL_GetError(), is_running = false);

		// Present final image
		SDL_RenderPresent(renderer);
	}

	SDL_Quit();
	return 0;
}
