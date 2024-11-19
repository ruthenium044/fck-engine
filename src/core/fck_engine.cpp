#include "core/fck_engine.h"

void fck_engine_free(fck_engine *engine)
{
	SDL_assert(engine != nullptr);

	SDL_DestroyRenderer(engine->renderer);
	SDL_DestroyWindow(engine->window);

	SDL_zerop(engine);
}