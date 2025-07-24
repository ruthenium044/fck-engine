#include "fck_instance.h"

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_render.h>

#include <fck_hash.h>

#include "fck_ui.h"

#include "fck_nuklear_demos.h"
#include "fck_ui_window_manager.h"

typedef fckc_u32 fck_entity_index;
typedef struct fck_entity
{
	fck_entity_index id;
} fck_entity;


int fck_ui_window_entities(struct fck_ui *ui, fck_ui_window *window, void *userdata)
{
	fck_ui_ctx *ctx = fck_ui_context(ui);

	// TODO: 
	return 0;
}

fck_instance_result fck_instance_overlay(fck_instance *instance)
{
	int width;
	int height;
	if (!SDL_GetWindowSize(instance->window, &width, &height))
	{
		return FCK_INSTANCE_FAILURE;
	}

	fck_ui_window_manager *window_manager = instance->window_manager;
	fck_ui_window_manager_tick(instance->ui, window_manager, 0, 0, width, height);

	switch (fck_ui_window_manager_query_text_input_signal(instance->ui, instance->window_manager))
	{
	case FCK_UI_WINDOW_MANAGER_TEXT_INPUT_SIGNAL_START:
		SDL_StartTextInput(instance->window);
		break;
	case FCK_UI_WINDOW_MANAGER_TEXT_INPUT_SIGNAL_STOP:
		SDL_StopTextInput(instance->window);
		break;
	default:
		// Shut up compiler
		break;
	}
	return FCK_INSTANCE_CONTINUE;
}

fck_instance *fck_instance_alloc(const char *title, int with, int height, SDL_WindowFlags window_flags, const char *renderer_name)
{
	fck_instance *app = (fck_instance *)SDL_malloc(sizeof(fck_instance));
	app->window = SDL_CreateWindow(title, 1920, 1080, SDL_WINDOW_RESIZABLE);
	app->renderer = SDL_CreateRenderer(app->window, renderer_name);
	app->ui = fck_ui_alloc(app->renderer);
	app->window_manager = fck_ui_window_manager_alloc(16);

	fck_ui_window_manager_create(app->window_manager, "Entities", NULL, fck_ui_window_entities);
	fck_ui_window_manager_create(app->window_manager, "Nk Overview", NULL, fck_ui_window_overview);
	fck_ui_set_style(fck_ui_context(app->ui), THEME_DRACULA);

	return app;
}

void fck_instance_free(fck_instance *instance)
{
	fck_ui_window_manager_free(instance->window_manager);

	fck_ui_free(instance->ui);
	SDL_DestroyRenderer(instance->renderer);
	SDL_DestroyWindow(instance->window);
	SDL_free(instance);
}

fck_instance_result fck_instance_event(fck_instance *instance, SDL_Event const *event)
{
	fck_ui_enqueue_event(instance->ui, event);
	return FCK_INSTANCE_CONTINUE;
}

fck_instance_result fck_instance_tick(fck_instance *instance)
{
	fck_instance_overlay(instance);

	SDL_SetRenderDrawColor(instance->renderer, 0, 0, 0, 0);
	SDL_RenderClear(instance->renderer);

	fck_ui_render(instance->ui, instance->renderer);

	SDL_RenderPresent(instance->renderer);

	return FCK_INSTANCE_CONTINUE;
}
