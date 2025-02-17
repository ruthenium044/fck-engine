#include "core/fck_instance.h"
#include "core/fck_drop_file.h"
#include "core/fck_engine.h"
#include "core/fck_keyboard.h"
#include "core/fck_mouse.h"

#include "ecs/snapshot/fck_ecs_timeline.h"

#include "assets/fck_assets.h"

SDL_EnumerationResult fck_print_directory(void *userdata, const char *dirname, const char *fname)
{
	const char *extension = SDL_strrchr(fname, '.');

	SDL_PathInfo path_info;

	size_t path_count = SDL_strlen(dirname);
	size_t file_name_count = SDL_strlen(fname);
	size_t total_count = file_name_count + path_count + 1;
	char *path = (char *)SDL_malloc(total_count);
	path[0] = '\0';

	size_t last = 0;
	last = SDL_strlcat(path, dirname, total_count);
	last = SDL_strlcat(path, fname, total_count);

	SDL_Log("%s - %s - %s", dirname, fname, extension);
	if (SDL_GetPathInfo(path, &path_info))
	{
		if (path_info.type == SDL_PATHTYPE_DIRECTORY)
		{
			SDL_EnumerateDirectory(path, fck_print_directory, userdata);
		}
	}

	SDL_free(path);

	return SDL_ENUM_SUCCESS;
}

void fck_engine_free(fck_engine *engine)
{
	SDL_assert(engine != nullptr);

	SDL_DestroyRenderer(engine->renderer);
	SDL_DestroyWindow(engine->window);

	SDL_zerop(engine);
}

static void engine_setup(fck_ecs *ecs, fck_system_once_info *)
{
	fck_instance_info *info = fck_ecs_unique_view<fck_instance_info>(ecs);

	fck_engine *engine = fck_ecs_unique_view<fck_engine>(ecs);

	const int window_width = 640;
	const int window_height = 640;
	engine->window = SDL_CreateWindow(info->title, window_width, window_height, SDL_WINDOW_RESIZABLE);
	CHECK_CRITICAL(engine->window, SDL_GetError());

	engine->renderer = SDL_CreateRenderer(engine->window, SDL_SOFTWARE_RENDERER);
	CHECK_CRITICAL(engine->renderer, SDL_GetError());

	fck_assets_load_multi(engine->renderer, gen_png_all, SDL_arraysize(gen_png_all));

	CHECK_WARNING(SDL_SetRenderVSync(engine->renderer, true), SDL_GetError());

	fck_keyboard_state *keyboard = fck_ecs_unique_create<fck_keyboard_state>(ecs);
	fck_mouse_state *mouse = fck_ecs_unique_create<fck_mouse_state>(ecs);

	fck_ecs_timeline *snapshot_timeline = fck_ecs_unique_create<fck_ecs_timeline>(ecs, fck_ecs_timeline_free);
	// TODO: Maybe fix default capacities
	fck_ecs_timeline_alloc(snapshot_timeline, 128, 16);

	fck_time *time = fck_ecs_unique_create<fck_time>(ecs);

	// Junk below here should probably get moved away!
	fck_drop_file_context *drop_file_context = fck_ecs_unique_create<fck_drop_file_context>(ecs, fck_drop_file_context_free);
	fck_drop_file_context_allocate(drop_file_context, 16);
	fck_drop_file_context_push(drop_file_context, fck_drop_file_receive_png);

	SDL_EnumerateDirectory(FCK_RESOURCE_DIRECTORY_PATH, fck_print_directory, nullptr);

	engine->is_running = true;
}

void fck_instance_alloc(fck_instance *instance, fck_instance_info const *info, fck_instance_setup_function instance_setup)
{
	SDL_assert(instance != nullptr);
	SDL_assert(instance_setup != nullptr);

	// Hardcoded to 128
	fck_queue_alloc(&instance->event_queue, 128);

	fck_ecs_alloc_info ecs_alloc_info = {256, 128, 64};
	fck_ecs_alloc(&instance->ecs, &ecs_alloc_info);

	fck_ecs_system_add(&instance->ecs, engine_setup);

	instance_setup(&instance->ecs);

	// We place the engine inside of the ECS as a unique - this way anything that
	// can access the ecs, can also access the engine. Ergo, we intigrate the
	// engine as part of the ECS workflow
	instance->engine = fck_ecs_unique_create<fck_engine>(&instance->ecs, fck_engine_free);
	// We flush the once systems since they might be relevant for startup
	// Adding once system during once system might file - We should enable that
	// If we queue a once system during a once system, what should happen?

	fck_ecs_unique_set<fck_instance_info>(&instance->ecs, info);

	fck_ecs_flush_system_once(&instance->ecs);
}

void fck_instance_free(fck_instance *instance)
{
	SDL_assert(instance != nullptr);

	fck_queue_free(&instance->event_queue);

	// We ignore free-ing memory for now
	// Since the ECS exists in this scope and we pass it along, we are pretty much
	// guaranteed that the OS will clean it up
	// I mean the ECS quite literally collects all the garbage in the application since it takes ownership
	// and then it decides to free... it just so happens it's at the very end
	fck_ecs_free(&instance->ecs);

	SDL_zerop(instance);
}