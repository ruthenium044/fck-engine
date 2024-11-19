// SDL core - functionality such as creating a window and getting events
#include <SDL3/SDL.h>

// Vulkan
#include <vulkan/vulkan.h>

// SDL image - Loads images... Many kinds. We only care about PNG
#include <SDL3_image/SDL_image.h>

// Networking
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

#include "core/fck_engine.h"
#include "core/fck_instance.h"
#include "core/fck_instances.h"
#include "net/cnt_peers.h"

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

	cnt_peers *peers = fck_ecs_unique_view<cnt_peers>(ecs);

	cnt_peer *peer;
	if (cnt_peers_try_add(peers, nullptr, &peer))
	{
		peer->state = cnt_peer_STATE_OK;
		cnt_peers_set_host(peers, peer->peer_id);
		cnt_peers_set_self(peers, peer->peer_id);
		SDL_Log("Created Avatar: %d", cammy);
	}
}

enum cnt_networking_communication_mode : uint32_t
{
	FCK_NETWORK_COMMUNICATION_MODE_MESSAGE_UNICAST,
	FCK_NETWORK_COMMUNICATION_MODE_REPLICATION_BROADCAST
};

void fck_serialise(fck_serialiser *serialiser, cnt_networking_communication_mode *value)
{
	fck_serialise(serialiser, (uint32_t *)value);
}

enum cnt_networking_segment_type : uint32_t
{
	FCK_NETWORK_SEGMENT_TYPE_ECS = 0xEC5,
	FCK_NETWORK_SEGMENT_TYPE_PEERS = 0xBEEF,
	FCK_NETWORK_SEGMENT_TYPE_EOF = 0xFFFF
};

void fck_serialise(fck_serialiser *serialiser, cnt_networking_segment_type *value)
{
	fck_serialise(serialiser, (uint32_t *)value);
}

struct cnt_welcome_message
{
	uint64_t id;

	cnt_peer_id self;
	cnt_peer_id host;

	fck_ecs::entity_type avatar;

	cnt_peer peer;
};

void fck_serialise(fck_serialiser *serialiser, cnt_welcome_message *value)
{
	fck_serialise(serialiser, &value->id);
	fck_serialise(serialiser, &value->self);
	fck_serialise(serialiser, &value->host);

	fck_serialise(serialiser, &value->peer);
	fck_serialise(serialiser, &value->avatar);
}

void cnt_networking_process_recv_message_unicast(fck_ecs *ecs, fck_serialiser *serialiser, cnt_connection_handle const *connection)
{
	// callback required

	cnt_peers *peers = fck_ecs_unique_view<cnt_peers>(ecs);
	cnt_welcome_message message;

	fck_serialise(serialiser, &message);
	SDL_assert(message.id == 0xB055);

	fck_control_layout *layout = fck_ecs_component_create<fck_control_layout>(ecs, message.avatar);
	*layout = {SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, SDL_SCANCODE_UP, SDL_SCANCODE_DOWN};
	fck_ecs_component_create<fck_authority>(ecs, message.avatar);

	message.peer.state = cnt_peer_STATE_OK;

	cnt_peers_set_host(peers, message.host);
	cnt_peers_set_self(peers, message.peer.peer_id);

	cnt_peers_emplace(peers, &message.peer, connection);
}

