// SDL core - functionality such as creating a window and getting events
#include <SDL3/SDL.h>

// Vulkan
#include <vulkan/vulkan.h>

// SDL image - Loads images... Many kinds. We only care about PNG
#include <SDL3_image/SDL_image.h>

// SDL net - networking... More later
#include <SDL3_net/SDL_net.h>

#include "fck_checks.h"
#include "fck_drop_file.h"
#include "fck_keyboard.h"
#include "fck_mouse.h"
#include "fck_spritesheet.h"
#include "fck_ui.h"
#include <ecs/fck_ecs.h>
#include <fck_animator.h>

#include "fck_student_testbed.h"

// NOTE: CHANGE THIS TO USE THE CONTENT OF THE student_program.cpp source
// file!!!
#define FCK_STUDENT_MODE false

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

struct fck_controller
{
	fck_input_flag input;
};

struct fck_position
{
	float x;
	float y;
};

struct fck_engine
{
	static constexpr float screen_scale = 2.0f;

	SDL_Window *window;
	SDL_Renderer *renderer;

	fck_font_asset default_editor_font;
	bool is_running;
	uint64_t delta_time;
};

void event_process(fck_ecs *ecs, fck_system_update_info *)
{
	fck_engine *engine = fck_ecs_unique_view<fck_engine>(ecs);
	fck_keyboard_state *keyboard = fck_ecs_unique_view<fck_keyboard_state>(ecs);
	fck_mouse_state *mouse = fck_ecs_unique_view<fck_mouse_state>(ecs);

	float scroll_delta_x = 0.0f;
	float scroll_delta_y = 0.0f;

	SDL_Event ev;
	while (SDL_PollEvent(&ev))
	{
		switch (ev.type)
		{
		case SDL_EventType::SDL_EVENT_QUIT:
			engine->is_running = false;
			break;
		case SDL_EventType::SDL_EVENT_DROP_FILE: {
			fck_drop_file_context *drop_file_context = fck_ecs_unique_view<fck_drop_file_context>(ecs);
			fck_drop_file_context_notify(drop_file_context, &ev.drop);
		}
		break;
		case SDL_EventType::SDL_EVENT_MOUSE_WHEEL:
			scroll_delta_x = scroll_delta_x + ev.wheel.x;
			scroll_delta_y = scroll_delta_y + ev.wheel.y;
			break;
		default:
			break;
		}
	}

	fck_keyboard_state_update(keyboard);
	fck_mouse_state_update(mouse, scroll_delta_x, scroll_delta_y);
}

void input_process(fck_ecs *ecs, fck_system_update_info *)
{
	fck_keyboard_state *keyboard = fck_ecs_unique_view<fck_keyboard_state>(ecs);

	// TODO: fck_controller_layout*, fck_controller
	// This would enable multiplayer - for now, whatever
	fck_ecs_apply(ecs, [keyboard](fck_controller *controller, fck_animator *animator) {
		int input_flag = 0;
		if (!fck_animator_is_playing(animator))
		{
			if (fck_key_down(keyboard, SDL_SCANCODE_LEFT))
			{
				input_flag = input_flag | FCK_INPUT_FLAG_LEFT;
			}
			if (fck_key_down(keyboard, SDL_SCANCODE_RIGHT))
			{
				input_flag = input_flag | FCK_INPUT_FLAG_RIGHT;
			}
			if (fck_key_down(keyboard, SDL_SCANCODE_DOWN))
			{
				input_flag = input_flag | FCK_INPUT_FLAG_DOWN;
			}
			if (fck_key_down(keyboard, SDL_SCANCODE_UP))
			{
				input_flag = input_flag | FCK_INPUT_FLAG_UP;
			}
		}
		if (fck_key_just_down(keyboard, SDL_SCANCODE_A))
		{
			input_flag = input_flag | FCK_INPUT_FLAG_PUNCH_A;
		}
		if (fck_key_just_down(keyboard, SDL_SCANCODE_S))
		{
			input_flag = input_flag | FCK_INPUT_FLAG_PUNCH_B;
		}
		if (fck_key_just_down(keyboard, SDL_SCANCODE_X))
		{
			input_flag = input_flag | FCK_INPUT_FLAG_KICK_A;
		}
		if (fck_key_just_down(keyboard, SDL_SCANCODE_Z))
		{
			input_flag = input_flag | FCK_INPUT_FLAG_KICK_B;
		}
		controller->input = (fck_input_flag)input_flag;
	});
}

