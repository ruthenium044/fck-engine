#ifndef FCK_ENGINE_INCLUDED
#define FCK_ENGINE_INCLUDED

#include "fck_time.h"
#include "SDL3/SDL_render.h"

struct fck_engine
{
	static constexpr float screen_scale = 4.0f;

	SDL_Window *window;
	SDL_Renderer *renderer;

	bool is_running;
};

#endif // !FCK_ENGINE_INCLUDED
