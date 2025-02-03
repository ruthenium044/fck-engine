#include "game/game_systems.h"

#include "core/fck_engine.h"
#include "core/fck_keyboard.h"
#include "ecs/fck_ecs.h"
#include "game/game_components.h"
#include "game/game_core.h"
#include "net/cnt_core.h"

#include "fck_ui.h"

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

void game_demo_ui_process(struct fck_ecs *ecs, struct fck_system_update_info *)
{
	fck_ui *ui = fck_ecs_unique_view<fck_ui>(ecs);
	if (ui == nullptr)
	{
		return;
	}

	nk_context *ctx = ui->ctx;

	struct nk_colorf bg;
	bg.r = 0.10f, bg.g = 0.18f, bg.b = 0.24f, bg.a = 1.0f;

	enum
	{
		EASY,
		HARD
	};

	/* GUI */
	if (nk_begin(ctx, "Demo", nk_rect(50, 50, 230, 250),
	             NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE))
	{
		static int op = EASY;
		static int property = 20;

		nk_layout_row_static(ctx, 30, 80, 1);
		if (nk_button_label(ctx, "button"))
		{
		}

		nk_layout_row_dynamic(ctx, 30, 2);
		if (nk_option_label(ctx, "easy", op == EASY))
		{
			op = EASY;
		}
		if (nk_option_label(ctx, "hard", op == HARD))
		{
			op = HARD;
		}
		nk_layout_row_dynamic(ctx, 25, 1);
		nk_property_int(ctx, "Compression:", 0, &property, 100, 10, 1);

		nk_layout_row_dynamic(ctx, 20, 1);
		nk_label(ctx, "background:", NK_TEXT_LEFT);
		nk_layout_row_dynamic(ctx, 25, 1);
		if (nk_combo_begin_color(ctx, nk_rgb_cf(bg), nk_vec2(nk_widget_width(ctx), 400)))
		{
			nk_layout_row_dynamic(ctx, 120, 1);
			bg = nk_color_picker(ctx, bg, NK_RGBA);
			nk_layout_row_dynamic(ctx, 25, 1);
			bg.r = nk_propertyf(ctx, "#R:", 0, bg.r, 1.0f, 0.01f, 0.005f);
			bg.g = nk_propertyf(ctx, "#G:", 0, bg.g, 1.0f, 0.01f, 0.005f);
			bg.b = nk_propertyf(ctx, "#B:", 0, bg.b, 1.0f, 0.01f, 0.005f);
			bg.a = nk_propertyf(ctx, "#A:", 0, bg.a, 1.0f, 0.01f, 0.005f);
			nk_combo_end(ctx);
		}
	}
	nk_end(ctx);
}