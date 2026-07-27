
extern "C"
{
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
}

#include <string.h>

#include <tuple>
#include <vector>

typedef struct fck_bird_game
{
	kll_allocator *allocator;
} fck_bird_game;

static fck_bird_game *fck_to_bird_game(fck_gameloop loop)
{
	// https://en.cppreference.com/cpp/language/reinterpret_cast
	fck_bird_game *game = reinterpret_cast<fck_bird_game *>(loop.handle);
	return game;
}

static fck_gameloop fck_bird_game_create(kll_allocator *allocator, const fck_gameloop_create_parameters *params)
{
	// Placement-new allocated memory: https://en.cppreference.com/cpp/language/new
	void *memory = kll_malloc(allocator, sizeof(fck_bird_game));
	fck_bird_game *game = new (memory) fck_bird_game();

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

// Reference-ish: https://cppreference.com/cpp/ranges
template <typename T>
struct fck_component_view
{
	// C++ Iterator: https://cppreference.com/cpp/iterator
	struct Iterator
	{
		const fck_entity *entities;
		T *data;
		fckc_u32 index;

		struct Reference
		{
			fck_entity entity;
			T &component_data;
		};

		Reference operator*() const
		{
			const auto entity = entities[index];
			return {entity, data[entity.index]};
		}

		Iterator &operator++()
		{
			++index;
			return *this;
		}

		bool operator!=(const Iterator &other) const
		{
			return index != other.index;
		}
	};

	const fck_entity *entities;
	T *data;
	fckc_u32 count;

	Iterator begin() const
	{
		return {entities, data, 0};
	}
	Iterator end() const
	{
		return {entities, data, count};
	}
};

template <typename T>
static auto fck_make_view(fck_ec *state, fck_ec_api *ec, const char *component_name)
{
	const fck_component_id comp_id = ec->registry->id(*state, component_name);
	const fck_entity *entities;
	const fckc_u32 count = ec->component->dense(*state, comp_id, &entities);
	T *data = reinterpret_cast<T *>(ec->component->buffer(*state, comp_id));

	return fck_component_view<T>{entities, data, count};
}

static int fck_bird_game_tick(fck_gameloop loop, const fck_gameloop_tick_parameters *params)
{
	fck_bird_game *game = fck_to_bird_game(loop);
	fck_ec *state = params->state;
	fck_ec_api *ec = params->ec;

	// Structured Bindings: https://en.cppreference.com/cpp/language/structured_binding
	for (auto [entity, id] : fck_make_view<fck_sprite_id>(params->state, params->ec, "sprite"))
	{
		os->io->log("%d %d", id.batch.value, id.entry.value);
	}
	return 1;
}

static fck_gameloop_interface bird_game = {
	.name = "Bird Game",
	.create = fck_bird_game_create,
	.destroy = fck_bird_game_destroy,
	.edit = fck_bird_game_edit,
	.tick = fck_bird_game_tick,
};

extern "C"
{
	FCK_EXPORT_API fck_gameloop_interface *fck_bird_game_load(fck_api_registry *apis, void *old)
	{
		apis->add(fck_gameloop_interface_name, &bird_game);
		return &bird_game;
	}
}
