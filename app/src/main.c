
//#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_render.h>


// TODO: Port over multi-instance utility from fck-v1
// TODO: Port over fck-ui (Nk -- Nuklear) for editor
// TODO:
#include "fck_instance.h"


SDL_AppResult fck_instance_result_to_sdl_app_result(fck_instance_result result)
{
	switch (result)
	{
	case FCK_INSTANCE_CONTINUE:
		return SDL_APP_CONTINUE;
	case FCK_INSTANCE_SUCCESS:
		return SDL_APP_SUCCESS;
	case FCK_INSTANCE_FAILURE:
	default:
		return SDL_APP_FAILURE;
	}
}

SDLMAIN_DECLSPEC SDL_AppResult SDLCALL SDL_AppInit(void **appstate, int argc, char *argv[])
{

	fck_instance *instance = *appstate = (fck_instance *)fck_instance_alloc("Window", 640, 640, 0, "vulkan");
	if (instance == NULL)
	{
		return SDL_APP_FAILURE;
	}
	return SDL_APP_CONTINUE;
}

SDLMAIN_DECLSPEC SDL_AppResult SDLCALL SDL_AppIterate(void *appstate)
{
	fck_instance_result result = fck_instance_tick((fck_instance*)appstate);
	return fck_instance_result_to_sdl_app_result(result);
}

SDLMAIN_DECLSPEC SDL_AppResult SDLCALL SDL_AppEvent(void *appstate, SDL_Event *event)
{
	if (event->type == SDL_EVENT_QUIT)
	{
		return SDL_APP_SUCCESS;
	}
	
	fck_instance_result result = fck_instance_event((fck_instance *)appstate, event);

	return fck_instance_result_to_sdl_app_result(result);
}

SDLMAIN_DECLSPEC void SDLCALL SDL_AppQuit(void *appstate, SDL_AppResult result)
{
	fck_instance_free((fck_instance *)appstate);
}

#ifndef SDL_MAIN_USE_CALLBACKS
int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

	fck_instance *app;
	SDL_AppInit((void**)&app, argc, argv);
	// Make this work without touching the core loop!
	while (SDL_AppIterate(app) == SDL_APP_CONTINUE)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (SDL_AppEvent(app, &event) != SDL_APP_CONTINUE)
			{
				goto fck_quit;
			}
		}
	}

fck_quit:
	SDL_Quit();
	return 0;
}
#endif