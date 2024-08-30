// fck main
// TODO:
// - Graphics
// - Input handling
// - Draw some images
// - Systems and data model
// - Data serialisation
// - Scripting
// - Networking!!

#include <SDL3/SDL.h>
#include <lua.hpp>

extern "C"
{
#include <lauxlib.h>
}

typedef unsigned char fck_byte;

#define ERROR_INFO __LINE__

#define log_last_critical_error() SDL_LogCritical(0, "%s:%d - %s", __FILE__, __LINE__, SDL_GetError());

void *fck_lua_allocate(void *ud, void *ptr, size_t osize, size_t nsize) // NOLINT
{
	return nullptr;
}

int main(int, char **)
{
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
	{
		log_last_critical_error();
		return -1;
	}

	SDL_Window *window = SDL_CreateWindow("fck - engine", 640, 640, 0);
	if (window == nullptr)
	{
		log_last_critical_error();
		return -1;
	}

	SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);
	if (renderer == nullptr)
	{
		log_last_critical_error();
		return -1;
	}

	if (SDL_SetRenderVSync(renderer, SDL_RENDERER_VSYNC_ADAPTIVE))
	{
		log_last_critical_error();
		return -1;
	}

	lua_State *lua_state = luaL_newstate();
	luaL_openlibs(lua_state);
	constexpr char lua_test_script[] = "print('test')\0";

	int lua_load_buffer_error = luaL_loadbuffer(lua_state, lua_test_script, strlen(lua_test_script), "hello_world");
	if (lua_load_buffer_error != LUA_OK)
	{
		printf("LUA LOAD ERROR: %s - %d\n", lua_test_script, lua_load_buffer_error);
		int print_error = fprintf(stderr, "%s", lua_tostring(lua_state, -1));

		return -1;
	}

	int lua_call_error = lua_pcall(lua_state, 0, 0, 0);
	if (lua_call_error)
	{
		int print_error = fprintf(stderr, "%s", lua_tostring(lua_state, -1));
		lua_pop(lua_state, 1);
	}

	bool is_running = true;
	while (is_running)
	{
		SDL_Event ev;
		while (SDL_PollEvent(&ev))
		{
			if (ev.type == SDL_EVENT_QUIT)
			{
				is_running = false;
			}
		}
	}

	lua_close(lua_state);

	SDL_Quit();
	return 0;
}