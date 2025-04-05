#include "game/game_systems.h"

#include "core/fck_instance.h"
#include "ecs/fck_ecs.h"

#include "game/game_components.h"
#include "game/game_core.h"

#include "netv2/cnt_net.h"

#include "fck_ui.h"

void game_networking_setup(fck_ecs *ecs, fck_system_once_info *)
{
	fck_instance_info *info = fck_ecs_unique_view<fck_instance_info>(ecs);
}

void game_cnt_user_host_close(cnt_user_host *user)
{
	if (user != nullptr)
	{
		cnt_user_host_close(user);
	}
}

void game_cnt_user_client_close(cnt_user_client *user)
{
	if (user != nullptr)
	{
		cnt_user_client_close(user);
	} 
}

struct game_network_debug
{

};

void game_network_ui_process(struct fck_ecs *ecs, struct fck_system_update_info *)
{
	fck_ui *ui = fck_ecs_unique_view<fck_ui>(ecs);
	if (ui == nullptr)
	{
		return;
	}

	// TODO: Make that better. I think...
	cnt_user_client *client = fck_ecs_unique_view<cnt_user_client>(ecs);
	if (client == nullptr)
	{
		client = fck_ecs_unique_create<cnt_user_client>(ecs, game_cnt_user_client_close);
	}

	cnt_user_host *host = fck_ecs_unique_view<cnt_user_host>(ecs);
	if (host == nullptr)
	{
		host = fck_ecs_unique_create<cnt_user_host>(ecs, game_cnt_user_host_close);
	}

	game_network_debug* network_debug = fck_ecs_unique_view<game_network_debug>(ecs);
	if (network_debug == nullptr)
	{
		network_debug = fck_ecs_unique_create<game_network_debug>(ecs);
	}

	nk_context *ctx = ui->ctx;

	nk_colorf bg;
	bg.r = 0.10f, bg.g = 0.18f, bg.b = 0.24f, bg.a = 1.0f;

	enum
	{
		EASY,
		HARD
	};

	/* GUI */
	if (nk_begin(ctx, "Network", nk_rect(50, 50, 230, 250),
	             NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE))
	{
		static int op = EASY;
		static int property = 20;

		nk_layout_row_dynamic(ctx, 30, 2);
		bool is_host_active = cnt_user_host_is_active(host);
		const char *host_label_text = is_host_active ? "Close Host" : "Open Host";
		if (nk_button_label(ctx, host_label_text))
		{
			if (is_host_active)
			{
				cnt_user_host_shut_down(host);
			}
			else
			{
				cnt_user_host_open(host, CNT_ANY_IP, 42069, 60);
			}
		}

		bool is_client_active = cnt_user_client_is_active(client);
		const char *client_label_text = is_client_active ? "Close Client" : "Open Client";
		if (nk_button_label(ctx, client_label_text))
		{
			if (is_client_active)
			{
				cnt_user_client_shut_down(client);
			}
			else
			{
				cnt_user_client_open(client, "127.0.0.1", 42069, 60);
			}
		}

		nk_layout_row_dynamic(ctx, 30, 2);
		nk_label(ctx, cnt_user_host_state_to_string(host), NK_TEXT_LEFT);
		nk_label(ctx, cnt_user_client_state_to_string(client), NK_TEXT_LEFT);

		nk_layout_row_dynamic(ctx, 30, 2);
		nk_label(ctx, host->host_ip ? host->host_ip : "None", NK_TEXT_LEFT);
		nk_label(ctx, client->host_ip ? client->host_ip : "None", NK_TEXT_LEFT);


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
