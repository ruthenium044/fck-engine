
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_log.h>

// TODO: Port over multi-instance utility from fck-v1
// TODO: Port over fck-ui (Nk -- Nuklear) for editor
// TODO:
#include "fck_ui.h"


typedef struct fck_app
{
	fck_ui ui;
	SDL_Window *window;
	SDL_Renderer *renderer;
} fck_app;

SDLMAIN_DECLSPEC SDL_AppResult SDLCALL SDL_AppInit(void **appstate, int argc, char *argv[])
{
	*appstate = SDL_malloc(sizeof(fck_app));
	fck_app *app = (fck_app *)(*appstate);
	app->window = SDL_CreateWindow("Window", 640, 640, 0);
	app->renderer = SDL_CreateRenderer(app->window, "vulkan");

	fck_ui_setup(&app->ui, app->renderer);

	return SDL_APP_CONTINUE;
}

SDLMAIN_DECLSPEC SDL_AppResult SDLCALL SDL_AppIterate(void *appstate)
{
	fck_app *app = (fck_app *)appstate;

	fck_ui_handle_grab(&app->ui, app->window);

	{
		struct nk_context* ctx = app->ui.ctx;

		enum
		{
			EASY,
			HARD
		};
		static int op = EASY;
		static float value = 0.6f;
		static int i = 20;

		if (nk_begin(ctx, "Show", nk_rect(50, 50, 220, 220), NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE | NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE))
		{
			/* fixed widget pixel width */
			nk_layout_row_static(ctx, 30, 80, 1);
			if (nk_button_label(ctx, "button"))
			{
				SDL_Log("Beep 0");
				/* event handling */
			}

			/* fixed widget window ratio width */
			nk_layout_row_dynamic(ctx, 30, 2);
			if (nk_option_label(ctx, "easy", op == EASY)) {
				op = EASY;
			}
			if (nk_option_label(ctx, "hard", op == HARD))
				op = HARD;

			/* custom widget pixel width */
			nk_layout_row_begin(ctx, NK_STATIC, 30, 2);
			{
				nk_layout_row_push(ctx, 50);
				nk_label(ctx, "Volume:", NK_TEXT_LEFT);
				nk_layout_row_push(ctx, 110);
				nk_slider_float(ctx, 0, &value, 1.0f, 0.1f);
			}
			nk_layout_row_end(ctx);
		}
		nk_end(ctx);
	}

	SDL_SetRenderDrawColor(app->renderer, 0, 0, 0, 0);
	SDL_RenderClear(app->renderer);

	fck_ui_render(&app->ui, app->renderer);

	SDL_RenderPresent(app->renderer);

	return SDL_APP_CONTINUE;
}

SDLMAIN_DECLSPEC SDL_AppResult SDLCALL SDL_AppEvent(void *appstate, SDL_Event *event)
{
	if (event->type == SDL_EVENT_QUIT)
	{
		return SDL_APP_SUCCESS;
	}

	fck_app *app = (fck_app *)appstate;

	fck_ui_handle_event(&app->ui, event);

	return SDL_APP_CONTINUE;
}

SDLMAIN_DECLSPEC void SDLCALL SDL_AppQuit(void *appstate, SDL_AppResult result)
{
	fck_app *app = (fck_app *)appstate;
	SDL_DestroyWindow(app->window);
	SDL_free(appstate);
}