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
		fck_event_input_device_keyboard keyboard;
		SDL_zero(keyboard);
		keyboard.common.size = sizeof(keyboard);
		keyboard.common.timestamp = event->common.timestamp;
		keyboard.common.type = FCK_EVENT_INPUT_TYPE_DEVICE;

		keyboard.device_type = FCK_INPUT_DEVICE_TYPE_KEYBOARD;
		keyboard.pkey = (fck_pkey)event->key.scancode;
		keyboard.vkey = event->key.key;
		keyboard.type = FCK_KEYBOARD_EVENT_TYPE_DOWN;
		keyboard.mod = event->key.mod;
		memcpy(&e, &keyboard, sizeof(keyboard));
	}
	break;
	case SDL_EVENT_KEY_UP: {
		fck_event_input_device_keyboard keyboard;
		SDL_zero(keyboard);
		keyboard.common.size = sizeof(keyboard);
		keyboard.common.timestamp = event->common.timestamp;
		keyboard.common.type = FCK_EVENT_INPUT_TYPE_DEVICE;

		keyboard.device_type = FCK_INPUT_DEVICE_TYPE_KEYBOARD;
		keyboard.pkey = (fck_pkey)event->key.scancode;
		keyboard.vkey = event->key.key;
		keyboard.type = FCK_KEYBOARD_EVENT_TYPE_UP;
		keyboard.mod = event->key.mod;
		memcpy(&e, &keyboard, sizeof(keyboard));
	}
	break;
	// case SDL_EVENT_TEXT_EDITING:
	case SDL_EVENT_TEXT_INPUT: {
		fck_event_input_text input;
		SDL_zero(input);
		input.common.size = sizeof(input);
		input.common.timestamp = event->common.timestamp;
		input.common.type = FCK_EVENT_INPUT_TYPE_TEXT;
		input.text = event->text.text;
		memcpy(&e, &input, sizeof(input));
	}
	break;
	case SDL_EVENT_MOUSE_MOTION: {
		fck_event_input_device_mouse mouse;
		SDL_zero(mouse);
		mouse.common.size = sizeof(mouse);
		mouse.common.timestamp = event->common.timestamp;
		mouse.common.type = FCK_EVENT_INPUT_TYPE_DEVICE;

		mouse.device_type = FCK_INPUT_DEVICE_TYPE_MOUSE;
		mouse.type = FCK_MOUSE_EVENT_TYPE_POSITION;
		mouse.x = event->motion.x;
		mouse.y = event->motion.y;
		mouse.dx = event->motion.xrel;
		mouse.dy = event->motion.yrel;
		memcpy(&e, &mouse, sizeof(mouse));
	}
	break;
	case SDL_EVENT_MOUSE_BUTTON_DOWN: {
		fck_event_input_device_mouse mouse;
		SDL_zero(mouse);
		mouse.common.size = sizeof(mouse);
		mouse.common.timestamp = event->common.timestamp;
		mouse.common.type = FCK_EVENT_INPUT_TYPE_DEVICE;

		mouse.device_type = FCK_INPUT_DEVICE_TYPE_MOUSE;

		switch (event->button.button)
		{
		case SDL_BUTTON_LEFT:
			mouse.type = FCK_MOUSE_EVENT_TYPE_BUTTON_LEFT;
			break;
		case SDL_BUTTON_MIDDLE:
			mouse.type = FCK_MOUSE_EVENT_TYPE_BUTTON_MIDDLE;
			break;
		case SDL_BUTTON_RIGHT:
			mouse.type = FCK_MOUSE_EVENT_TYPE_BUTTON_RIGHT;
			break;
		case SDL_BUTTON_X1:
			mouse.type = FCK_MOUSE_EVENT_TYPE_BUTTON_4;
			break;
		case SDL_BUTTON_X2:
			mouse.type = FCK_MOUSE_EVENT_TYPE_BUTTON_5;
			break;
		}
		mouse.clicks = event->button.clicks;
		mouse.is_down = 1;
		mouse.x = event->button.x;
		mouse.y = event->button.y;
		memcpy(&e, &mouse, sizeof(mouse));
	}
	break;
	case SDL_EVENT_MOUSE_BUTTON_UP: {
		fck_event_input_device_mouse mouse;
		SDL_zero(mouse);
		mouse.common.size = sizeof(mouse);
		mouse.common.timestamp = event->common.timestamp;
		mouse.common.type = FCK_EVENT_INPUT_TYPE_DEVICE;

		mouse.device_type = FCK_INPUT_DEVICE_TYPE_MOUSE;

		switch (event->button.button)
		{
		case SDL_BUTTON_LEFT:
			mouse.type = FCK_MOUSE_EVENT_TYPE_BUTTON_LEFT;
			break;
		case SDL_BUTTON_MIDDLE:
			mouse.type = FCK_MOUSE_EVENT_TYPE_BUTTON_MIDDLE;
			break;
		case SDL_BUTTON_RIGHT:
			mouse.type = FCK_MOUSE_EVENT_TYPE_BUTTON_RIGHT;
			break;
		case SDL_BUTTON_X1:
			mouse.type = FCK_MOUSE_EVENT_TYPE_BUTTON_4;
			break;
		case SDL_BUTTON_X2:
			mouse.type = FCK_MOUSE_EVENT_TYPE_BUTTON_5;
			break;
		}

		mouse.is_down = 0;
		mouse.x = event->button.x;
		mouse.y = event->button.y;
		memcpy(&e, &mouse, sizeof(mouse));
	}
	break;
	case SDL_EVENT_MOUSE_WHEEL: {
		fck_event_input_device_mouse mouse;
		SDL_zero(mouse);
		mouse.common.size = sizeof(mouse);
		mouse.common.timestamp = event->common.timestamp;
		mouse.common.type = FCK_EVENT_INPUT_TYPE_DEVICE;

		mouse.device_type = FCK_INPUT_DEVICE_TYPE_MOUSE;
		mouse.type = FCK_MOUSE_EVENT_TYPE_WHEEL;
		
		mouse.is_down = 0;
		mouse.x = event->wheel.x;
		mouse.y = event->wheel.y;
		memcpy(&e, &mouse, sizeof(mouse));
	}
	break;
	}

	fck_app* app = (fck_app*)appstate;
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