void cnt_networking_process_recv_replication_broadcast(fck_ecs *ecs, fck_serialiser *serialiser, cnt_connection_handle const *connection)
{
	cnt_peers *peers = fck_ecs_unique_view<cnt_peers>(ecs);
	cnt_session *session = fck_ecs_unique_view<cnt_session>(ecs);

	// Peer info
	cnt_networking_segment_type peers_segment;
	fck_serialise(serialiser, &peers_segment);
	SDL_assert(peers_segment == FCK_NETWORK_SEGMENT_TYPE_PEERS);

	cnt_peer_id peer_count = 0;
	fck_serialise(serialiser, &peer_count);
	for (cnt_peer_id index = 0; index < peer_count; index++)
	{
		cnt_peer avatar = {cnt_peers::invalid_peer};
		fck_serialise(serialiser, &avatar);
		SDL_assert(avatar.peer_id != cnt_peers::invalid_peer);

		cnt_peer *peer = cnt_peers_view_peer_from_index(peers, index);
		if (peer != nullptr)
		{
			// We can only BUILD up peer state, never go back.
			// If a peer reached a specific state, it can only remain in there
			avatar.state = SDL_max(avatar.state, peer->state);
		}
		cnt_peers_emplace(peers, &avatar, nullptr);
	}

	cnt_peer peer;
	if (cnt_peers_try_get_peer_from_connection(peers, connection, &peer))
	{
		// Receive from... client
		if (peer.state == cnt_peer_STATE_OK)
		{
			// ECS snapshot
			cnt_networking_segment_type ecs_segment;
			fck_serialise(serialiser, &ecs_segment);
			SDL_assert(ecs_segment == FCK_NETWORK_SEGMENT_TYPE_ECS);

			if (cnt_peers_is_hosting(peers))
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

void cnt_networking_process_recv(fck_ecs *ecs)
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
		cnt_networking_communication_mode mode;
		fck_serialise(&serialiser, &mode);

		// This shit can happen in ANY order
		// FUCK ME
		switch (mode)
		{
		case FCK_NETWORK_COMMUNICATION_MODE_MESSAGE_UNICAST:
			cnt_networking_process_recv_message_unicast(ecs, &serialiser, &memory_view.info.connection);
			break;
		case FCK_NETWORK_COMMUNICATION_MODE_REPLICATION_BROADCAST:
			cnt_networking_process_recv_replication_broadcast(ecs, &serialiser, &memory_view.info.connection);
			break;
		default:
			SDL_assert(false && "Great message buddy. The communication mode we received is unknown lol");
			break;
		}
	}
}

void cnt_networking_process_send(fck_ecs *ecs)
{
	cnt_session *session = fck_ecs_unique_view<cnt_session>(ecs);
	cnt_peers *peers = fck_ecs_unique_view<cnt_peers>(ecs);

	// Setup
	fck_serialiser serialiser;
	fck_serialiser_alloc(&serialiser);
	fck_serialiser_byte_writer(&serialiser.self);

	// Header
	cnt_networking_communication_mode mode = FCK_NETWORK_COMMUNICATION_MODE_REPLICATION_BROADCAST;
	fck_serialise(&serialiser, &mode);

	// Peer info
	cnt_networking_segment_type peers_segment = FCK_NETWORK_SEGMENT_TYPE_PEERS;
	fck_serialise(&serialiser, &peers_segment);

	cnt_peer_id peer_count = peers->peers.dense.count;
	fck_serialise(&serialiser, &peer_count);
	for (fck_item<cnt_peer_id, cnt_peer> item : &peers->peers)
	{
		SDL_assert(*item.index == item.value->peer_id);
		fck_serialise(&serialiser, item.value);
	}

	// ECS snapshot
	// Send out
	if (cnt_peers_is_ok(peers))
	{
		cnt_networking_segment_type ecs_segment = FCK_NETWORK_SEGMENT_TYPE_ECS;
		fck_serialise(&serialiser, &ecs_segment);

		if (cnt_peers_is_hosting(peers))
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

typedef void (*cnt_peer_on_connect_function)(fck_ecs *ecs, cnt_peer *);

void cnt_networking_process_setup_new_connections(fck_ecs *ecs)
{
	cnt_session *session = fck_ecs_unique_view<cnt_session>(ecs);
	cnt_peers *peers = fck_ecs_unique_view<cnt_peers>(ecs);

	cnt_connection_handle handle;
	cnt_connection connection;
	while (cnt_session_try_dequeue_new_connection(session, &handle, &connection))
	{
		// callback required
		//
		// The requester is never responsible, the one accepting is responsible!
		bool is_hosting = (connection.flags & CNT_CONNECTION_FLAG_CLIENT) != CNT_CONNECTION_FLAG_CLIENT;
		if (is_hosting)
		{
			cnt_peer *peer;
			if (cnt_peers_try_add(peers, &handle, &peer))
			{
				fck_ecs::entity_type avatar = create_cammy(ecs);

				uint8_t buffer[32];
				fck_serialiser serialiser;
				fck_serialiser_create(&serialiser, buffer, sizeof(buffer));
				fck_serialiser_byte_writer(&serialiser.self);

				// Header
				cnt_networking_communication_mode mode = FCK_NETWORK_COMMUNICATION_MODE_MESSAGE_UNICAST;
				fck_serialise(&serialiser, &mode);

				// Message
				cnt_welcome_message message;
				message.id = 0xB055;

				message.peer = *peer;
				message.avatar = avatar;
				fck_serialise(&serialiser, &message);

				cnt_session_send(session, &handle, serialiser.data, serialiser.at);
				SDL_Log("Created Avatar: %d", avatar);
			}
		}
		else
		{
			fck_ecs_entity_destroy_all(ecs);
			cnt_peers_clear(peers);
		}
	}
}

void networking_process(fck_ecs *ecs, fck_system_update_info *)
{
	cnt_session *session = fck_ecs_unique_view<cnt_session>(ecs);
	fck_time *time = fck_ecs_unique_view<fck_time>(ecs);

	cnt_networking_process_recv(ecs);

	bool will_tick_this_frame = cnt_session_will_tick(session, time->delta);
	if (will_tick_this_frame)
	{
		cnt_networking_process_send(ecs);
	}

	cnt_session_tick(session, time->current, time->delta);

	if (will_tick_this_frame)
	{
		// Technically on a connection (from tick) we will first process networking events
		// AND THEN we will send the on connect setup message to the player
		// Man... I suck
		cnt_networking_process_setup_new_connections(ecs);
	}
}

void networking_setup(fck_ecs *ecs, fck_system_once_info *)
{
	fck_instance_info *info = fck_ecs_unique_view<fck_instance_info>(ecs);

	cnt_session *session = fck_ecs_unique_create<cnt_session>(ecs, cnt_session_free);
	cnt_peers *peers = fck_ecs_unique_create<cnt_peers>(ecs, cnt_peers_free);

	cnt_peers_alloc(peers);

	constexpr size_t tick_rate = 1000 / 60;
	cnt_session_alloc(session, 4, 128, 64, tick_rate);
	cnt_socket_handle socket_handle = cnt_session_socket_create(session, info->ip, info->source_port);
	if (info->destination_port != 0)
	{
		cnt_address_handle address_handle = cnt_session_address_create(session, info->ip, info->destination_port);
		cnt_session_connect(session, &socket_handle, &address_handle);
	}

	fck_ecs_system_add(ecs, networking_process);
}

void fck_instance_setup(fck_ecs *ecs)
{
	// Good old fashioned init systems
	fck_ecs_system_add(ecs, networking_setup);
	fck_ecs_system_add(ecs, local_cammy_setup);

	// Good old fasioned update systems
	fck_ecs_system_add(ecs, input_process);
	fck_ecs_system_add(ecs, gameplay_process);
	fck_ecs_system_add(ecs, animations_process);
	fck_ecs_system_add(ecs, render_process);
}

int main(int argc, char **argv)
{
	CHECK_CRITICAL(SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS), SDL_GetError());
	CHECK_CRITICAL(IMG_Init(IMG_INIT_PNG), SDL_GetError());

	fck_instances instances;
	fck_instances_alloc(&instances, 8);

	fck_instance_info client_info;
	client_info.title = "fck engine - client";
	client_info.ip = "127.0.0.1";
	client_info.source_port = 42069;
	client_info.destination_port = 42072;
	fck_instances_add(&instances, &client_info, fck_instance_setup);

	fck_instance_info server_info;
	server_info.title = "fck engine - server";
	server_info.ip = "127.0.0.1";
	server_info.source_port = 42072;
	server_info.destination_port = 0;
	fck_instances_add(&instances, &server_info, fck_instance_setup);

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