void gameplay_process(fck_ecs *ecs, fck_system_update_info *)
{
	fck_engine *engine = fck_ecs_unique_view<fck_engine>(ecs);

	// STATE EVAL
	// Movement
	fck_ecs_apply(ecs, [](fck_controller *controller, fck_position *position) {
		fck_input_flag input_flag = controller->input;
		if (input_flag < FCK_INPUT_FLAG_MOVEMENT_END)
		{
			float direction = 0.0f;
			if (input_flag == FCK_INPUT_FLAG_RIGHT)
			{
				direction = direction + fck_engine::screen_scale * 2.0f;
			}
			if (input_flag == FCK_INPUT_FLAG_LEFT)
			{
				direction = direction - fck_engine::screen_scale * 2.0f;
			}
			position->x = position->x + direction;
		}
	});
}

void animations_process(fck_ecs *ecs, fck_system_update_info *)
{
	fck_engine *engine = fck_ecs_unique_view<fck_engine>(ecs);

	// STATE EVAL
	// Movement
	fck_ecs_apply(ecs, [](fck_controller *controller, fck_animator *animator) {
		fck_input_flag input_flag = controller->input;
		if (input_flag < FCK_INPUT_FLAG_MOVEMENT_END)
		{
			if (input_flag == FCK_INPUT_FLAG_ZERO)
			{
				fck_animator_set(animator, FCK_COMMON_ANIMATION_IDLE);
			}

			if ((input_flag & FCK_INPUT_FLAG_UP) == FCK_INPUT_FLAG_UP)
			{
				if ((input_flag & FCK_INPUT_FLAG_LEFT) == FCK_INPUT_FLAG_LEFT)
				{
					fck_animator_play(animator, FCK_COMMON_ANIMATION_JUMP_BACKWARD);
				}
				else if ((input_flag & FCK_INPUT_FLAG_RIGHT) == FCK_INPUT_FLAG_RIGHT)
				{
					fck_animator_play(animator, FCK_COMMON_ANIMATION_JUMP_FORWARD);
				}
				else
				{
					fck_animator_play(animator, FCK_COMMON_ANIMATION_JUMP_UP);
				}
			}
			else if ((input_flag & FCK_INPUT_FLAG_DOWN) == FCK_INPUT_FLAG_DOWN)
			{
				fck_animator_set(animator, FCK_COMMON_ANIMATION_CROUCH);
			}
			else
			{
				if (input_flag == FCK_INPUT_FLAG_RIGHT)
				{
					fck_animator_set(animator, FCK_COMMON_ANIMATION_WALK_FORWARD);
				}
				if (input_flag == FCK_INPUT_FLAG_LEFT)
				{
					fck_animator_set(animator, FCK_COMMON_ANIMATION_WALK_BACKWARD);
				}
			}
		}

		// Combat - Punch
		if ((input_flag & FCK_INPUT_FLAG_PUNCH_A) == FCK_INPUT_FLAG_PUNCH_A)
		{
			fck_animator_play(animator, FCK_COMMON_ANIMATION_PUNCH_A);
		}
		if ((input_flag & FCK_INPUT_FLAG_PUNCH_B) == FCK_INPUT_FLAG_PUNCH_B)
		{
			fck_animator_play(animator, FCK_COMMON_ANIMATION_PUNCH_B);
		}
		// Combat - Kick
		if ((input_flag & FCK_INPUT_FLAG_KICK_A) == FCK_INPUT_FLAG_KICK_A)
		{
			fck_animator_play(animator, FCK_COMMON_ANIMATION_KICK_A);
		}
		if ((input_flag & FCK_INPUT_FLAG_KICK_B) == FCK_INPUT_FLAG_KICK_B)
		{
			fck_animator_play(animator, FCK_COMMON_ANIMATION_KICK_B);
		}
	});
}

