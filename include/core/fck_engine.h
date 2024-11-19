#ifndef FCK_ENGINE_INCLUDED
#define FCK_ENGINE_INCLUDED

#include "fck_ui.h"
#include <SDL3/SDL_stdinc.h>

using fck_milliseconds = uint64_t;

struct fck_time
{
	fck_milliseconds delta;
	fck_milliseconds current;
};

struct fck_engine
{
	static constexpr float screen_scale = 2.0f;

	SDL_Window *window;
	SDL_Renderer *renderer;

	// Remove this shit
	fck_font_asset default_editor_font;

	bool is_running;
};

void fck_engine_free(fck_engine *engine);

#endif // !FCK_ENGINE_INCLUDED
