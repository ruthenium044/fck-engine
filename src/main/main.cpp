// SDL core - functionality such as creating a window and getting events
#include <SDL3/SDL.h>

// Vulkan
#include <vulkan/vulkan.h>

// SDL image - Loads images... Many kinds. We only care about PNG
#include <SDL3_image/SDL_image.h>

// SDL net - networking... More later
// #include <SDL3_net/SDL_net.h>
// We use our own net - TODO: Port to windows
#include "net/cnt_session.h"

#include "fck_checks.h"
#include "fck_drop_file.h"
#include "fck_keyboard.h"
#include "fck_memory_stream.h"
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

using fck_milliseconds = uint64_t;

struct fck_time
{
	fck_milliseconds delta;
	// TODO: make it per-instance so each instance has its own instance time
	fck_milliseconds current;
};

struct fck_control_layout
{
	SDL_Scancode left;
	SDL_Scancode right;
	SDL_Scancode up;
	SDL_Scancode down;

	SDL_Scancode light_punch;
	SDL_Scancode hard_punch;
	SDL_Scancode light_kick;
	SDL_Scancode hard_kick;
};

struct fck_gamepad_layout
{
	SDL_Scancode left;
	SDL_Scancode right;
	SDL_Scancode up;
	SDL_Scancode down;

	SDL_Scancode light_punch;
	SDL_Scancode hard_punch;
	SDL_Scancode light_kick;
	SDL_Scancode hard_kick;
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

// void fck_serialise(fck_serialiser *serialiser, fck_position *positions, size_t count)
//{
// }

struct fck_engine
{
	static constexpr float screen_scale = 2.0f;

	SDL_Window *window;
	SDL_Renderer *renderer;

	fck_font_asset default_editor_font;

