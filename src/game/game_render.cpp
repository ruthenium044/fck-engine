#include "game/game_systems.h"

#include <SDL3/SDL_render.h>

#include "core/fck_engine.h"
#include "core/fck_time.h"
#include "ecs/fck_ecs.h"
#include "game/game_components.h"

#include "assets/fck_assets.h"

#include "fck_ui.h"

void game_render_process(fck_ecs *ecs, fck_system_update_info *)
{
	fck_engine *engine = fck_ecs_unique_view<fck_engine>(ecs);
	fck_time *time = fck_ecs_unique_view<fck_time>(ecs);

	SDL_SetRenderDrawColor(engine->renderer, 0, 0, 0, 255);
	SDL_RenderClear(engine->renderer);

#ifdef GEN_DEFINED_PNG
	fck_ecs_apply(ecs, [engine](game_position *position, game_size *size, game_sprite *sprite) {
		float target_x = position->x;
		float target_y = position->y;
		float target_width = size->w;
		float target_height = size->h;
		SDL_FRect dst = {target_x, target_y, target_width, target_height};

		SDL_Texture *texture = fck_assets_get(sprite->texture);
		SDL_RenderTexture(engine->renderer, texture, &sprite->src, &dst);
	});
#endif

	fck_ecs_apply(ecs, [engine](game_position *position, game_size *size, game_debug_colour *colour) {
		float target_x = position->x;
		float target_y = position->y;
		float target_width = size->w;
		float target_height = size->h;
		SDL_FRect dst = {target_x, target_y, target_width, target_height};

		SDL_SetRenderDrawColor(engine->renderer, colour->r, colour->g, colour->b, colour->a);

		SDL_RenderRect(engine->renderer, &dst);
	});

	fck_ui_render(ecs, true);

	SDL_RenderPresent(engine->renderer);
}
