#include "game/game_systems.h"
#include "shared/fck_checks.h"
#include <SDL3/SDL_timer.h>

#include "fck.h"

#include "assets/fck_assets.h"

#include "fck_ui.h"

#include "netv2/cnt_net.h"

void game_instance_setup(fck_ecs *ecs)
{
	// Good old fashioned init systems
	// TODO: sprite_sheet_setup is loading: Cammy. That is not so muy bien
	fck_ecs_system_add(ecs, fck_ui_setup);
	// fck_ecs_system_add(ecs, game_networking_setup);
	fck_ecs_system_add(ecs, game_authority_controllable_create);

	// Good old fasioned update systems
	fck_ecs_system_add(ecs, game_input_process);
	fck_ecs_system_add(ecs, game_gameplay_process);

	fck_ecs_system_add(ecs, game_network_ui_process);

	fck_ecs_system_add(ecs, game_render_process);
}

int main(int argc, char **argv)
{
	// cnt_user_host user_host;
	// cnt_user_host_open(&user_host, CNT_ANY_IP, 42069, 60);

	// cnt_user_client user_client;
	// cnt_user_client_open(&user_client, "127.0.0.1", 42069, 60);

	// char user_host_hello[] = "Hello from host app";
	// char user_client_hello[] = "Hello from client app";

	// bool finish_up_client = false;
	// while (true)
	//{
	//	char recv_buffer[2048];

	//	if (!finish_up_client)
	//	{
	//		cnt_user_client_send(&user_client, user_client_hello, sizeof(user_client_hello));
	//		while (int recv_count = cnt_user_client_recv(&user_client, recv_buffer, sizeof(recv_buffer)))
	//		{
	//			SDL_Log("%*s", recv_count, recv_buffer);
	//			cnt_user_client_shut_down(&user_client);
	//			//finish_up_client = true;
	//		}
	//	}

	//	cnt_sparse_index from;
	//	cnt_user_host_broadcast(&user_host, user_host_hello, sizeof(user_host_hello));
	//	while (int recv_count = cnt_user_host_recv(&user_host, &from, recv_buffer, sizeof(recv_buffer)))
	//	{
	//		//cnt_user_host_shut_down(&user_host);

	//		SDL_Log("%*s", recv_count, recv_buffer);
	//		//cnt_user_host_kick(&user_host, from);
	//	}

	//	SDL_Delay(30);
	//}

	// return 0;

	fck fck;
	fck_init(&fck, 3);
	{
		fck_instance_info client_info0;
		client_info0.title = "fck engine - client 0";
		fck_prepare(&fck, &client_info0, game_instance_setup);

		fck_instance_info host_info;
		host_info.title = "fck engine - host";
		fck_prepare(&fck, &host_info, game_instance_setup);
	}

	int exit_code = fck_run(&fck);
	SDL_Log("fck - exit code: %d", exit_code);

	return exit_code;
}
