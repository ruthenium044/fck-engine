#include "fck.h"

#include "core/fck_instances.h"
#include "shared/fck_checks.h"

// SDL core - functionality such as creating a window and getting events
#include <SDL3/SDL.h>

// SDL image - Loads images... Many kinds. We only care about PNG
#include <core/fck_time.h>
#include <SDL3_image/SDL_image.h>


void fck_init(fck *core, uint8_t instance_capacity)
{
	SDL_assert(core != nullptr);
	SDL_zerop(core);

	CHECK_CRITICAL(SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS), SDL_GetError());

	fck_instances_alloc(&core->instances, instance_capacity);
}

void fck_prepare(fck *core, fck_instance_info const *info, fck_instance_setup_function setup_function)
{
	SDL_assert(core != nullptr);
	// Passing it down multiple times... Not a fan
	fck_instances_add(&core->instances, info, setup_function);
}

int fck_run(fck *core)
{
	fck_milliseconds tp = SDL_GetTicks();
	while (fck_instances_any_active(&core->instances))
	{
		// Maybe global control later
		fck_milliseconds now = SDL_GetTicks();
		fck_milliseconds delta_time = now - tp;
		tp = now;

		for (fck_instance *instance : &core->instances)
		{
			fck_time *time = fck_ecs_unique_view<fck_time>(&instance->ecs);
			time->current = tp;
			time->delta = delta_time;
		}

		fck_instances_process_events(&core->instances);

		for (fck_instance *instance : &core->instances)
		{
			fck_ecs_tick(&instance->ecs);
		}
	}

	// If we get any other return codes or things that require error processing, we should just return it here

	return 0;
}

void fck_quit(fck *core)
{
	SDL_assert(core != nullptr);

	fck_instances_free(&core->instances);
	
	SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
	SDL_Quit();

	SDL_zerop(core);
}