// fck main
// TODO:
// - Graphics
// - Input handling
// - Draw some images
// - Systems and data model
// - Data serialisation
// - Scripting
// - Networking!! <- implies multiplayer

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

// We add Lua later
/*extern "C"
{
#include <lauxlib.h>

}
void *fck_lua_allocate(void *ud, void *ptr, size_t osize, size_t nsize) // NOLINT
{
    return nullptr;
}*/

#define LOG_LAST_CRITICAL_SDL_ERROR() SDL_LogCritical(0, "%s:%d - %s", __FILE__, __LINE__, SDL_GetError());

typedef unsigned char fck_byte;

struct fck_keyboard_state
{
	Uint8 current_state[SDL_NUM_SCANCODES];
	Uint8 previous_state[SDL_NUM_SCANCODES];
};

struct fck_mouse_state
{
	float current_x;
	float current_y;
	float previous_x;
	float previous_y;

	Uint32 current_button_state;
	Uint32 previous_button_state;
};

struct fck_entity_storage_header
{
	size_t byte_index;
	size_t byte_count;
	size_t definition_index;
};

struct fck_entity_definition
{
	// Fill out definition info
};

struct fck_entity
{
	size_t definition_index;
};

struct fck_engine_state
{
	Uint8 *entity_date;
	fck_entity_definition *definitions;

	size_t data_count;
	size_t data_capacity;

	size_t entity_count;
	size_t entity_capacity;
};

static void fck_keyboard_state_update(fck_keyboard_state *keyboard_state)
{
	int num_keys = 0;
	Uint8 const *current_state = SDL_GetKeyboardState(&num_keys);

	SDL_memcpy((Uint8 *)keyboard_state->previous_state, (Uint8 *)keyboard_state->current_state, num_keys);

	SDL_memcpy((Uint8 *)keyboard_state->current_state, (Uint8 *)current_state, num_keys);
}

static bool fck_key_down(fck_keyboard_state const *keyboard_state, SDL_Scancode scancode)
{
	return keyboard_state->current_state[(size_t)scancode];
}

static bool fck_key_up(fck_keyboard_state const *keyboard_state, SDL_Scancode scancode)
{
	return !fck_key_down(keyboard_state, scancode);
}

static void fck_mouse_state_update(fck_mouse_state *mouse_state)
{
	mouse_state->previous_x = mouse_state->current_x;
	mouse_state->previous_y = mouse_state->current_y;

	mouse_state->previous_button_state = mouse_state->current_button_state;
	mouse_state->current_button_state = SDL_GetMouseState(&mouse_state->current_x, &mouse_state->current_y);
}

static bool fck_button_down(fck_mouse_state const *mouse_state, int button_index /* 1 ... n - Mice are weird*/)
{
	int button_mask = SDL_BUTTON(button_index);
	return (mouse_state->current_button_state & button_mask) == button_mask;
}

static bool fck_button_up(fck_mouse_state const *mouse_state, int button_index /* 1 ... n - Mice are weird*/)
{
	return !fck_button_down(mouse_state, button_index);
}

static fck_entity fck_entity_create(fck_engine_state *engine_state, size_t byte_count)
{
	SDL_assert(engine_state->entity_count < engine_state->entity_capacity);
	SDL_assert(engine_state->data_count < engine_state->data_capacity);

	size_t index = engine_state->entity_count;
	engine_state->entity_count = engine_state->entity_count + 1; // Advance

	size_t byte_begin = engine_state->data_count;
	engine_state->data_count = engine_state->data_count + byte_count;

	fck_entity_definition definition;

	fck_entity_storage_header storage_header;
	storage_header.definition_index = index;
	storage_header.byte_index = byte_begin;
	storage_header.byte_count = byte_count;

	fck_entity entity;
	entity.definition_index = index;

	return entity;
}

static void *fck_entity_data_get(fck_engine_state *engine_state, fck_entity const *entity)
{
	fck_entity_definition definition = engine_state->definitions[entity->definition_index];
}

int main(int, char **)
{
	IMG_Load
	/* // TODOL Lets implement lua bindings later as needed - when we iterate
	    if (SDL_SetRenderVSync(renderer, SDL_RENDERER_VSYNC_ADAPTIVE))
	    {
	        LOG_LAST_CRITICAL_SDL_ERROR();
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
	        return -1;
	    }
	*/

	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
	{
		LOG_LAST_CRITICAL_SDL_ERROR();
		return -1;
	}

	SDL_Window *window = SDL_CreateWindow("fck - engine", 640, 640, 0);
	if (window == nullptr)
	{
		LOG_LAST_CRITICAL_SDL_ERROR();
		return -1;
	}

	SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);
	if (renderer == nullptr)
	{
		LOG_LAST_CRITICAL_SDL_ERROR();
		return -1;
	}
	if (!SDL_SetRenderVSync(renderer, true))
	{
		LOG_LAST_CRITICAL_SDL_ERROR();
		return -1;
	}

	fck_keyboard_state keyboard_state;
	SDL_zero(keyboard_state);

	fck_mouse_state mouse_state;
	SDL_zero(mouse_state);

	fck_engine_state engine_state;
	SDL_zero(engine_state);

	// Initialise data blob
	const int INITIAL_ENGINE_ENTITY_COUNT = 128;
	const int INITIAL_ENGINE_DATA_BYTE_COUNT = 128 * sizeof(size_t);
	engine_state.entity_date = (Uint8 *)SDL_calloc(sizeof(*engine_state.entity_date), INITIAL_ENGINE_DATA_BYTE_COUNT);
	engine_state.definitions =
		(fck_entity_definition *)SDL_calloc(sizeof(*engine_state.definitions), INITIAL_ENGINE_ENTITY_COUNT);
	engine_state.entity_count = 0;
	engine_state.entity_capacity = INITIAL_ENGINE_ENTITY_COUNT;
	engine_state.data_count = 0;
	engine_state.data_capacity = INITIAL_ENGINE_DATA_BYTE_COUNT;

	fck_entity player_entity = fck_entity_create(&engine_state, sizeof(SDL_FRect));
	SDL_FRect player = {0, 0, 128, 128};

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
		fck_keyboard_state_update(&keyboard_state);
		fck_mouse_state_update(&mouse_state);

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		if (fck_button_up(&mouse_state, 1))
		{
			SDL_SetRenderDrawColor(renderer, 0, 255, 0, 0);
		}
		else
		{
			SDL_SetRenderDrawColor(renderer, 255, 0, 0, 0);
		}
		SDL_RenderFillRect(renderer, &player);

		SDL_RenderPresent(renderer);
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	// lua_close(lua_state);

	SDL_Quit();
	return 0;
}