#include "game/game_systems.h"
#include "shared/fck_checks.h"

#include "fck.h"

#include "assets/fck_assets.h"

#include "fck_ui.h"

#include "netv2/cnt_net.h"
#include <SDL3/SDL_thread.h>

void game_instance_setup(fck_ecs *ecs)
{
	// Good old fashioned init systems
	// TODO: sprite_sheet_setup is loading: Cammy. That is not so muy bien
	fck_ecs_system_add(ecs, fck_ui_setup);
	fck_ecs_system_add(ecs, game_networking_setup);
	fck_ecs_system_add(ecs, game_authority_controllable_create);

	// Good old fasioned update systems
	fck_ecs_system_add(ecs, game_input_process);
	fck_ecs_system_add(ecs, game_gameplay_process);
	fck_ecs_system_add(ecs, game_demo_ui_process);
	fck_ecs_system_add(ecs, game_render_process);
}

int main(int argc, char **argv)
{
	cnt_user_host user_host;
	cnt_user_host_create(&user_host, CNT_ANY_IP, 42069);

	cnt_user_client user_client;
	cnt_user_client_create(&user_client, "192.168.68.55", 42069);

	SDL_Thread *thread_server = SDL_CreateThread((SDL_ThreadFunction)example_host, "", &user_host);
	SDL_Thread *thread_client = SDL_CreateThread((SDL_ThreadFunction)example_client, "", &user_client);

	int status_server, status_client;
	SDL_WaitThread(thread_server, &status_server);
	SDL_WaitThread(thread_client, &status_client);

	return 0;
	fck fck;
	fck_init(&fck, 2);
	{
		fck_instance_info client_info;
		client_info.title = "fck engine - client";
		client_info.ip = "127.0.0.1";
		client_info.source_port = 42069;
		client_info.destination_port = 42072;
		fck_prepare(&fck, &client_info, game_instance_setup);

		fck_instance_info host_info;
		host_info.title = "fck engine - host";
		host_info.ip = "127.0.0.1";
		host_info.source_port = 42072;
		host_info.destination_port = 0;
		fck_prepare(&fck, &host_info, game_instance_setup);
	}

	int exit_code = fck_run(&fck);
	SDL_Log("fck - exit code: %d", exit_code);

	return exit_code;
}
