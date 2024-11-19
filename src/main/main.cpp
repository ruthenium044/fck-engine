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

fck_ecs::entity_type create_cammy(fck_ecs *ecs);

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
	fck_milliseconds current;
};

FCK_SERIALISE_OFF(fck_authority)
struct fck_authority
{
};

FCK_SERIALISE_OFF(fck_control_layout)
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

struct fck_controller
{
	fck_input_flag input;
};

struct fck_position
{
	float x;
	float y;
};

void fck_serialise(fck_serialiser *serialiser, fck_position *positions, size_t count)
{
	const size_t total_floats = count * 2;
	fck_serialise(serialiser, (float *)positions, total_floats);
}

FCK_SERIALISE_OFF(fck_future_position)
struct fck_future_position
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
};

void fck_engine_free(fck_engine *engine)
{
	SDL_assert(engine != nullptr);

	SDL_DestroyRenderer(engine->renderer);
	SDL_DestroyWindow(engine->window);

	SDL_zerop(engine);
}

enum fck_peer_state : uint8_t
{
	FCK_PEER_STATE_NONE,
	FCK_PEER_STATE_CREATING,
	FCK_PEER_STATE_OK
};

void fck_serialise(fck_serialiser *serialiser, fck_peer_state *value)
{
	fck_serialise(serialiser, (uint8_t *)value);
}

typedef uint16_t fck_peer_id;

struct fck_peer
{
	fck_peer_id peer_id;
	fck_ecs::entity_type avatar;
	fck_peer_state state;
};

void fck_serialise(fck_serialiser *serialiser, fck_peer *value, size_t count = 1)
{
	for (int i = 0; i < count; i++)
	{
		fck_serialise(serialiser, &value->peer_id);
		fck_serialise(serialiser, &value->avatar);
		fck_serialise(serialiser, &value->state);
	}
}

struct fck_peers
{
	constexpr static fck_peer_id max_peers = 32;
	constexpr static fck_peer_id invalid_peer = ~0;

	fck_peer_id self;
	fck_peer_id host;

	fck_sparse_list<fck_peer_id, fck_peer> peers;

	fck_sparse_lookup<cnt_connection_id, fck_peer_id> connection_to_peer;
};

void fck_peers_alloc(fck_peers *peers)
{
	SDL_assert(peers != nullptr);
	SDL_zerop(peers);

	peers->self = fck_peers::invalid_peer;
	peers->host = fck_peers::invalid_peer;

	fck_sparse_list_alloc(&peers->peers, fck_peers::max_peers);
	fck_sparse_lookup_alloc(&peers->connection_to_peer, 64, fck_peers::invalid_peer);
}

void fck_peers_clear(fck_peers *peers)
{
	SDL_assert(peers != nullptr);

	peers->self = fck_peers::invalid_peer;
	peers->host = fck_peers::invalid_peer;

	fck_sparse_list_clear(&peers->peers);
	fck_sparse_lookup_clear(&peers->connection_to_peer);
}

void fck_peers_free(fck_peers *peers)
{
	SDL_assert(peers != nullptr);

	peers->self = fck_peers::invalid_peer;
	peers->host = fck_peers::invalid_peer;

	fck_sparse_list_free(&peers->peers);
	fck_sparse_lookup_free(&peers->connection_to_peer);
}

bool fck_peers_try_add(fck_peers *peers, cnt_connection_handle const *connection_handle, fck_ecs::entity_type *avatar)
{
	SDL_assert(peers != nullptr);
	SDL_assert(avatar != nullptr);

	fck_peer peer;
	SDL_zero(peer);

	peer.state = FCK_PEER_STATE_NONE;
	peer.peer_id = fck_sparse_list_add(&peers->peers, &peer);
	if (!fck_sparse_list_exists(&peers->peers, peer.peer_id))
	{
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Cannot add peer - No space left. TODO: Implement disconnect functionality");
		return false;
	}

	if (connection_handle != nullptr)
	{
		fck_sparse_lookup_set(&peers->connection_to_peer, connection_handle->id, &peer.peer_id);
	}
	peer.avatar = *avatar;
	// When we succeed we can pass it into the view
	fck_peer *private_peer = fck_sparse_list_view(&peers->peers, peer.peer_id);
	*private_peer = peer;
	return true;
}

