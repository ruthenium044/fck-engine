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
#include "fck_spritesheet.h"
#include "fck_ui.h"

#include "fck_student_testbed.h"

// NOTE: CHANGE THIS TO USE THE CONTENT OF THE student_program.cpp source file!!!
#define FCK_STUDENT_MODE false

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

int fck_print_directory(void *userdata, const char *dirname, const char *fname)
{
	const char *extension = SDL_strrchr(fname, '.');

	SDL_PathInfo path_info;

	size_t path_count = SDL_strlen(dirname);
	size_t file_name_count = SDL_strlen(fname);
	size_t total_count = file_name_count + path_count + 1;
	char *path = (char *)SDL_malloc(total_count);
	path[0] = '\0';

	size_t last = 0;
	last = SDL_strlcat(path, dirname, total_count);
	last = SDL_strlcat(path, fname, total_count);

	SDL_Log("%s - %s - %s", dirname, fname, extension);
	if (SDL_GetPathInfo(path, &path_info))
	{
		if (path_info.type == SDL_PATHTYPE_DIRECTORY)
		{
			SDL_EnumerateDirectory(path, fck_print_directory, userdata);
		}
	}

	SDL_free(path);

	return 1;
}

enum fck_common_animations
{
	FCK_COMMON_ANIMATION_IDLE,
	FCK_COMMON_ANIMATION_CROUCH,
	FCK_COMMON_ANIMATION_JUMP_UP,
	FCK_COMMON_ANIMATION_JUMP_FORWARD,
	FCK_COMMON_ANIMATION_JUMP_BACKWARD,
	FCK_COMMON_ANIMATION_WALK_FORWARD,
	FCK_COMMON_ANIMATION_WALK_BACKWARD,
	FCK_COMMON_ANIMATION_PUNCH_A,
	FCK_COMMON_ANIMATION_PUNCH_B,
	FCK_COMMON_ANIMATION_KICK_A,
	FCK_COMMON_ANIMATION_KICK_B,
	FCK_COMMON_ANIMATION_COUNT
};

enum fck_input_type
{
	FCK_INPUT_TYPE_LEFT,
	FCK_INPUT_TYPE_RIGHT,
	FCK_INPUT_TYPE_UP,
	FCK_INPUT_TYPE_DOWN,
	FCK_INPUT_TYPE_MOVEMENT_END = FCK_INPUT_TYPE_DOWN + 1,
	FCK_INPUT_TYPE_PUNCH_A = FCK_INPUT_TYPE_MOVEMENT_END,
	FCK_INPUT_TYPE_PUNCH_B,
	FCK_INPUT_TYPE_KICK_A,
	FCK_INPUT_TYPE_KICK_B,
};

enum fck_input_flag
{
	FCK_INPUT_FLAG_ZERO = 0,
	FCK_INPUT_FLAG_LEFT = 1 << FCK_INPUT_TYPE_LEFT,
	FCK_INPUT_FLAG_RIGHT = 1 << FCK_INPUT_TYPE_RIGHT,
	FCK_INPUT_FLAG_UP = 1 << FCK_INPUT_TYPE_UP,
	FCK_INPUT_FLAG_DOWN = 1 << FCK_INPUT_TYPE_DOWN,

	FCK_INPUT_FLAG_MOVEMENT_END = 1 << FCK_INPUT_TYPE_MOVEMENT_END,

	FCK_INPUT_FLAG_PUNCH_A = 1 << FCK_INPUT_TYPE_PUNCH_A,
	FCK_INPUT_FLAG_PUNCH_B = 1 << FCK_INPUT_TYPE_PUNCH_B,
	FCK_INPUT_FLAG_KICK_A = 1 << FCK_INPUT_TYPE_KICK_A,
	FCK_INPUT_FLAG_KICK_B = 1 << FCK_INPUT_TYPE_KICK_B,
};

enum fck_animation_type
{
	FCK_ANIMATION_TYPE_LOOP,
	FCK_ANIMATION_TYPE_ONCE,
};

struct fck_animation
{
	fck_animation_type animation_type;
	fck_rect_list_view rect_view;
	uint64_t frame_time_ms;
	SDL_FPoint offset;
};

struct fck_animator
{
	fck_spritesheet *spritesheet;
	fck_animation animations[FCK_COMMON_ANIMATION_COUNT];

