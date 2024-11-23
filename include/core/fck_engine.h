#ifndef FCK_ENGINE_INCLUDED
#define FCK_ENGINE_INCLUDED

#include "fck_time.h"
#include "fck_ui.h"

struct fck_engine
{
	static constexpr float screen_scale = 2.0f;

	SDL_Window *window;
	SDL_Renderer *renderer;

	// Remove this shit
	fck_font_asset default_editor_font;

	bool is_running;
};

#endif // !FCK_ENGINE_INCLUDED