bool fck_peers_try_get_peer_from_connection(fck_peers *peers, cnt_connection_handle const *connection, fck_peer *out_peer)
{
	SDL_assert(peers != nullptr);
	SDL_assert(out_peer != nullptr);
	if (connection == nullptr)
	{
		return false;
	}
	fck_peer_id peer_id;
	bool exists = fck_sparse_lookup_try_get(&peers->connection_to_peer, connection->id, &peer_id);
	if (exists && peer_id != fck_peers::invalid_peer)
	{
		*out_peer = *fck_sparse_list_view(&peers->peers, peer_id);
		return true;
	}

	return false;
}

void fck_peers_set_self(fck_peers *peers, fck_peer_id id)
{
	SDL_assert(peers != nullptr);

	peers->self = id;
}

void fck_peers_set_host(fck_peers *peers, fck_peer_id id)
{
	SDL_assert(peers != nullptr);

	peers->host = id;
}

bool fck_peers_try_view_peer_from_entity(fck_peers *peers, fck_ecs::entity_type entity, fck_peer **out_peer)
{
	SDL_assert(peers != nullptr);
	SDL_assert(out_peer != nullptr);

	for (fck_peer *peer : &peers->peers.dense)
	{
		if (peer->avatar == entity)
		{
			*out_peer = peer;
			return true;
		}
	}

	return false;
}

fck_peer *fck_peers_view_peer_from_index(fck_peers *peers, fck_peer_id index)
{
	SDL_assert(peers != nullptr);

	return fck_sparse_list_view(&peers->peers, index);
}

bool fck_peers_is_hosting(fck_peers *peers)
{
	SDL_assert(peers != nullptr);

	return peers->self == peers->host && peers->self != fck_peers::invalid_peer;
}

bool fck_peers_is_ok(fck_peers *peers)
{
	SDL_assert(peers != nullptr);

	if (peers->self == fck_peers::invalid_peer)
	{
		return false;
	}

	fck_peer *peer = fck_peers_view_peer_from_index(peers, peers->self);
	if (peer != nullptr)
	{
		return peer->state == FCK_PEER_STATE_OK;
	}
	return false;
}

void fck_peers_emplace(fck_peers *peers, fck_peer *peer, cnt_connection_handle const *connection_handle)
{
	SDL_assert(peers != nullptr);
	SDL_assert(peer != nullptr);

	if (connection_handle != nullptr)
	{
		fck_sparse_lookup_set(&peers->connection_to_peer, connection_handle->id, &peer->peer_id);
	}
	fck_sparse_list_emplace(&peers->peers, peer->peer_id, peer);
}

struct fck_instance_info
{
	char const *title;

	char const *ip;
	uint16_t source_port;
	uint16_t destination_port;
};

