// SDL core - functionality such as creating a window and getting events
#include <SDL3/SDL.h>

// Vulkan - For later :)
#include <vulkan/vulkan.h>

// SDL image - Loads images... Many kinds. We only care about PNG
#include <SDL3_image/SDL_image.h>

#include "core/fck_instances.h"
#include "core/fck_time.h"
#include "game/game_systems.h"
#include "shared/fck_checks.h"

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
	CHECK_CRITICAL(SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS), SDL_GetError());
	CHECK_CRITICAL(IMG_Init(IMG_INIT_PNG), SDL_GetError());

	fck_instances instances;
	fck_instances_alloc(&instances, 8);

	fck_instance_info client_info;
	client_info.title = "fck engine - client";
	client_info.ip = "127.0.0.1";
	client_info.source_port = 42069;
	client_info.destination_port = 42072;
	fck_instances_add(&instances, &client_info, game_instance_setup);

	fck_instance_info server_info;
	server_info.title = "fck engine - server";
	server_info.ip = "127.0.0.1";
	server_info.source_port = 42072;
	server_info.destination_port = 0;
	fck_instances_add(&instances, &server_info, game_instance_setup);

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
