#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#include <fck_app_loadable.h>

#include <fck_os.h>

#include <fck_events.h>

SDLMAIN_DECLSPEC SDL_AppResult SDLCALL SDL_AppInit(void **appstate, int argc, char *argv[])
{
	fck_shared_object so = os->so->load("fck-app");

	fck_load_func *so_load = (fck_load_func *)os->so->symbol(so, FCK_APP_LOAD);
	fck_app_api *app_api = (fck_app_api *)so_load();

	fck_app *app = app_api->init(argc, argv);
	if (!app)
	{
		return SDL_APP_FAILURE;
	}
	app->api = app_api;
	*appstate = (void *)app;
	return SDL_APP_CONTINUE;
}

SDLMAIN_DECLSPEC SDL_AppResult SDLCALL SDL_AppIterate(void *appstate)
{
	fck_app *app = (fck_app *)appstate;
	if (!app->api->tick(app))
	{
		return SDL_APP_CONTINUE;
	}
	return SDL_APP_SUCCESS;
}

SDLMAIN_DECLSPEC SDL_AppResult SDLCALL SDL_AppEvent(void *appstate, SDL_Event *event)
{
	if (event->type == SDL_EVENT_QUIT)
	{
		return SDL_APP_SUCCESS;
	}

	fck_event e;
	SDL_zero(e);

	switch (event->type)
	{
	case SDL_EVENT_KEY_DOWN: {
		e.key.common.size = sizeof(e.key);
		e.key.common.timestamp = event->common.timestamp;
		e.key.common.type = FCK_EVENT_TYPE_DEVICE;

		e.key.device_type = FCK_INPUT_DEVICE_TYPE_KEYBOARD;
		e.key.pkey = (fck_pkey)event->key.scancode;
		// e.key.vkey = event->key.key;
		e.key.type = FCK_KEYBOARD_EVENT_TYPE_DOWN;
		e.key.mod = event->key.mod;
	}
	break;
	case SDL_EVENT_KEY_UP: {
		e.key.common.size = sizeof(e.key);
		e.key.common.timestamp = event->common.timestamp;
		e.key.common.type = FCK_EVENT_TYPE_DEVICE;

		e.key.device_type = FCK_INPUT_DEVICE_TYPE_KEYBOARD;
		e.key.pkey = (fck_pkey)event->key.scancode;
		// e.key.vkey = event->key.key;
		e.key.type = FCK_KEYBOARD_EVENT_TYPE_UP;
		e.key.mod = event->key.mod;
	}
	break;
	// case SDL_EVENT_TEXT_EDITING:
	case SDL_EVENT_TEXT_INPUT: {
		e.text.common.size = sizeof(e.text);
		e.text.common.timestamp = event->common.timestamp;
		e.text.common.type = FCK_EVENT_TYPE_TEXT;
		e.text.text = event->text.text;
	}
	break;
	case SDL_EVENT_MOUSE_MOTION: {
		e.mouse.common.size = sizeof(e.mouse);
		e.mouse.common.timestamp = event->common.timestamp;
		e.mouse.common.type = FCK_EVENT_TYPE_DEVICE;

		e.mouse.device_type = FCK_INPUT_DEVICE_TYPE_MOUSE;
		e.mouse.type = FCK_MOUSE_EVENT_TYPE_POSITION;
		e.mouse.x = event->motion.x;
		e.mouse.y = event->motion.y;
		e.mouse.dx = event->motion.xrel;
		e.mouse.dy = event->motion.yrel;
	}
	break;
	case SDL_EVENT_MOUSE_BUTTON_DOWN: {
		e.mouse.common.size = sizeof(e.mouse);
		e.mouse.common.timestamp = event->common.timestamp;
		e.mouse.common.type = FCK_EVENT_TYPE_DEVICE;

		e.mouse.device_type = FCK_INPUT_DEVICE_TYPE_MOUSE;

		switch (event->button.button)
		{
		case SDL_BUTTON_LEFT:
			e.mouse.type = FCK_MOUSE_EVENT_TYPE_BUTTON_LEFT;
			break;
		case SDL_BUTTON_MIDDLE:
			e.mouse.type = FCK_MOUSE_EVENT_TYPE_BUTTON_MIDDLE;
			break;
		case SDL_BUTTON_RIGHT:
			e.mouse.type = FCK_MOUSE_EVENT_TYPE_BUTTON_RIGHT;
			break;
		case SDL_BUTTON_X1:
			e.mouse.type = FCK_MOUSE_EVENT_TYPE_BUTTON_4;
			break;
		case SDL_BUTTON_X2:
			e.mouse.type = FCK_MOUSE_EVENT_TYPE_BUTTON_5;
			break;
		default:
			// TODO: Should not happen
			break;
		}
		e.mouse.clicks = event->button.clicks;
		e.mouse.is_down = 1;
		e.mouse.x = event->button.x;
		e.mouse.y = event->button.y;
	}
	break;
	case SDL_EVENT_MOUSE_BUTTON_UP: {
		e.mouse.common.size = sizeof(e.mouse);
		e.mouse.common.timestamp = event->common.timestamp;
		e.mouse.common.type = FCK_EVENT_TYPE_DEVICE;

		e.mouse.device_type = FCK_INPUT_DEVICE_TYPE_MOUSE;

		switch (event->button.button)
		{
		case SDL_BUTTON_LEFT:
			e.mouse.type = FCK_MOUSE_EVENT_TYPE_BUTTON_LEFT;
			break;
		case SDL_BUTTON_MIDDLE:
			e.mouse.type = FCK_MOUSE_EVENT_TYPE_BUTTON_MIDDLE;
			break;
		case SDL_BUTTON_RIGHT:
			e.mouse.type = FCK_MOUSE_EVENT_TYPE_BUTTON_RIGHT;
			break;
		case SDL_BUTTON_X1:
			e.mouse.type = FCK_MOUSE_EVENT_TYPE_BUTTON_4;
			break;
		case SDL_BUTTON_X2:
			e.mouse.type = FCK_MOUSE_EVENT_TYPE_BUTTON_5;
			break;
		default:
			// TODO: Should not happen
			break;
		}

		e.mouse.is_down = 0;
		e.mouse.x = event->button.x;
		e.mouse.y = event->button.y;
	}
	break;
	case SDL_EVENT_MOUSE_WHEEL: {
		e.mouse.common.size = sizeof(e.mouse);
		e.mouse.common.timestamp = event->common.timestamp;
		e.mouse.common.type = FCK_EVENT_TYPE_DEVICE;

		e.mouse.device_type = FCK_INPUT_DEVICE_TYPE_MOUSE;
		e.mouse.type = FCK_MOUSE_EVENT_TYPE_WHEEL;

		e.mouse.is_down = 0;
		e.mouse.x = event->wheel.x;
		e.mouse.y = event->wheel.y;
	}
	break;
	}

	fck_app *app = (fck_app *)appstate;
	app->api->on_event(app, &e);

	return SDL_APP_CONTINUE;
}

SDLMAIN_DECLSPEC void SDLCALL SDL_AppQuit(void *appstate, SDL_AppResult result)
{
	fck_app *app = (fck_app *)appstate;
	app->api->quit(app);
}

#ifndef SDL_MAIN_USE_CALLBACKS
int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

	struct fck_app_api *app = NULL;
	SDL_AppInit((void **)&app, argc, argv);
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