void input_process(fck_ecs *ecs, fck_system_update_info *)
{
	fck_keyboard_state *keyboard = fck_ecs_unique_view<fck_keyboard_state>(ecs);

	// TODO: fck_controller_layout*, fck_controller
	// This would enable multiplayer - for now, whatever
	fck_ecs_apply(ecs, [ecs, keyboard](fck_authority *, fck_control_layout *layout, fck_controller *controller, fck_animator *animator) {
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
		if (fck_key_just_down(keyboard, SDL_SCANCODE_SPACE))
		{
			fck_ecs::entity_type cammy = create_cammy(ecs);
			fck_ecs_component_create<fck_authority>(ecs, cammy);
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
	fck_spritesheet *spritesheet = fck_ecs_unique_view<fck_spritesheet>(ecs);

	SDL_SetRenderDrawColor(engine->renderer, 0, 0, 0, 255);
	SDL_RenderClear(engine->renderer);

	fck_ecs_apply(ecs, [engine, spritesheet](fck_static_sprite *sprite, fck_position *position) {
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

	fck_ecs_apply(ecs, [engine, time, spritesheet](fck_animator *animator, fck_position *position) {
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
	fck_instance_info *info = fck_ecs_unique_view<fck_instance_info>(ecs);

	fck_engine *engine = fck_ecs_unique_view<fck_engine>(ecs);

	const int window_width = 640;
	const int window_height = 640;
	engine->window = SDL_CreateWindow(info->title, window_width, window_height, SDL_WINDOW_RESIZABLE);
	CHECK_CRITICAL(engine->window, SDL_GetError());

	engine->renderer = SDL_CreateRenderer(engine->window, SDL_SOFTWARE_RENDERER);
	CHECK_CRITICAL(engine->renderer, SDL_GetError());

	CHECK_WARNING(SDL_SetRenderVSync(engine->renderer, true), SDL_GetError());

	fck_font_asset_load(engine->renderer, "special", &engine->default_editor_font);

	fck_drop_file_context *drop_file_context = fck_ecs_unique_create<fck_drop_file_context>(ecs, fck_drop_file_context_free);

	fck_keyboard_state *keyboard = fck_ecs_unique_create<fck_keyboard_state>(ecs);
	fck_mouse_state *mouse = fck_ecs_unique_create<fck_mouse_state>(ecs);

	fck_time *time = fck_ecs_unique_create<fck_time>(ecs);

	fck_drop_file_context_allocate(drop_file_context, 16);
	fck_drop_file_context_push(drop_file_context, fck_drop_file_receive_png);

	SDL_EnumerateDirectory(FCK_RESOURCE_DIRECTORY_PATH, fck_print_directory, nullptr);

	fck_spritesheet *spritesheet = fck_ecs_unique_create<fck_spritesheet>(ecs, fck_free);
	CHECK_ERROR(fck_spritesheet_load(engine->renderer, "cammy.png", spritesheet, false), SDL_GetError());

	engine->is_running = true;
}

fck_ecs::entity_type create_cammy(fck_ecs *ecs)
{
	fck_engine *engine = fck_ecs_unique_view<fck_engine>(ecs);
	fck_spritesheet *spritesheet = fck_ecs_unique_view<fck_spritesheet>(ecs);

	fck_ecs::entity_type cammy = fck_ecs_entity_create(ecs);

	fck_animator *animator = fck_ecs_component_create<fck_animator>(ecs, cammy);
	fck_position *position = fck_ecs_component_create<fck_position>(ecs, cammy);

	fck_controller *controller = fck_ecs_component_create<fck_controller>(ecs, cammy);

	// We just offset the y position based on the entity ID. Good enough
	position->x = 0.0f;
	position->y = 128.0f + (cammy * 128);

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

	return cammy;
}

void local_cammy_setup(fck_ecs *ecs, fck_system_once_info *)
{
	fck_ecs::entity_type cammy = create_cammy(ecs);

	// These are not getting replicated
	fck_control_layout *layout = fck_ecs_component_create<fck_control_layout>(ecs, cammy);
	*layout = {SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, SDL_SCANCODE_UP, SDL_SCANCODE_DOWN};

	// This one is not supposed to get repio
	fck_ecs_component_create<fck_authority>(ecs, cammy);

	fck_peers *peers = fck_ecs_unique_view<fck_peers>(ecs);

	if (fck_peers_try_add(peers, nullptr, &cammy))
	{
		fck_peer *peer;
		if (fck_peers_try_view_peer_from_entity(peers, cammy, &peer))
		{
			peer->state = FCK_PEER_STATE_OK;
			fck_peers_set_host(peers, peer->peer_id);
			fck_peers_set_self(peers, peer->peer_id);
		}
		SDL_Log("Created Avatar: %d", cammy);
	}
}

void cammy_setup_2(fck_ecs *ecs, fck_system_once_info *)
{
	fck_engine *engine = fck_ecs_unique_view<fck_engine>(ecs);
	fck_ecs::entity_type cammy = fck_ecs_entity_create(ecs);

	fck_animator *animator = fck_ecs_component_create<fck_animator>(ecs, cammy);
	fck_position *position = fck_ecs_component_create<fck_position>(ecs, cammy);
	fck_spritesheet *spritesheet = fck_ecs_component_create<fck_spritesheet>(ecs, cammy);
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

enum fck_network_communication_mode : uint32_t
{
	FCK_NETWORK_COMMUNICATION_MODE_MESSAGE_UNICAST,
	FCK_NETWORK_COMMUNICATION_MODE_REPLICATION_BROADCAST
};

void fck_serialise(fck_serialiser *serialiser, fck_network_communication_mode *value)
{
	fck_serialise(serialiser, (uint32_t *)value);
}

enum fck_network_segment_type : uint32_t
{
	FCK_NETWORK_SEGMENT_TYPE_ECS = 0xEC5,
	FCK_NETWORK_SEGMENT_TYPE_PEERS = 0xBEEF,
	FCK_NETWORK_SEGMENT_TYPE_EOF = 0xFFFF
};

void fck_serialise(fck_serialiser *serialiser, fck_network_segment_type *value)
{
	fck_serialise(serialiser, (uint32_t *)value);
}

struct fck_communication_state
{
	bool should_send;
};

struct fck_avatar_assignment_message
{
	uint64_t id;

	fck_peer_id self;
	fck_peer_id host;

	fck_peer peer;
};

void fck_serialise(fck_serialiser *serialiser, fck_avatar_assignment_message *value)
{
	fck_serialise(serialiser, &value->id);
	fck_serialise(serialiser, &value->peer);
}

void network_process_recv_message_unicast(fck_ecs *ecs, fck_serialiser *serialiser, cnt_connection_handle const *connection)
{
	fck_peers *peers = fck_ecs_unique_view<fck_peers>(ecs);

	fck_avatar_assignment_message message;

	fck_serialise(serialiser, &message);
	SDL_assert(message.id == 0xB055);

	fck_control_layout *layout = fck_ecs_component_create<fck_control_layout>(ecs, message.peer.avatar);
	*layout = {SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, SDL_SCANCODE_UP, SDL_SCANCODE_DOWN};
	fck_ecs_component_create<fck_authority>(ecs, message.peer.avatar);

	message.peer.state = FCK_PEER_STATE_OK;

	fck_peers_set_host(peers, message.host);
	fck_peers_set_self(peers, message.peer.peer_id);

	fck_peers_emplace(peers, &message.peer, connection);
}

void network_process_recv_replication_broadcast(fck_ecs *ecs, fck_serialiser *serialiser, cnt_connection_handle const *connection)
{
	fck_peers *peers = fck_ecs_unique_view<fck_peers>(ecs);
	cnt_session *session = fck_ecs_unique_view<cnt_session>(ecs);

	// Peer info
	fck_network_segment_type peers_segment;
	fck_serialise(serialiser, &peers_segment);
	SDL_assert(peers_segment == FCK_NETWORK_SEGMENT_TYPE_PEERS);

	fck_peer_id peer_count = 0;
	fck_serialise(serialiser, &peer_count);
	for (fck_peer_id index = 0; index < peer_count; index++)
	{
		fck_peer avatar = {0};
		fck_serialise(serialiser, &avatar);

		fck_peer *peer = fck_peers_view_peer_from_index(peers, index);
		if (peer != nullptr)
		{
			// We can only BUILD up peer state, never go back.
			// If a peer reached a specific state, it can only remain in there
			avatar.state = SDL_max(avatar.state, peer->state);
		}
		fck_peers_emplace(peers, &avatar, nullptr);
	}

	fck_peer peer;
	if (fck_peers_try_get_peer_from_connection(peers, connection, &peer))
	{
		// Receive from... client
		if (peer.state == FCK_PEER_STATE_OK)
		{
			// ECS snapshot
			fck_network_segment_type ecs_segment;
			fck_serialise(serialiser, &ecs_segment);
			SDL_assert(ecs_segment == FCK_NETWORK_SEGMENT_TYPE_ECS);

			if (fck_peers_is_hosting(peers))
			{
				// Receive from client - Partial is ok
				fck_ecs_snapshot_load_partial(ecs, serialiser);
			}
			else
			{
				// Receive from host - Always FULL state
				// TODO: Implement a backbuffer ECS so we have a reliable storage
				fck_serialiser temp;
				fck_serialiser_alloc(&temp);
				fck_serialiser_byte_writer(&temp.self);

				fck_ecs::sparse_array<fck_authority> *entities = fck_ecs_view_single<fck_authority>(ecs);
				fck_ecs_snapshot_store_partial(ecs, &temp, &entities->owner);

				fck_ecs_snapshot_load(ecs, serialiser);
				fck_serialiser_reset(&temp);

				fck_serialiser_byte_reader(&temp.self);
				fck_ecs_snapshot_load_partial(ecs, &temp);

				fck_serialiser_free(&temp);
			}
		}
	}
}

void networking_process_recv(fck_ecs *ecs)
{
	cnt_session *session = fck_ecs_unique_view<cnt_session>(ecs);

	cnt_memory_view memory_view;
	while (cnt_session_try_receive_from(session, &memory_view))
	{
		// Setup
		fck_serialiser serialiser;
		fck_serialiser_create(&serialiser, memory_view.data, memory_view.length);
		fck_serialiser_byte_reader(&serialiser.self);

		// Header
		fck_network_communication_mode mode;
		fck_serialise(&serialiser, &mode);

		// This shit can happen in ANY order
		// FUCK ME
		switch (mode)
		{
		case FCK_NETWORK_COMMUNICATION_MODE_MESSAGE_UNICAST:
			network_process_recv_message_unicast(ecs, &serialiser, &memory_view.info.connection);
			break;
		case FCK_NETWORK_COMMUNICATION_MODE_REPLICATION_BROADCAST:
			network_process_recv_replication_broadcast(ecs, &serialiser, &memory_view.info.connection);
			break;
		default:
			SDL_assert(false && "Great message buddy. The communication mode we received is unknown lol");
			break;
		}
	}
}

void networking_process_send(fck_ecs *ecs)
{
	cnt_session *session = fck_ecs_unique_view<cnt_session>(ecs);
	fck_peers *peers = fck_ecs_unique_view<fck_peers>(ecs);

	// Setup
	fck_serialiser serialiser;
	fck_serialiser_alloc(&serialiser);
	fck_serialiser_byte_writer(&serialiser.self);

	// Header
	fck_network_communication_mode mode = FCK_NETWORK_COMMUNICATION_MODE_REPLICATION_BROADCAST;
	fck_serialise(&serialiser, &mode);

	// Peer info
	fck_network_segment_type peers_segment = FCK_NETWORK_SEGMENT_TYPE_PEERS;
	fck_serialise(&serialiser, &peers_segment);

	fck_peer_id peer_count = peers->peers.dense.count;
	fck_serialise(&serialiser, &peer_count);
	for (fck_item<fck_peer_id, fck_peer> item : &peers->peers)
	{
		SDL_assert(*item.index == item.value->peer_id);
		fck_serialise(&serialiser, item.value);
	}

	// ECS snapshot
	// Send out
	if (fck_peers_is_ok(peers))
	{
		fck_network_segment_type ecs_segment = FCK_NETWORK_SEGMENT_TYPE_ECS;
		fck_serialise(&serialiser, &ecs_segment);

		if (fck_peers_is_hosting(peers))
		{
			// Send to client - ALWAYS full state
			fck_ecs_snapshot_store(ecs, &serialiser);
		}
		else
		{
			// Send to host - Partial is ok
			fck_ecs::sparse_array<fck_authority> *entities = fck_ecs_view_single<fck_authority>(ecs);
			fck_ecs_snapshot_store_partial(ecs, &serialiser, &entities->owner);
		}
	}

	cnt_session_broadcast(session, serialiser.data, serialiser.at);

	fck_serialiser_free(&serialiser);
}

void networking_process_setup_new_connections(fck_ecs *ecs)
{
	cnt_session *session = fck_ecs_unique_view<cnt_session>(ecs);
	fck_peers *peers = fck_ecs_unique_view<fck_peers>(ecs);

	cnt_connection_handle handle;
	cnt_connection connection;
	while (cnt_session_try_dequeue_new_connection(session, &handle, &connection))
	{
		// The requester is never responsible, the one accepting is responsible!
		bool is_hosting = (connection.flags & CNT_CONNECTION_FLAG_CLIENT) != CNT_CONNECTION_FLAG_CLIENT;
		if (is_hosting)
		{
			fck_ecs::entity_type avatar = create_cammy(ecs);
			if (fck_peers_try_add(peers, &handle, &avatar))
			{
				uint8_t buffer[32];
				fck_serialiser serialiser;
				fck_serialiser_create(&serialiser, buffer, sizeof(buffer));
				fck_serialiser_byte_writer(&serialiser.self);

				// Header
				fck_network_communication_mode mode = FCK_NETWORK_COMMUNICATION_MODE_MESSAGE_UNICAST;
				fck_serialise(&serialiser, &mode);

				// Message
				fck_avatar_assignment_message message;
				message.id = 0xB055;

				fck_peer *peer;
				bool peer_exists = fck_peers_try_view_peer_from_entity(peers, avatar, &peer);
				SDL_assert(peer_exists &&
				           "Oh, this error would really suck. Peer and avatar tuple does not exist, RIGHT AFTER ADDING (??)");

				message.peer = *peer;
				fck_serialise(&serialiser, &message);

				cnt_session_send(session, &handle, serialiser.data, serialiser.at);
				SDL_Log("Created Avatar: %d", avatar);
			}
		}
		else
		{
			// Do we need these? ...
			fck_ecs_entity_destroy_all(ecs);
			fck_peers_clear(peers);
		}
	}
}

void networking_process(fck_ecs *ecs, fck_system_update_info *)
{
	cnt_session *session = fck_ecs_unique_view<cnt_session>(ecs);
	fck_time *time = fck_ecs_unique_view<fck_time>(ecs);

	networking_process_recv(ecs);

	bool will_tick_this_frame = cnt_session_will_tick(session, time->delta);
	if (will_tick_this_frame)
	{
		networking_process_send(ecs);
	}

	cnt_session_tick(session, time->current, time->delta);

	if (will_tick_this_frame)
	{
		// Technically on a connection (from tick) we will first process networking events
		// AND THEN we will send the on connect setup message to the player
		// Man... I suck
		networking_process_setup_new_connections(ecs);
	}
}

void networking_setup(fck_ecs *ecs, fck_system_once_info *)
{
	fck_communication_state *communication_state = fck_ecs_unique_create<fck_communication_state>(ecs);
	fck_instance_info *info = fck_ecs_unique_view<fck_instance_info>(ecs);

	cnt_session *session = fck_ecs_unique_create<cnt_session>(ecs, cnt_session_free);
	fck_peers *peers = fck_ecs_unique_create<fck_peers>(ecs, fck_peers_free);

	fck_peers_alloc(peers);

	static bool should_send = false;
	communication_state->should_send = should_send;
	should_send = !should_send;

	constexpr size_t tick_rate = 1000 / 60;
	cnt_session_alloc(session, 4, 128, 64, tick_rate);
	cnt_socket_handle socket_handle = cnt_session_socket_create(session, info->ip, info->source_port);
	if (info->destination_port != 0)
	{
		cnt_address_handle address_handle = cnt_session_address_create(session, info->ip, info->destination_port);
		cnt_session_connect(session, &socket_handle, &address_handle);
		// fck_ecs_component_clear<fck_control_layout>(ecs);
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
	fck_ecs_system_add(&instance->ecs, local_cammy_setup);

	// Good old fasioned update systems
	fck_ecs_system_add(&instance->ecs, input_process);
	fck_ecs_system_add(&instance->ecs, gameplay_process);
	fck_ecs_system_add(&instance->ecs, animations_process);
	fck_ecs_system_add(&instance->ecs, render_process);

	// We place the engine inside of the ECS as a unique - this way anything that
	// can access the ecs, can also access the engine. Ergo, we intigrate the
	// engine as part of the ECS workflow
	instance->engine = fck_ecs_unique_create<fck_engine>(&instance->ecs, fck_engine_free);
	// We flush the once systems since they might be relevant for startup
	// Adding once system during once system might file - We should enable that
	// If we queue a once system during a once system, what should happen?

	instance->info = fck_ecs_unique_set<fck_instance_info>(&instance->ecs, info);

	fck_ecs_flush_system_once(&instance->ecs);
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
			// Can we come up with a cool little quit way? IDK :-D
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

	constexpr char const *titles[instance_count] = {"fck engine - client", "fck engine - server"};

	constexpr uint16_t source_ports[instance_count] = {42069, 42072};
	constexpr uint16_t destination_ports[instance_count] = {42072, 0};

	for (uint16_t index = 0; index < instance_count; index++)
	{
		// Entry point to inject something custom
		fck_instance_info info;
		info.title = titles[index];
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