void render_process(fck_ecs *ecs, fck_system_update_info *)
{
	fck_engine *engine = fck_ecs_unique_view<fck_engine>(ecs);

	SDL_SetRenderDrawColor(engine->renderer, 0, 0, 0, 255);
	SDL_RenderClear(engine->renderer);

	fck_ecs_apply(ecs, [engine](fck_animator *animator, fck_spritesheet *spritesheet, fck_position *position) {
		fck_animator_update(animator, engine->delta_time);
		SDL_FRect const *source = fck_animator_get_rect(animator);

		float target_x = position->x;
		float target_y = position->y;
		float target_width = source->w * fck_engine::screen_scale;
		float target_height = source->h * fck_engine::screen_scale;
		SDL_FRect dst = {target_x, target_y, target_width, target_height};
		fck_animator_apply(animator, &dst, fck_engine::screen_scale);

		SDL_RenderTextureRotated(engine->renderer, spritesheet->texture, source, &dst, 0.0f, nullptr, SDL_FLIP_HORIZONTAL);
	});

	SDL_RenderPresent(engine->renderer);
}

void setup_engine(fck_ecs *ecs, fck_system_once_info *)
{
	fck_engine *engine = fck_ecs_unique_view<fck_engine>(ecs);

	CHECK_CRITICAL(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS), SDL_GetError());

	CHECK_CRITICAL(IMG_Init(IMG_INIT_PNG), SDL_GetError());

	CHECK_CRITICAL(SDLNet_Init() != -1, SDL_GetError())

	const int window_width = 640;
	const int window_height = 640;
	engine->window = SDL_CreateWindow("fck - engine", window_width, window_height, SDL_WINDOW_RESIZABLE);
	CHECK_CRITICAL(engine->window, SDL_GetError());

	engine->renderer = SDL_CreateRenderer(engine->window, nullptr);
	CHECK_CRITICAL(engine->renderer, SDL_GetError());

	CHECK_WARNING(SDL_SetRenderVSync(engine->renderer, true), SDL_GetError());

	fck_font_asset_load(engine->renderer, "special", &engine->default_editor_font);

	fck_drop_file_context *drop_file_context = fck_ecs_unique_set_empty<fck_drop_file_context>(ecs);

	fck_keyboard_state *keyboard = fck_ecs_unique_set_empty<fck_keyboard_state>(ecs);
	fck_mouse_state *mouse = fck_ecs_unique_set_empty<fck_mouse_state>(ecs);

	fck_drop_file_context_allocate(drop_file_context, 16);
	fck_drop_file_context_push(drop_file_context, fck_drop_file_receive_png);

	SDL_EnumerateDirectory(FCK_RESOURCE_DIRECTORY_PATH, fck_print_directory, nullptr);

	engine->is_running = true;
}

