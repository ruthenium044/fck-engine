#include "core/fck_time.h"
#include "game/game_systems.h"

#include "ecs/fck_ecs.h"
#include "ecs/snapshot/fck_ecs_timeline.h"

#include "netv2/cnt_net.h"

#include "fck_ui.h"

struct game_network_frequency
{
	uint32_t accumulator;
	uint32_t ms;
};

struct game_host_frequency
{
	game_network_frequency freq;
};

struct game_client_frequency
{
	game_network_frequency freq;
};

void game_network_frequency_set(game_network_frequency *freq, uint32_t hz)
{
	float ms_as_float = 1000.0f / SDL_min(1000, hz);
	uint32_t ms_as_u32 = (uint32_t)ms_as_float;
	freq->ms = ms_as_u32;
	freq->accumulator = 0;
}

void game_network_host_send_process(struct fck_ecs *ecs, struct fck_system_update_info *)
{
	cnt_user_host *host = fck_ecs_unique_view<cnt_user_host>(ecs);
	if (!cnt_user_host_is_active(host))
	{
		return;
	}

	fck_time *time = fck_ecs_unique_view<fck_time>(ecs);
	game_host_frequency *freq = fck_ecs_unique_view<game_host_frequency>(ecs);
	freq->freq.accumulator = freq->freq.accumulator + time->delta;
	if (freq->freq.accumulator < freq->freq.ms)
	{
		return;
	}
	freq->freq.accumulator = 0;

	fck_ecs_mc_timeline *timeline = fck_ecs_unique_view<fck_ecs_mc_timeline>(ecs);

	fck_serialiser serialiser;
	fck_serialiser_alloc(&serialiser);
	fck_serialiser_byte_writer(&serialiser.self);

	cnt_user_host_client_list_lock(host);

	uint32_t count;
	// TODO: Clean this up to:
	// uint32_t count = cnt_user_host_client_list_get(&host, &clients);
	cnt_client_on_host *clients = cnt_user_host_client_list_get(host, &count);

	for (int index = 0; index < count; index++)
	{
		cnt_client_on_host *client = clients + index;

		fck_serialiser_reset(&serialiser);
		fck_ecs_mc_timeline_delta_capture(timeline, client->id.index, ecs, &serialiser);
		cnt_user_host_send(host, client->id, serialiser.data, serialiser.at);
	}

	cnt_user_host_client_list_unlock(host);

	fck_serialiser_free(&serialiser);
}

void game_network_host_recv_process(struct fck_ecs *ecs, struct fck_system_update_info *)
{
	cnt_user_host *host = fck_ecs_unique_view<cnt_user_host>(ecs);
	if (!cnt_user_host_is_active(host))
	{
		return;
	}

	fck_ecs_mc_timeline *timeline = fck_ecs_unique_view<fck_ecs_mc_timeline>(ecs);

	uint8_t data[1024];
	cnt_sparse_index client;
	while (int recv_count = cnt_user_host_recv(host, &client, data, sizeof(data)))
	{
		fck_serialiser serialiser;
		fck_serialiser_create(&serialiser, data, recv_count);
		fck_serialiser_byte_reader(&serialiser.self);

		uint32_t ack;
		fck_serialise(&serialiser, &ack);
		fck_ecs_mc_timeline_delta_ack(timeline, client.index, ack);
	}
}

void game_network_client_send_process(struct fck_ecs *ecs, struct fck_system_update_info *)
{
	cnt_user_client *client = fck_ecs_unique_view<cnt_user_client>(ecs);
	if (!cnt_user_client_is_active(client))
	{
		return;
	}

	fck_time *time = fck_ecs_unique_view<fck_time>(ecs);
	game_client_frequency *freq = fck_ecs_unique_view<game_client_frequency>(ecs);
	freq->freq.accumulator = freq->freq.accumulator + time->delta;
	if (freq->freq.accumulator < freq->freq.ms)
	{
		return;
	}
	freq->freq.accumulator = 0;

	fck_ecs_sc_timeline *timeline = fck_ecs_unique_view<fck_ecs_sc_timeline>(ecs);

	fck_serialiser serialiser;
	fck_serialiser_alloc(&serialiser);
	fck_serialiser_byte_writer(&serialiser.self);

	// Last received
	fck_serialise(&serialiser, &timeline->protocol.ackd);

	cnt_user_client_send(client, serialiser.data, serialiser.at);

	fck_serialiser_free(&serialiser);
}

void game_network_client_recv_process(struct fck_ecs *ecs, struct fck_system_update_info *)
{
	cnt_user_client *client = fck_ecs_unique_view<cnt_user_client>(ecs);
	if (!cnt_user_client_is_active(client))
	{
		return;
	}

	fck_ecs_sc_timeline *timeline = fck_ecs_unique_view<fck_ecs_sc_timeline>(ecs);

	uint8_t data[1 << 16];
	while (int recv_count = cnt_user_client_recv(client, data, sizeof(data)))
	{
		fck_serialiser serialiser;
		fck_serialiser_create(&serialiser, data, recv_count);
		fck_serialiser_byte_reader(&serialiser.self);

		fck_ecs_sc_timeline_delta_apply(timeline, ecs, &serialiser);
	}
}

