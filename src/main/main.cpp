#include "game/game_systems.h"
#include "shared/fck_checks.h"

#include "fck.h"

void game_instance_setup(fck_ecs *ecs)
{
	// Good old fashioned init systems
	fck_ecs_system_add(ecs, game_spritesheet_setup);
	fck_ecs_system_add(ecs, game_networking_setup);
	fck_ecs_system_add(ecs, game_cammy_setup);

	// Good old fasioned update systems
	fck_ecs_system_add(ecs, game_input_process);
	fck_ecs_system_add(ecs, game_gameplay_process);
	fck_ecs_system_add(ecs, game_animation_process);
	fck_ecs_system_add(ecs, game_render_process);
}

int main(int argc, char **argv)
{
	fck fck;
	fck_init(&fck, 2);
	{
		fck_instance_info client_info;
		client_info.title = "fck engine - client";
		client_info.ip = "127.0.0.1";
		client_info.source_port = 42069;
		client_info.destination_port = 42072;
		fck_prepare(&fck, &client_info, game_instance_setup);

		fck_instance_info server_info;
		server_info.title = "fck engine - server";
		server_info.ip = "127.0.0.1";
		server_info.source_port = 42072;
		server_info.destination_port = 0;
		fck_prepare(&fck, &server_info, game_instance_setup);
	}

	int exit_code = fck_run(&fck);
	SDL_Log("fck - exit code: %d", exit_code);

	fck_quit(&fck);
	return exit_code;
}
