#include "game/game_systems.h"
#include "shared/fck_checks.h"
#include <SDL3/SDL_timer.h>

#include "fck.h"

#include "assets/fck_assets.h"

#include "fck_ui.h"

void game_instance_setup(fck_ecs *ecs)
{
	// Good old fashioned init systems
	// TODO: sprite_sheet_setup is loading: Cammy. That is not so muy bien
	fck_ecs_system_add(ecs, fck_ui_setup);
	// fck_ecs_system_add(ecs, game_networking_setup);
	fck_ecs_system_add(ecs, game_authority_controllable_create);
	fck_ecs_system_add(ecs, game_networking_setup);

	// Good old fasioned update systems
	fck_ecs_system_add(ecs, game_input_process);
	fck_ecs_system_add(ecs, game_gameplay_process);

	fck_ecs_system_add(ecs, game_network_ui_process);
	fck_ecs_system_add(ecs, game_render_process);
}

int main(int argc, char **argv)
{
	fck fck;
	fck_init(&fck, 3);
	{
		fck_instance_info client_info0;
		client_info0.title = "fck engine - client 0";
		fck_prepare(&fck, &client_info0, game_instance_setup);

		fck_instance_info client_info1;
		client_info0.title = "fck engine - client 1";
		fck_prepare(&fck, &client_info0, game_instance_setup);

		fck_instance_info host_info;
		host_info.title = "fck engine - host";
		fck_prepare(&fck, &host_info, game_instance_setup);
	}

	int exit_code = fck_run(&fck);
	SDL_Log("fck - exit code: %d", exit_code);

	return exit_code;
}