void game_networking_setup(fck_ecs *ecs, fck_system_once_info *)
{
	fck_ecs_unique_create<cnt_user_host>(ecs, cnt_user_host_close);
	fck_ecs_unique_create<cnt_user_client>(ecs, cnt_user_client_close);

	fck_ecs_sc_timeline *client_timeline = fck_ecs_unique_create<fck_ecs_sc_timeline>(ecs, fck_ecs_sc_timeline_free);
	fck_ecs_sc_timeline_alloc(client_timeline, 128, 16);

	fck_ecs_mc_timeline *host_timeline = fck_ecs_unique_create<fck_ecs_mc_timeline>(ecs, fck_ecs_mc_timeline_free);
	fck_ecs_mc_timeline_alloc(host_timeline, 128, 16, 32);

	game_network_frequency_set(&fck_ecs_unique_create<game_host_frequency>(ecs)->freq, 30);
	game_network_frequency_set(&fck_ecs_unique_create<game_client_frequency>(ecs)->freq, 30);

	fck_ecs_system_add(ecs, game_network_host_send_process);
	fck_ecs_system_add(ecs, game_network_client_send_process);

	fck_ecs_system_add(ecs, game_network_host_recv_process);
	fck_ecs_system_add(ecs, game_network_client_recv_process);
}

void game_network_ui_process(struct fck_ecs *ecs, struct fck_system_update_info *)
{
	fck_ui *ui = fck_ecs_unique_view<fck_ui>(ecs);
	if (ui == nullptr)
	{
		return;
	}

	cnt_user_client *client = fck_ecs_unique_view<cnt_user_client>(ecs);
	cnt_user_host *host = fck_ecs_unique_view<cnt_user_host>(ecs);
	nk_context *ctx = ui->ctx;

	nk_colorf bg;
	bg.r = 0.10f, bg.g = 0.18f, bg.b = 0.24f, bg.a = 1.0f;

	/* GUI */
	if (nk_begin(ctx, "Network", nk_rect(50, 50, 400, 400),
	             NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE))
	{
		char port_as_text[sizeof(int) * 8 + 1];

		// HOST
		nk_layout_row_dynamic(ctx, 24, 1);
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
				cnt_user_host_open(host, CNT_ANY_IP, 42069, 32, 60);
			}
		}
		nk_label(ctx, cnt_user_host_state_to_string(host), NK_TEXT_LEFT);

		nk_layout_row_dynamic(ctx, 12, 2);
		nk_label(ctx, "Hosting on:", NK_TEXT_LEFT);

		nk_layout_row_dynamic(ctx, 12, 2);
		nk_label(ctx, host->host_ip ? host->host_ip : "None", NK_TEXT_LEFT);
		nk_label(ctx, SDL_itoa((int)host->host_port, port_as_text, 10), NK_TEXT_LEFT);

		nk_layout_row_dynamic(ctx, 12, 1);
		nk_label(ctx, "Client List:", NK_TEXT_LEFT);

		if (is_host_active)
		{
			cnt_user_host_client_list_lock(host);

			uint32_t count;
			cnt_client_on_host *clients = cnt_user_host_client_list_get(host, &count);

			for (int i = 0; i < count; i++)
			{
				cnt_client_on_host *c = clients + i;

				nk_layout_row_dynamic(ctx, 12, 6);
				char i_as_text[sizeof(int) * 8 + 1];
				nk_label(ctx, SDL_itoa(c->id.index, i_as_text, 10), NK_TEXT_LEFT);

				nk_label(ctx, SDL_itoa(c->secret.private_value, i_as_text, 10), NK_TEXT_LEFT);
				nk_label(ctx, SDL_itoa(c->secret.public_value, i_as_text, 10), NK_TEXT_LEFT);

				nk_label(ctx, cnt_protocol_state_host_to_string(c->protocol), NK_TEXT_LEFT);

				nk_label(ctx, SDL_itoa(c->attempts, i_as_text, 10), NK_TEXT_LEFT);

				if (nk_button_label(ctx, "kick"))
				{
					cnt_user_host_kick(host, c->id);
				}
			}

			cnt_user_host_client_list_unlock(host);
		}

		// CLIENT

		nk_layout_row_dynamic(ctx, 24, 1);
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
		nk_layout_row_dynamic(ctx, 12, 2);
		nk_label(ctx, SDL_itoa((int)cnt_user_client_get_client_id_on_host(client).index, port_as_text, 10), NK_TEXT_LEFT);
		nk_label(ctx, cnt_user_client_state_to_string(client), NK_TEXT_LEFT);
		nk_layout_row_dynamic(ctx, 12, 2);
		nk_label(ctx, "Connected to:", NK_TEXT_LEFT);
		nk_layout_row_dynamic(ctx, 12, 2);
		nk_label(ctx, client->host_ip ? client->host_ip : "None", NK_TEXT_LEFT);
		nk_label(ctx, SDL_itoa((int)client->host_port, port_as_text, 10), NK_TEXT_LEFT);
		nk_layout_row_dynamic(ctx, 12, 2);

		nk_layout_row_dynamic(ctx, 12, 2);
		nk_label(ctx, "Client Protocol:", NK_TEXT_LEFT);
		nk_label(ctx, cnt_user_client_client_protocol_to_string(client), NK_TEXT_LEFT);
		nk_layout_row_dynamic(ctx, 12, 2);
		nk_label(ctx, "Host Protocol:", NK_TEXT_LEFT);
		nk_label(ctx, cnt_user_client_host_protocol_to_string(client), NK_TEXT_LEFT);

		/*nk_layout_row_dynamic(ctx, 30, 2);
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
		}*/
	}
	nk_end(ctx);
}