	fck_animation *active_oneshot;
	fck_animation *active_animation;

	uint64_t time_accumulator_ms;
	size_t current_frame;
};

void fck_animator_alloc(fck_animator *animator, fck_spritesheet *spritsheet)
{
	SDL_assert(spritsheet != nullptr);
	SDL_zerop(animator);

	// What a hard dependency
	animator->spritesheet = spritsheet;
}

void fck_animator_free(fck_animator *animator)
{
	SDL_assert(animator != nullptr);
	animator->spritesheet = nullptr;
}

void fck_animator_insert(fck_animator *animator, fck_common_animations anim, fck_animation_type animation_type, size_t start, size_t count,
                         uint64_t frame_time_ms, float offset_x, float offset_y)
{
	SDL_assert(animator != nullptr);
	SDL_assert(animator->spritesheet != nullptr && "Resource data for animator not set!");

	fck_animation *animation = &animator->animations[anim];
	SDL_assert(animation->rect_view.rect_list == nullptr && "Overwriting animation");

	fck_rect_list_view_create(&animator->spritesheet->rect_list, start, count, &animation->rect_view);
	animation->animation_type = animation_type;
	animation->frame_time_ms = frame_time_ms;
	animation->offset.x = offset_x;
	animation->offset.y = offset_y;
}

void fck_animator_set(fck_animator *animator, fck_common_animations anim)
{
	SDL_assert(animator != nullptr);

	fck_animation *next_animation = &animator->animations[anim];
	if (next_animation != animator->active_animation)
	{
		animator->active_animation = next_animation;
		animator->current_frame = 0;
		animator->time_accumulator_ms = 0;
	}
}

void fck_animator_play(fck_animator *animator, fck_common_animations anim)
{
	SDL_assert(animator != nullptr);

	if (animator->active_oneshot == nullptr)
	{
		fck_animation *next_animation = &animator->animations[anim];
		animator->active_oneshot = next_animation;
		animator->current_frame = 0;
		animator->time_accumulator_ms = 0;
	}
}

bool fck_animator_is_playing(fck_animator *animator)
{
	SDL_assert(animator != nullptr);

	return animator->active_oneshot != nullptr;
}

bool fck_animator_update(fck_animator *animator, uint64_t delta_ms)
{
	SDL_assert(animator != nullptr);

	if (animator->active_animation == nullptr)
	{
		return false;
	}

	if (animator->active_animation->rect_view.rect_list == nullptr)
	{
		return false;
	}

	animator->time_accumulator_ms += delta_ms;

	fck_animation *animation = animator->active_animation;

	bool is_oneshot_animation = animator->active_oneshot != nullptr;
	if (is_oneshot_animation)
	{
		animation = animator->active_oneshot;
	}

	while (animator->time_accumulator_ms > animation->frame_time_ms)
	{
		animator->time_accumulator_ms = animator->time_accumulator_ms - animation->frame_time_ms;
		animator->current_frame = animator->current_frame + 1;

		if (animator->current_frame >= animation->rect_view.count)
		{
			if (is_oneshot_animation)
			{
				animator->active_oneshot = nullptr;
				animator->current_frame = 0;
				return true;
			}

			switch (animation->animation_type)
			{
			case FCK_ANIMATION_TYPE_LOOP:
				animator->current_frame = 0;
				break;

			case FCK_ANIMATION_TYPE_ONCE:
				animator->current_frame = animation->rect_view.count - 1;
				break;
			}
		}
	}
	return true;
}

SDL_FRect const *fck_animator_get_rect(fck_animator *animator)
{
	SDL_assert(animator != nullptr);
	SDL_assert(animator->active_animation != nullptr);

	fck_animation *animation = animator->active_animation;
	if (animator->active_oneshot != nullptr)
	{
		animation = animator->active_oneshot;
	}
	fck_rect_list_view const *view = &animation->rect_view;
	return fck_rect_list_view_get(view, animator->current_frame);
}

void fck_animator_apply(fck_animator *animator, SDL_FRect *rect, float scale)
{
	SDL_assert(animator != nullptr);
	SDL_assert(animator->active_animation != nullptr);

	fck_animation *animation = animator->active_animation;
	if (animator->active_oneshot != nullptr)
	{
		animation = animator->active_oneshot;
	}
	rect->x = rect->x + (animation->offset.x * scale);
	rect->y = rect->y + (animation->offset.y * scale);
}