void setup_cammy(fck_ecs *ecs, fck_system_once_info *)
{
	fck_engine *engine = fck_ecs_unique_view<fck_engine>(ecs);

	fck_ecs::entity_type cammy = fck_ecs_entity_create(ecs);

	fck_controller *controller = fck_ecs_component_set_empty<fck_controller>(ecs, cammy);
	fck_spritesheet *spritesheet = fck_ecs_component_set_empty<fck_spritesheet>(ecs, cammy);
	fck_animator *animator = fck_ecs_component_set_empty<fck_animator>(ecs, cammy);
	fck_position *position = fck_ecs_component_set_empty<fck_position>(ecs, cammy);
	position->x = 0.0f;
	position->y = 128.0f;

	CHECK_ERROR(fck_spritesheet_load(engine->renderer, "cammy.png", spritesheet, false), SDL_GetError());

	fck_animator_alloc(animator, spritesheet);

	fck_animator_insert(animator, FCK_COMMON_ANIMATION_IDLE, FCK_ANIMATION_TYPE_LOOP, 24, 8, 60, 0.0f, 0.0f);
	fck_animator_insert(animator, FCK_COMMON_ANIMATION_CROUCH, FCK_ANIMATION_TYPE_ONCE, 35, 3, 40, 0.0f, 0.0f);
	fck_animator_insert(animator, FCK_COMMON_ANIMATION_JUMP_UP, FCK_ANIMATION_TYPE_ONCE, 58, 7, 120, 0.0f, -32.0f);
	fck_animator_insert(animator, FCK_COMMON_ANIMATION_JUMP_FORWARD, FCK_ANIMATION_TYPE_ONCE, 52, 6, 120, 0.0f, -32.0f);
	fck_animator_insert(animator, FCK_COMMON_ANIMATION_JUMP_BACKWARD, FCK_ANIMATION_TYPE_ONCE, 65, 6, 120, 0.0f, -32.0f);
	fck_animator_insert(animator, FCK_COMMON_ANIMATION_WALK_FORWARD, FCK_ANIMATION_TYPE_LOOP, 84, 12, 40, -5.0f, -10.0f);
	fck_animator_insert(animator, FCK_COMMON_ANIMATION_WALK_BACKWARD, FCK_ANIMATION_TYPE_LOOP, 96, 12, 40, -5.0f, -10.0f);
	fck_animator_insert(animator, FCK_COMMON_ANIMATION_PUNCH_A, FCK_ANIMATION_TYPE_ONCE, 118, 3, 60, 0.0f, 0.0f);
	fck_animator_insert(animator, FCK_COMMON_ANIMATION_PUNCH_B, FCK_ANIMATION_TYPE_ONCE, 121, 3, 60, 0.0f, 0.0f);
	fck_animator_insert(animator, FCK_COMMON_ANIMATION_KICK_A, FCK_ANIMATION_TYPE_ONCE, 130, 5, 60, 0.0f, 0.0f);
	// Something is missing for this anim
	fck_animator_insert(animator, FCK_COMMON_ANIMATION_KICK_B, FCK_ANIMATION_TYPE_ONCE, 145, 3, 120, -24.0f, -16.0f);
	fck_animator_set(animator, FCK_COMMON_ANIMATION_IDLE);
}

int main(int argc, char **argv)
{
#if FCK_STUDENT_MODE
	if (FCK_STUDENT_MODE)
	{
		return fck_run_student_testbed(argc, argv);
	}
#endif // FCK_STUDENT_MODE

	fck_ecs ecs;
	fck_ecs_alloc_info ecs_alloc_info = {256, 128, 64};
	fck_ecs_alloc(&ecs, &ecs_alloc_info);

	// Good old fashioned init systems
	fck_ecs_system_add(&ecs, setup_engine);
	fck_ecs_system_add(&ecs, setup_cammy);

	// Good old fasioned update systems
	fck_ecs_system_add(&ecs, event_process);
	fck_ecs_system_add(&ecs, input_process);
	fck_ecs_system_add(&ecs, gameplay_process);
	fck_ecs_system_add(&ecs, animations_process);

	fck_ecs_system_add(&ecs, render_process);

	// We place the engine inside of the ECS as a unique - this way anything that
	// can access the ecs, can also access the engine. Ergo, we intigrate the
	// engine as part of the ECS workflow
	fck_engine *engine = fck_ecs_unique_set_empty<fck_engine>(&ecs);

	// We flush the once systems since they might be relevant for startup
	// Adding once system during once system might file - We should enable that
	// If we queue a once system during a once system, what should happen?
	fck_ecs_flush_system_once(&ecs);

	Uint64 tp = SDL_GetTicks();
	while (engine->is_running)
	{
		// Maybe global control later
		Uint64 now = SDL_GetTicks();
		engine->delta_time = now - tp;
		tp = now;

		fck_ecs_tick(&ecs);
	}

	// We ignore free-ing memory for now
	// Since the ECS exists in this scope and we pass it along, we are pretty much
	// guaranteed that the OS will clean it up
	SDL_DestroyRenderer(engine->renderer);
	SDL_DestroyWindow(engine->window);

	SDLNet_Quit();

	IMG_Quit();

	SDL_Quit();
	return 0;
}