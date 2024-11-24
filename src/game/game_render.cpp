#include "game/game_systems.h"

#include "core/fck_engine.h"
#include "core/fck_time.h"
#include "ecs/fck_ecs.h"
#include "fck_animator.h"
#include "game/game_components.h"

void game_render_process(fck_ecs *ecs, fck_system_update_info *)
{
	fck_engine *engine = fck_ecs_unique_view<fck_engine>(ecs);
	fck_time *time = fck_ecs_unique_view<fck_time>(ecs);
	fck_spritesheet *spritesheet = fck_ecs_unique_view<fck_spritesheet>(ecs);

	SDL_SetRenderDrawColor(engine->renderer, 0, 0, 0, 255);
	SDL_RenderClear(engine->renderer);

	fck_ecs_apply(ecs, [engine, spritesheet](fck_static_sprite *sprite, game_position *position) {
		SDL_FRect const *source = spritesheet->rect_list.data + sprite->sprite_index;

		float target_x = position->x;
		float target_y = position->y;
		float target_width = source->w * fck_engine::screen_scale;
		float target_height = source->h * fck_engine::screen_scale;
		SDL_FRect dst = {target_x, target_y, target_width, target_height};

		dst.x = dst.x + (sprite->offset.x * fck_engine::screen_scale);
		dst.y = dst.y + (sprite->offset.y * fck_engine::screen_scale);

		SDL_RenderTextureRotated(engine->renderer, spritesheet->texture, source, &dst, 0.0f, nullptr, SDL_FLIP_HORIZONTAL);
	});

	fck_ecs_apply(ecs, [engine, time, spritesheet](fck_animator *animator, game_position *position) {
		if (fck_animator_update(animator, time->delta))
		{
			SDL_FRect const *source = fck_animator_get_rect(animator, spritesheet);

			float target_x = position->x;
			float target_y = position->y;
			float target_width = source->w * fck_engine::screen_scale;
			float target_height = source->h * fck_engine::screen_scale;
			SDL_FRect dst = {target_x, target_y, target_width, target_height};
			fck_animator_apply(animator, &dst, fck_engine::screen_scale);

			SDL_RenderTextureRotated(engine->renderer, spritesheet->texture, source, &dst, 0.0f, nullptr, SDL_FLIP_HORIZONTAL);
		}
	});

	SDL_RenderPresent(engine->renderer);
}
