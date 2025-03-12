#include "game/game_systems.h"
#include "shared/fck_checks.h"

#include "fck.h"

#include "assets/fck_assets.h"

#include "fck_ui.h"

#include <SDL3/SDL_thread.h>
#include "netv2/cnt_net.h"

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
	cnt_start_up();
	SDL_Thread* thread_server = SDL_CreateThread(example_server, "", nullptr);
	SDL_Thread* thread_client = SDL_CreateThread(example_client, "", nullptr);

	int status_server, status_client;
	SDL_WaitThread(thread_server, &status_server);
	SDL_WaitThread(thread_client, &status_client);
	cnt_tead_down();

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