	bool is_running;
};

void fck_engine_free(fck_engine *engine)
{
	SDL_assert(engine != nullptr);

	SDL_DestroyRenderer(engine->renderer);
	SDL_DestroyWindow(engine->window);

	SDL_zerop(engine);
}

struct fck_instance_info
{
	char const *ip;
	uint16_t source_port;
	uint16_t destination_port;
};

void input_process(fck_ecs *ecs, fck_system_update_info *)
{
	fck_keyboard_state *keyboard = fck_ecs_unique_view<fck_keyboard_state>(ecs);

	// TODO: fck_controller_layout*, fck_controller
	// This would enable multiplayer - for now, whatever
	fck_ecs_apply(ecs, [keyboard](fck_control_layout *layout, fck_controller *controller, fck_animator *animator) {
		int input_flag = 0;
		if (!fck_animator_is_playing(animator))
		{
			if (fck_key_down(keyboard, layout->left))
			{
				input_flag = input_flag | FCK_INPUT_FLAG_LEFT;
			}
			if (fck_key_down(keyboard, layout->right))
			{
				input_flag = input_flag | FCK_INPUT_FLAG_RIGHT;
			}
			if (fck_key_down(keyboard, layout->down))
			{
				input_flag = input_flag | FCK_INPUT_FLAG_DOWN;
			}
			if (fck_key_down(keyboard, layout->up))
			{
				input_flag = input_flag | FCK_INPUT_FLAG_UP;
			}
		}
		if (fck_key_just_down(keyboard, layout->light_punch))
		{
			input_flag = input_flag | FCK_INPUT_FLAG_PUNCH_A;
		}
		if (fck_key_just_down(keyboard, layout->hard_punch))
		{
			input_flag = input_flag | FCK_INPUT_FLAG_PUNCH_B;
		}
		if (fck_key_just_down(keyboard, layout->light_kick))
		{
			input_flag = input_flag | FCK_INPUT_FLAG_KICK_A;
		}
		if (fck_key_just_down(keyboard, layout->hard_kick))
		{
			input_flag = input_flag | FCK_INPUT_FLAG_KICK_B;
		}
		controller->input = (fck_input_flag)input_flag;
	});
}

void gameplay_process(fck_ecs *ecs, fck_system_update_info *)
{
	cnt_session *session = fck_ecs_unique_view<cnt_session>(ecs);
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
	fck_time *time = fck_ecs_unique_view<fck_time>(ecs);

	SDL_SetRenderDrawColor(engine->renderer, 0, 0, 0, 255);
	SDL_RenderClear(engine->renderer);

	fck_ecs_apply(ecs, [engine](fck_static_sprite *sprite, fck_spritesheet *spritesheet, fck_position *position) {
		SDL_FRect const *source = spritesheet->rect_list.data + sprite->sprite_index;

		float target_x = position->x;
		float target_y = position->y;
		float target_width = source->w * fck_engine::screen_scale;
		float target_height = source->h * fck_engine::screen_scale;
		SDL_FRect dst = {target_x, target_y, target_width, target_height};

		dst.x = dst.x + (sprite->offset.x * fck_engine::screen_scale);
		dst.y = dst.y + (sprite->offset.y * fck_engine::screen_scale);

		SDL_RenderTextureRotated(engine->renderer, spritesheet->texture, source, &dst, 0.0f, nullptr, SDL_FLIP_HORIZONTAL);
	});

	fck_ecs_apply(ecs, [engine, time](fck_animator *animator, fck_spritesheet *spritesheet, fck_position *position) {
		if (fck_animator_update(animator, time->delta))
		{
			SDL_FRect const *source = fck_animator_get_rect(animator, spritesheet);

			float target_x = position->x;
			float target_y = position->y;
			float target_width = source->w * fck_engine::screen_scale;
			float target_height = source->h * fck_engine::screen_scale;
			SDL_FRect dst = {target_x, target_y, target_width, target_height};
			fck_animator_apply(animator, &dst, fck_engine::screen_scale);

			SDL_RenderTextureRotated(engine->renderer, spritesheet->texture, source, &dst, 0.0f, nullptr, SDL_FLIP_HORIZONTAL);
		}
	});

	SDL_RenderPresent(engine->renderer);
}

void engine_setup(fck_ecs *ecs, fck_system_once_info *)
{
	fck_engine *engine = fck_ecs_unique_view<fck_engine>(ecs);

	const int window_width = 640;
	const int window_height = 640;
	engine->window = SDL_CreateWindow("fck - engine", window_width, window_height, SDL_WINDOW_RESIZABLE);
	CHECK_CRITICAL(engine->window, SDL_GetError());

	engine->renderer = SDL_CreateRenderer(engine->window, nullptr);
	CHECK_CRITICAL(engine->renderer, SDL_GetError());

	CHECK_WARNING(SDL_SetRenderVSync(engine->renderer, true), SDL_GetError());

	fck_font_asset_load(engine->renderer, "special", &engine->default_editor_font);

	fck_drop_file_context *drop_file_context = fck_ecs_unique_set_empty<fck_drop_file_context>(ecs, fck_drop_file_context_free);

	fck_keyboard_state *keyboard = fck_ecs_unique_set_empty<fck_keyboard_state>(ecs);
	fck_mouse_state *mouse = fck_ecs_unique_set_empty<fck_mouse_state>(ecs);

	fck_time *time = fck_ecs_unique_set_empty<fck_time>(ecs);

	fck_drop_file_context_allocate(drop_file_context, 16);
	fck_drop_file_context_push(drop_file_context, fck_drop_file_receive_png);

	SDL_EnumerateDirectory(FCK_RESOURCE_DIRECTORY_PATH, fck_print_directory, nullptr);

	engine->is_running = true;
}

void cammy_setup(fck_ecs *ecs, fck_system_once_info *)
{
	fck_engine *engine = fck_ecs_unique_view<fck_engine>(ecs);
	fck_ecs::entity_type cammy = fck_ecs_entity_create(ecs);

	static fck_control_layout control_layout[2] = {
		{SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, SDL_SCANCODE_UP, SDL_SCANCODE_DOWN},
		{SDL_SCANCODE_A, SDL_SCANCODE_D, SDL_SCANCODE_W, SDL_SCANCODE_S},
	};

	fck_control_layout *layout = fck_ecs_component_set_empty<fck_control_layout>(ecs, cammy);
	fck_controller *controller = fck_ecs_component_set_empty<fck_controller>(ecs, cammy);
	fck_animator *animator = fck_ecs_component_set_empty<fck_animator>(ecs, cammy);
	fck_position *position = fck_ecs_component_set_empty<fck_position>(ecs, cammy);
	fck_spritesheet *spritesheet = fck_ecs_component_set_empty<fck_spritesheet>(ecs, cammy);
	CHECK_ERROR(fck_spritesheet_load(engine->renderer, "cammy.png", spritesheet, false), SDL_GetError());
	position->x = 0.0f;
	position->y = 128.0f + (cammy * 64);

	*layout = control_layout[cammy];

	fck_animator_alloc(animator, spritesheet);

	fck_animator_insert(animator, spritesheet, FCK_COMMON_ANIMATION_IDLE, FCK_ANIMATION_TYPE_LOOP, 24, 8, 60, 0.0f, 0.0f);
	fck_animator_insert(animator, spritesheet, FCK_COMMON_ANIMATION_CROUCH, FCK_ANIMATION_TYPE_ONCE, 35, 3, 40, 0.0f, 0.0f);
	fck_animator_insert(animator, spritesheet, FCK_COMMON_ANIMATION_JUMP_UP, FCK_ANIMATION_TYPE_ONCE, 58, 7, 120, 0.0f, -32.0f);
	fck_animator_insert(animator, spritesheet, FCK_COMMON_ANIMATION_JUMP_FORWARD, FCK_ANIMATION_TYPE_ONCE, 52, 6, 120, 0.0f, -32.0f);
	fck_animator_insert(animator, spritesheet, FCK_COMMON_ANIMATION_JUMP_BACKWARD, FCK_ANIMATION_TYPE_ONCE, 65, 6, 120, 0.0f, -32.0f);
	fck_animator_insert(animator, spritesheet, FCK_COMMON_ANIMATION_WALK_FORWARD, FCK_ANIMATION_TYPE_LOOP, 84, 12, 40, -5.0f, -10.0f);
	fck_animator_insert(animator, spritesheet, FCK_COMMON_ANIMATION_WALK_BACKWARD, FCK_ANIMATION_TYPE_LOOP, 96, 12, 40, -5.0f, -10.0f);
	fck_animator_insert(animator, spritesheet, FCK_COMMON_ANIMATION_PUNCH_A, FCK_ANIMATION_TYPE_ONCE, 118, 3, 60, 0.0f, 0.0f);
	fck_animator_insert(animator, spritesheet, FCK_COMMON_ANIMATION_PUNCH_B, FCK_ANIMATION_TYPE_ONCE, 121, 3, 60, 0.0f, 0.0f);
	fck_animator_insert(animator, spritesheet, FCK_COMMON_ANIMATION_KICK_A, FCK_ANIMATION_TYPE_ONCE, 130, 5, 60, 0.0f, 0.0f);
	// Something is missing for this anim
	fck_animator_insert(animator, spritesheet, FCK_COMMON_ANIMATION_KICK_B, FCK_ANIMATION_TYPE_ONCE, 145, 3, 120, -24.0f, -16.0f);
	fck_animator_set(animator, FCK_COMMON_ANIMATION_IDLE);

	// fck_ecs_component_remove<fck_controller>(ecs, cammy);
}

void cammy_setup_2(fck_ecs *ecs, fck_system_once_info *)
{
	fck_engine *engine = fck_ecs_unique_view<fck_engine>(ecs);
	fck_ecs::entity_type cammy = fck_ecs_entity_create(ecs);

	fck_animator *animator = fck_ecs_component_set_empty<fck_animator>(ecs, cammy);
	fck_position *position = fck_ecs_component_set_empty<fck_position>(ecs, cammy);
	fck_spritesheet *spritesheet = fck_ecs_component_set_empty<fck_spritesheet>(ecs, cammy);
	CHECK_ERROR(fck_spritesheet_load(engine->renderer, "cammy.png", spritesheet, false), SDL_GetError());
	position->x = 0.0f;
	position->y = 128.0f + (cammy * 32);

	fck_animator_alloc(animator, spritesheet);

	fck_animator_insert(animator, spritesheet, FCK_COMMON_ANIMATION_IDLE, FCK_ANIMATION_TYPE_LOOP, 24, 8, 60, 0.0f, 0.0f);
	fck_animator_insert(animator, spritesheet, FCK_COMMON_ANIMATION_CROUCH, FCK_ANIMATION_TYPE_ONCE, 35, 3, 40, 0.0f, 0.0f);
	fck_animator_insert(animator, spritesheet, FCK_COMMON_ANIMATION_JUMP_UP, FCK_ANIMATION_TYPE_ONCE, 58, 7, 120, 0.0f, -32.0f);
	fck_animator_insert(animator, spritesheet, FCK_COMMON_ANIMATION_JUMP_FORWARD, FCK_ANIMATION_TYPE_ONCE, 52, 6, 120, 0.0f, -32.0f);
	fck_animator_insert(animator, spritesheet, FCK_COMMON_ANIMATION_JUMP_BACKWARD, FCK_ANIMATION_TYPE_ONCE, 65, 6, 120, 0.0f, -32.0f);
	fck_animator_insert(animator, spritesheet, FCK_COMMON_ANIMATION_WALK_FORWARD, FCK_ANIMATION_TYPE_LOOP, 84, 12, 40, -5.0f, -10.0f);
	fck_animator_insert(animator, spritesheet, FCK_COMMON_ANIMATION_WALK_BACKWARD, FCK_ANIMATION_TYPE_LOOP, 96, 12, 40, -5.0f, -10.0f);
	fck_animator_insert(animator, spritesheet, FCK_COMMON_ANIMATION_PUNCH_A, FCK_ANIMATION_TYPE_ONCE, 118, 3, 60, 0.0f, 0.0f);
	fck_animator_insert(animator, spritesheet, FCK_COMMON_ANIMATION_PUNCH_B, FCK_ANIMATION_TYPE_ONCE, 121, 3, 60, 0.0f, 0.0f);
	fck_animator_insert(animator, spritesheet, FCK_COMMON_ANIMATION_KICK_A, FCK_ANIMATION_TYPE_ONCE, 130, 5, 60, 0.0f, 0.0f);
	// Something is missing for this anim
	fck_animator_insert(animator, spritesheet, FCK_COMMON_ANIMATION_KICK_B, FCK_ANIMATION_TYPE_ONCE, 145, 3, 120, -24.0f, -16.0f);
	fck_animator_set(animator, FCK_COMMON_ANIMATION_IDLE);

	// fck_ecs_component_remove<fck_controller>(ecs, cammy);
}

struct fck_stream_writer
{
	fck_memory_stream *stream;
};

struct fck_stream_reader
{
	fck_memory_stream *stream;
};

template <typename type>
void fck_serialize(fck_stream_writer *writer, type *data)
{
	fck_memory_stream_write(writer->stream, data, sizeof(*data));
}

template <typename type>
void fck_serialize(fck_stream_reader *reader, type *data)
{
	data = (type *)fck_memory_stream_read(reader->stream, sizeof(*data));
}

enum network_payload_type
{
	NETWORK_PAYLOAD_TYPE_DEFAULT,
	NETWORK_PAYLOAD_TYPE_SYNC_REQUEST,
	NETWORK_PAYLOAD_TYPE_SYNC_RESPONSE,
};

void networking_process(fck_ecs *ecs, fck_system_update_info *)
{
	cnt_session *session = fck_ecs_unique_view<cnt_session>(ecs);
	fck_time *time = fck_ecs_unique_view<fck_time>(ecs);

	/*
	cnt_connection connection;
	while (cnt_session_try_dequeue_new_connection(session, &connection))
	{
	    SDL_Log("New connection: %d %s", connection.flags, cnt_address_to_string(session, connection.destination));
	    if ((connection.flags & CNT_CONNECTION_FLAG_INITIATOR) == CNT_CONNECTION_FLAG_INITIATOR)
	    {
	        uint8_t payload_type = NETWORK_PAYLOAD_TYPE_SYNC_REQUEST;
	        fck_ecs::entity_list *outgoing_entities = &ecs->entities.owner;
	        cnt_session_send_to_all(session, &payload_type, sizeof(payload_type));
	        cnt_session_send_to_all(session, &outgoing_entities->count, sizeof(outgoing_entities->count));
	        cnt_session_send_to_all(session, outgoing_entities->data, outgoing_entities->count * (sizeof(*outgoing_entities->data)));
	    }
	}

	cnt_memory_view view;
	if (cnt_session_try_receive_from(session, &view))
	{
	    uint8_t payload_type = *(uint8_t *)view.data;
	    if (cnt_session_try_receive_from(session, &view))
	    {
	        fck_ecs::entity_type count = *(fck_ecs::entity_type *)view.data;
	        if (cnt_session_try_receive_from(session, &view))
	        {
	            fck_ecs::entity_list incoming_entities;
	            incoming_entities.data = (fck_ecs::entity_type *)view.data;
	            incoming_entities.count = count;
	            incoming_entities.capacity = count;

	            if (payload_type == NETWORK_PAYLOAD_TYPE_SYNC_REQUEST)
	            {
	                for (fck_ecs::entity_type *entity : &incoming_entities)
	                {
	                    // fck_ecs_entity_create(ecs);
	                    cammy_setup_2(ecs, {});
	                }
	            }
	        }
	    }
	}*/

	if (cnt_session_will_tick(session, time->delta))
	{
	}

	cnt_session_tick(session, time->current, time->delta);
}

void networking_setup(fck_ecs *ecs, fck_system_once_info *)
{
	fck_instance_info *info = fck_ecs_unique_view<fck_instance_info>(ecs);
	cnt_session *session = fck_ecs_unique_set_empty<cnt_session>(ecs, cnt_session_free);

	constexpr size_t tick_rate = 1000 / 120; // 120 tick server, baby. Esports ready
	cnt_session_alloc(session, 4, 128, 64, tick_rate);
	cnt_socket_handle socket_handle = cnt_session_socket_create(session, info->ip, info->source_port);
	if (info->destination_port != 0)
	{
		cnt_address_handle address_handle = cnt_session_address_create(session, info->ip, info->destination_port);
		cnt_session_connect(session, &socket_handle, &address_handle);
	}

	fck_ecs_system_add(ecs, networking_process);
}

struct fck_instance
{
	fck_ecs ecs;
	fck_engine *engine;
	fck_instance_info *info;
};

void fck_instance_alloc(fck_instance *instance, fck_instance_info const *info)
{
	SDL_assert(instance != nullptr);

	fck_ecs_alloc_info ecs_alloc_info = {256, 128, 64};
	fck_ecs_alloc(&instance->ecs, &ecs_alloc_info);

	// Good old fashioned init systems
	fck_ecs_system_add(&instance->ecs, engine_setup);
	fck_ecs_system_add(&instance->ecs, networking_setup);
	fck_ecs_system_add(&instance->ecs, cammy_setup);

	// Good old fasioned update systems
	fck_ecs_system_add(&instance->ecs, input_process);
	fck_ecs_system_add(&instance->ecs, gameplay_process);
	fck_ecs_system_add(&instance->ecs, animations_process);
	fck_ecs_system_add(&instance->ecs, render_process);

	// We place the engine inside of the ECS as a unique - this way anything that
	// can access the ecs, can also access the engine. Ergo, we intigrate the
	// engine as part of the ECS workflow
	instance->engine = fck_ecs_unique_set_empty<fck_engine>(&instance->ecs, fck_engine_free);
	// We flush the once systems since they might be relevant for startup
	// Adding once system during once system might file - We should enable that
	// If we queue a once system during a once system, what should happen?

	instance->info = fck_ecs_unique_set<fck_instance_info>(&instance->ecs, info);

	fck_ecs_flush_system_once(&instance->ecs);

	fck_ecs_snapshot snapshot;
	fck_ecs_snapshot_store(&instance->ecs, &snapshot);
	fck_ecs_snapshot_load(&instance->ecs, &snapshot);
	fck_ecs_snapshot_free(&snapshot);
}

void fck_instance_free(fck_instance *instance)
{
	SDL_assert(instance != nullptr);

	// We ignore free-ing memory for now
	// Since the ECS exists in this scope and we pass it along, we are pretty much
	// guaranteed that the OS will clean it up
	// I mean the ECS quite literally collects all the garbage in the application since it takes ownership
	// and then it decides to free... it just so happens it's at the very end
	fck_ecs_free(&instance->ecs);

	SDL_zerop(instance);
}

struct fck_instances
{
	fck_sparse_array<uint8_t, fck_instance> data;
	// If anything breaks, make sure to come up with a better mapping
	fck_dense_list<uint8_t, SDL_WindowID> pending_destroyed;
};

fck_iterator<fck_instance> begin(fck_instances *instances)
{
	SDL_assert(instances != nullptr);

	return begin(&instances->data.dense);
}

fck_iterator<fck_instance> end(fck_instances *instances)
{
	SDL_assert(instances != nullptr);

	return end(&instances->data.dense);
}

void fck_instances_alloc(fck_instances *instances, uint8_t capacity)
{
	SDL_assert(instances != nullptr);
	fck_sparse_array_alloc(&instances->data, capacity);
	fck_dense_list_alloc(&instances->pending_destroyed, capacity);
}

void fck_instances_free(fck_instances *instances)
{
	fck_dense_list_free(&instances->pending_destroyed);
	SDL_assert(instances != nullptr);
	for (fck_instance *instance : instances)
	{
		fck_instance_free(instance);
	}
	fck_sparse_array_free(&instances->data);
}

bool fck_instances_any_active(fck_instances *instances)
{
	SDL_assert(instances != nullptr);

	bool is_any_instance_running = false;
	for (fck_instance *instance : instances)
	{
		is_any_instance_running |= instance->engine->is_running;
	}
	return is_any_instance_running;
}

fck_instance *fck_instances_add(fck_instances *instances, fck_instance_info const *info)
{
	SDL_assert(instances != nullptr);

	fck_instance instance;
	fck_instance_alloc(&instance, info);
	SDL_WindowID id = SDL_GetWindowID(instance.engine->window);
	return fck_sparse_array_emplace(&instances->data, id, &instance);
}

fck_instance *fck_instances_view(fck_instances *instances, SDL_WindowID const *windowId)
{
	SDL_assert(instances != nullptr);
	return fck_sparse_array_view(&instances->data, *windowId);
}

void fck_instances_remove(fck_instances *instances, SDL_WindowID const *windowId)
{
	SDL_assert(instances != nullptr);

	SDL_WindowID id = *windowId;
	fck_instance *instance = fck_instances_view(instances, &id);
	// Hide the window before destroying it so MacOS deals with it better
	CHECK_WARNING(SDL_HideWindow(instance->engine->window), SDL_GetError());

	fck_instance_free(instance);
	fck_sparse_array_remove(&instances->data, id);
}

void fck_instances_process_events(fck_instances *instances)
{
	float scroll_delta_x = 0.0f;
	float scroll_delta_y = 0.0f;

	// We defer killing instances by about one tick
	// Ok, it's exactly one tick. This makes sure we do not kill it during event polling
	for (SDL_WindowID *id : &instances->pending_destroyed)
	{
		fck_instances_remove(instances, id);
	}
	fck_dense_list_clear(&instances->pending_destroyed);
	SDL_Event ev;
	while (SDL_PollEvent(&ev))
	{
		switch (ev.type)
		{
		case SDL_EventType::SDL_EVENT_QUIT:
			// engine->is_running = false;
			break;
		case SDL_EventType::SDL_EVENT_WINDOW_CLOSE_REQUESTED: {
			fck_dense_list_add(&instances->pending_destroyed, &ev.window.windowID);
		}
		break;
		case SDL_EventType::SDL_EVENT_DROP_FILE: {
			fck_instance *instance = fck_instances_view(instances, &ev.drop.windowID);
			fck_drop_file_context *drop_file_context = fck_ecs_unique_view<fck_drop_file_context>(&instance->ecs);
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

	for (fck_instance *instance : instances)
	{
		// Maybe we should cache these modules, maybe access is fast enough, maybe, maybe
		fck_ecs *ecs = &instance->ecs;
		fck_engine *engine = fck_ecs_unique_view<fck_engine>(ecs);
		fck_keyboard_state *keyboard = fck_ecs_unique_view<fck_keyboard_state>(ecs);
		fck_mouse_state *mouse = fck_ecs_unique_view<fck_mouse_state>(ecs);
		SDL_WindowFlags window_flags = SDL_GetWindowFlags(engine->window);

		if ((window_flags & SDL_WINDOW_INPUT_FOCUS) == SDL_WINDOW_INPUT_FOCUS)
		{
			fck_keyboard_state_update(keyboard);
			fck_mouse_state_update(mouse, scroll_delta_x, scroll_delta_y);
		}
		else
		{
			fck_keyboard_state_update_empty(keyboard);
			fck_mouse_state_update_empty(mouse);
		}
	}
}

int main(int argc, char **argv)
{
#if FCK_STUDENT_MODE
	if (FCK_STUDENT_MODE)
	{
		return fck_run_student_testbed(argc, argv);
	}
#endif // FCK_STUDENT_MODE
	CHECK_CRITICAL(SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS), SDL_GetError());
	CHECK_CRITICAL(IMG_Init(IMG_INIT_PNG), SDL_GetError());

	fck_instances instances;
	fck_instances_alloc(&instances, 8);

	constexpr uint16_t instance_count = 2;
	constexpr uint16_t source_ports[instance_count] = {42069, 42072};
	constexpr uint16_t destination_ports[instance_count] = {42072, 0};

	for (uint16_t index = 0; index < instance_count; index++)
	{
		// Entry point to inject something custom
		fck_instance_info info;
		info.ip = "127.0.0.1";
		info.source_port = source_ports[index];
		info.destination_port = destination_ports[index];
		fck_instance *instance = fck_instances_add(&instances, &info);
	}

	fck_milliseconds tp = SDL_GetTicks();
	while (fck_instances_any_active(&instances))
	{
		// Maybe global control later
		fck_milliseconds now = SDL_GetTicks();
		fck_milliseconds delta_time = now - tp;
		tp = now;

		for (fck_instance *instance : &instances)
		{
			fck_time *time = fck_ecs_unique_view<fck_time>(&instance->ecs);
			time->current = tp;
			time->delta = delta_time;
		}

		fck_instances_process_events(&instances);

		for (fck_instance *instance : &instances)
		{
			fck_ecs_tick(&instance->ecs);
		}
	}

	fck_instances_free(&instances);

	SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
	SDL_Quit();

	return 0;
}