#ifndef FCK_ENGINE_INCLUDED
#define FCK_ENGINE_INCLUDED

#include "fck_time.h"

struct fck_engine
{
	// TODO: Remove this!
	static constexpr float screen_scale = 4.0f;

	struct SDL_Window *window;
	struct SDL_Renderer *renderer;

	bool is_running;
};

#endif // !FCK_ENGINE_INCLUDED
