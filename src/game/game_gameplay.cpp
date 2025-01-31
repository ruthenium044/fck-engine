#include "game/game_systems.h"

#include "core/fck_engine.h"
#include "core/fck_keyboard.h"
#include "ecs/fck_ecs.h"
#include "game/game_components.h"
#include "game/game_core.h"
#include "net/cnt_core.h"

void game_cammy_setup(fck_ecs *ecs, fck_system_once_info *)
{
	fck_ecs::entity_type cammy = game_cammy_create(ecs);

	// These are not getting replicated
	game_control_layout *layout = fck_ecs_component_create<game_control_layout>(ecs, cammy);
	*layout = {SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, SDL_SCANCODE_UP, SDL_SCANCODE_DOWN};

	// This one is not supposed to get repio
	fck_ecs_component_create<cnt_authority>(ecs, cammy);

	// Annoying dependency on cnt? Unsure!
	cnt_peers *peers = fck_ecs_unique_view<cnt_peers>(ecs);
	cnt_peer *peer;
	if (peers != nullptr && cnt_peers_try_add(peers, nullptr, &peer))
	{
		peer->state = CNT_PEER_STATE_OK;
		cnt_peers_set_host(peers, peer->peer_id);
		cnt_peers_set_self(peers, peer->peer_id);
		SDL_Log("Created Avatar: %d", cammy);
	}
}

void game_input_process(fck_ecs *ecs, fck_system_update_info *)
{
	fck_keyboard_state *keyboard = fck_ecs_unique_view<fck_keyboard_state>(ecs);

	fck_ecs_apply(ecs, [ecs, keyboard](cnt_authority *, game_control_layout *layout, game_controller *controller) {
		int input_flag = 0;

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
		controller->input = (game_input_flag)input_flag;
	});
}

void game_gameplay_process(fck_ecs *ecs, fck_system_update_info *)
{
	fck_engine *engine = fck_ecs_unique_view<fck_engine>(ecs);

	// STATE EVAL
	// Movement
	fck_ecs_apply(ecs, [](game_controller *controller, game_position *position) {
		game_input_flag input_flag = controller->input;
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