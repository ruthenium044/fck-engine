#include "game/game_core.h"

#include "core/fck_engine.h"
#include "ecs/fck_ecs.h"

#include "game/game_components.h"

#include "gen/gen_assets.h"

fck_ecs::entity_type game_cammy_create(fck_ecs *ecs)
{
	fck_engine *engine = fck_ecs_unique_view<fck_engine>(ecs);

	fck_ecs::entity_type cammy = fck_ecs_entity_create(ecs);

	game_position *position = fck_ecs_component_create<game_position>(ecs, cammy);

	game_controller *controller = fck_ecs_component_create<game_controller>(ecs, cammy);
	game_size* size = fck_ecs_component_create<game_size>(ecs, cammy);
	game_debug_colour* color = fck_ecs_component_create<game_debug_colour>(ecs, cammy);

	game_sprite* sprite = fck_ecs_component_create<game_sprite>(ecs, cammy);
	sprite->texture = gen_assets_png::PestLogo;
	sprite->src = { 0.0f, 0.0f, 1000.0f, 1000.0f };

	// We just offset the y position based on the entity ID. Good enough
	position->x = 0.0f;
	position->y = 128.0f + (cammy * 128);
	size->w = 32.0f;
	size->h = 32.0f;
	*color = {255, 0, 0, 255};

	return cammy;
}