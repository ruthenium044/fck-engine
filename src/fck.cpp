#include "fck.h"

#include "core/fck_instances.h"
#include "shared/fck_checks.h"

// SDL core - functionality such as creating a window and getting events
#include <SDL3/SDL.h>

// SDL image - Loads images... Many kinds. We only care about PNG
#include <SDL3_image/SDL_image.h>
#include <core/fck_time.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

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

static void fck_tick(fck *core)
{
#ifdef __EMSCRIPTEN__
	if (!fck_instances_any_active(&core->instances))
	{
		emscripten_cancel_main_loop(); /* this should "kill" the app. */
	}
#endif

	// Maybe global control later
	fck_milliseconds now = SDL_GetTicks();

	for (fck_instance *instance : &core->instances)
	{
		fck_time *time = fck_ecs_unique_view<fck_time>(&instance->ecs);
		fck_milliseconds tp = time->current;
		fck_milliseconds delta_time = now - tp;

		time->current = tp;
		time->delta = delta_time;
	}

	fck_instances_process_events(&core->instances);

	for (fck_instance *instance : &core->instances)
	{
		fck_ecs_tick(&instance->ecs);
	}
}

int fck_run(fck *core)
{
#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop_arg((em_arg_callback_func)fck_tick, (void *)core, 0, true);
#else
	while (fck_instances_any_active(&core->instances))
	{
		fck_tick(core);
	}
#endif

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