int main(int argc, char **argv)
{
#if FCK_STUDENT_MODE
	if (FCK_STUDENT_MODE)
	{
		return fck_run_student_testbed(argc, argv);
	}
#endif // FCK_STUDENT_MODE

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
	CHECK_ERROR(fck_spritesheet_load(engine.renderer, "cammy.png", &cammy_sprites, false), SDL_GetError());

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

	fck_animator animator;
	fck_animator_alloc(&animator, &cammy_sprites);

	fck_animator_insert(&animator, FCK_COMMON_ANIMATION_IDLE, FCK_ANIMATION_TYPE_LOOP, 24, 8, 60, 0.0f, 0.0f);
	fck_animator_insert(&animator, FCK_COMMON_ANIMATION_CROUCH, FCK_ANIMATION_TYPE_ONCE, 35, 3, 40, 0.0f, 0.0f);
	fck_animator_insert(&animator, FCK_COMMON_ANIMATION_JUMP_UP, FCK_ANIMATION_TYPE_ONCE, 58, 7, 120, 0.0f, -32.0f);
	fck_animator_insert(&animator, FCK_COMMON_ANIMATION_JUMP_FORWARD, FCK_ANIMATION_TYPE_ONCE, 52, 6, 120, 0.0f, -32.0f);
	fck_animator_insert(&animator, FCK_COMMON_ANIMATION_JUMP_BACKWARD, FCK_ANIMATION_TYPE_ONCE, 65, 6, 120, 0.0f, -32.0f);
	fck_animator_insert(&animator, FCK_COMMON_ANIMATION_WALK_FORWARD, FCK_ANIMATION_TYPE_LOOP, 84, 12, 40, -5.0f, -10.0f);
	fck_animator_insert(&animator, FCK_COMMON_ANIMATION_WALK_BACKWARD, FCK_ANIMATION_TYPE_LOOP, 96, 12, 40, -5.0f, -10.0f);
	fck_animator_insert(&animator, FCK_COMMON_ANIMATION_PUNCH_A, FCK_ANIMATION_TYPE_ONCE, 118, 3, 60, 0.0f, 0.0f);
	fck_animator_insert(&animator, FCK_COMMON_ANIMATION_PUNCH_B, FCK_ANIMATION_TYPE_ONCE, 121, 3, 60, 0.0f, 0.0f);
	fck_animator_insert(&animator, FCK_COMMON_ANIMATION_KICK_A, FCK_ANIMATION_TYPE_ONCE, 130, 5, 60, 0.0f, 0.0f);
	// Something is missing for this anim
	fck_animator_insert(&animator, FCK_COMMON_ANIMATION_KICK_B, FCK_ANIMATION_TYPE_ONCE, 145, 3, 120, -24.0f, -16.0f);

	fck_animator_set(&animator, FCK_COMMON_ANIMATION_IDLE);

	// Maybe global control later
	const float screen_scale = 2.0f;

	float px = 0.0;

	Uint64 tp = SDL_GetTicks();
	bool is_running = true;
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

		// INPUT
		int input_flag = 0;
		if (!fck_animator_is_playing(&animator))
		{
			if (fck_key_down(&engine.keyboard, SDL_SCANCODE_LEFT))
			{
				input_flag = input_flag | FCK_INPUT_FLAG_LEFT;
			}
			if (fck_key_down(&engine.keyboard, SDL_SCANCODE_RIGHT))
			{
				input_flag = input_flag | FCK_INPUT_FLAG_RIGHT;
			}
			if (fck_key_down(&engine.keyboard, SDL_SCANCODE_DOWN))
			{
				input_flag = input_flag | FCK_INPUT_FLAG_DOWN;
			}
			if (fck_key_down(&engine.keyboard, SDL_SCANCODE_UP))
			{
				input_flag = input_flag | FCK_INPUT_FLAG_UP;
			}
		}
		if (fck_key_just_down(&engine.keyboard, SDL_SCANCODE_A))
		{
			input_flag = input_flag | FCK_INPUT_FLAG_PUNCH_A;
		}
		if (fck_key_just_down(&engine.keyboard, SDL_SCANCODE_S))
		{
			input_flag = input_flag | FCK_INPUT_FLAG_PUNCH_B;
		}
		if (fck_key_just_down(&engine.keyboard, SDL_SCANCODE_X))
		{
			input_flag = input_flag | FCK_INPUT_FLAG_KICK_A;
		}
		if (fck_key_just_down(&engine.keyboard, SDL_SCANCODE_Z))
		{
			input_flag = input_flag | FCK_INPUT_FLAG_KICK_B;
		}

		// STATE EVAL
		// Movement
		if (input_flag < FCK_INPUT_FLAG_MOVEMENT_END)
		{
			if (input_flag == FCK_INPUT_FLAG_ZERO)
			{
				fck_animator_set(&animator, FCK_COMMON_ANIMATION_IDLE);
			}

			if ((input_flag & FCK_INPUT_FLAG_UP) == FCK_INPUT_FLAG_UP)
			{
				if ((input_flag & FCK_INPUT_FLAG_LEFT) == FCK_INPUT_FLAG_LEFT)
				{
					fck_animator_play(&animator, FCK_COMMON_ANIMATION_JUMP_BACKWARD);
				}
				else if ((input_flag & FCK_INPUT_FLAG_RIGHT) == FCK_INPUT_FLAG_RIGHT)
				{
					fck_animator_play(&animator, FCK_COMMON_ANIMATION_JUMP_FORWARD);
				}
				else
				{
					fck_animator_play(&animator, FCK_COMMON_ANIMATION_JUMP_UP);
				}
			}

			// Cround is dominant!
			else if ((input_flag & FCK_INPUT_FLAG_DOWN) == FCK_INPUT_FLAG_DOWN)
			{
				fck_animator_set(&animator, FCK_COMMON_ANIMATION_CROUCH);
			}
			else
			{
				if (input_flag == FCK_INPUT_FLAG_RIGHT)
				{
					fck_animator_set(&animator, FCK_COMMON_ANIMATION_WALK_FORWARD);
					px += 2.0f * screen_scale;
				}
				if (input_flag == FCK_INPUT_FLAG_LEFT)
				{
					fck_animator_set(&animator, FCK_COMMON_ANIMATION_WALK_BACKWARD);
					px -= 2.0f * screen_scale;
				}
			}
		}

		// Combat - Punch
		if ((input_flag & FCK_INPUT_FLAG_PUNCH_A) == FCK_INPUT_FLAG_PUNCH_A)
		{
			fck_animator_play(&animator, FCK_COMMON_ANIMATION_PUNCH_A);
		}
		if ((input_flag & FCK_INPUT_FLAG_PUNCH_B) == FCK_INPUT_FLAG_PUNCH_B)
		{
			fck_animator_play(&animator, FCK_COMMON_ANIMATION_PUNCH_B);
		}
		// Combat - Kick
		if ((input_flag & FCK_INPUT_FLAG_KICK_A) == FCK_INPUT_FLAG_KICK_A)
		{
			fck_animator_play(&animator, FCK_COMMON_ANIMATION_KICK_A);
		}
		if ((input_flag & FCK_INPUT_FLAG_KICK_B) == FCK_INPUT_FLAG_KICK_B)
		{
			fck_animator_play(&animator, FCK_COMMON_ANIMATION_KICK_B);
		}

		SDL_SetRenderDrawColor(engine.renderer, 0, 0, 0, 255);
		SDL_RenderClear(engine.renderer);

		// Render processing
		if (is_font_editor_open)
		{
			fck_font_editor_update(&engine);
		}
		else
		{
			fck_animator_update(&animator, delta);
			SDL_FRect const *source = fck_animator_get_rect(&animator);

			float target_x = px;
			float target_y = 128.0f;
			float target_width = source->w * screen_scale;
			float target_height = source->h * screen_scale;
			SDL_FRect dst = {target_x, target_y, target_width, target_height};
			fck_animator_apply(&animator, &dst, screen_scale);

			SDL_RenderTextureRotated(engine.renderer, cammy_sprites.texture, source, &dst, 0.0f, nullptr, SDL_FLIP_HORIZONTAL);

			SDL_SetRenderDrawColor(engine.renderer, 0, 255, 0, 255);
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