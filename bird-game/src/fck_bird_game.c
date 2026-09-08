
#include "../../db/include/fck_db.h"

#include <fckc_apidef.h>

#include <fck_gameloop.h>

#include <fck_apis.h>
#include <fck_ec.h>

#include <fck_nuklear.h>
#include <fck_os.h>
#include <fck_sprite.h>
#include <fckc_assert.h>
#include <fckc_inttypes.h>

#include <kll.h>
#include <kll_malloc.h>

typedef struct fck_bird_game
{
	kll_allocator *allocator;
} fck_bird_game;

static fck_bird_game *fck_to_bird_game(fck_gameloop loop)
{
	fck_bird_game *game = (fck_bird_game *)(loop.handle);
	return game;
}

static fck_gameloop fck_bird_game_create(kll_allocator *allocator, const fck_gameloop_create_parameters *params)
{
	fck_bird_game *game = (fck_bird_game *)kll_malloc(allocator, sizeof(*game));

	game->allocator = allocator;
	const fck_gameloop gameloop = {.handle = (void *)game};
	return gameloop;
}

static void fck_bird_game_destroy(fck_gameloop loop, const fck_gameloop_destroy_parameters *params)
{
	fck_bird_game *game = fck_to_bird_game(loop);
	kll_free(game->allocator, game);
}

static int fck_bird_game_edit(fck_gameloop loop, const fck_gameloop_edit_parameters *params)
{
	fck_bird_game *game = fck_to_bird_game(loop);
	fck_nuklear_api *nk = params->nk;
	const fck_nk view = *params->view;

	if (nk->panel->begin_label(view, "Bird - Game", 400.0f))
	{
		nk->panel->end(view);
	}

	return 1;
}
static int fck_bird_game_tick(fck_gameloop loop, const fck_gameloop_tick_parameters *params)
{
	fck_bird_game *game = fck_to_bird_game(loop);
	fck_ec *state = params->state;
	fck_ec_api *ec = params->ec;

	fck_sprite_api *sprite_api = (fck_sprite_api *)params->apis->find(fck_sprite_api_name);

	return 1;
}

static fck_gameloop_interface bird_game = {
	.name = "Bird Game",
	.create = fck_bird_game_create,
	.destroy = fck_bird_game_destroy,
	.edit = fck_bird_game_edit,
	.tick = fck_bird_game_tick,
};

FCK_EXPORT_API fck_gameloop_interface *fck_bird_game_load(fck_api_registry *apis, void *old)
{
	apis->add(fck_gameloop_interface_name, &bird_game);
	return &bird_game;
}
