
#include "../../db/include/fck_db.h"

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
#include <algorithm>

typedef struct fck_bird_game
{
	kll_allocator *allocator;
} fck_bird_game;

static fck_bird_game *fck_to_bird_game(fck_gameloop loop)
{
	fck_bird_game *game = reinterpret_cast<fck_bird_game *>(loop.handle);
	return game;
}

void *operator new(std::size_t size, kll_allocator *alloc)
{
	void *memory = kll_malloc(alloc, sizeof(fck_bird_game));
	return memory;
}

void operator delete(void *ptr, kll_allocator *alloc)
{
	if(ptr)
	{
		kll_free(alloc, ptr);
	}
}

static fck_gameloop fck_bird_game_create(kll_allocator *allocator, const fck_gameloop_create_parameters *params)
{
	fck_bird_game *game = new (allocator) fck_bird_game();

	game->allocator = allocator;
	const fck_gameloop gameloop = {.handle = (void *)game};
	return gameloop;
}

static void fck_bird_game_destroy(fck_gameloop loop, const fck_gameloop_destroy_parameters *params)
{
	fck_bird_game *game = fck_to_bird_game(loop);
	delete game;
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

template <typename T>
struct fck_component_view
{
	struct iterator
	{
		const fck_entity *current;
		T *data;

		struct reference
		{
			fck_entity entity;
			T &component_data;
		};

		auto operator*() const
		{
			if constexpr (std::is_same<T, fck_entity>::value)
			{
				return *current;
			}
			else
			{
				return reference{*current, data[current->index]};
			}
		}

		iterator &operator++()
		{
			current++;
			return *this;
		}

		bool operator!=(const iterator &other) const
		{
			// In usual forward iteration, other->current == entities + count => end
			return current != other.current;
		}
	};

	const fck_entity *entities;
	T *data;
	fckc_u32 count;

	iterator begin() const
	{
		return {entities + 0, data};
	}
	iterator end() const
	{
		return {entities + count, data};
	}
};

struct fck_component_entity_view
{
	struct iterator
	{
		const fck_entity *current;

		fck_entity operator*() const
		{
			return *current;
		}

		iterator &operator++()
		{
			current++;
			return *this;
		}

		bool operator!=(const iterator &other) const
		{
			return current != other.current;
		}
	};

	const fck_entity *entities;
	fckc_u32 count;

	iterator begin() const
	{
		return {entities + 0};
	}
	iterator end() const
	{
		return {entities + count};
	}
};

template <typename T>
struct fck_component_data_view
{
	struct iterator
	{
		const fck_entity *current;
		T *data;

		T &operator*() const
		{
			return data[current->index];
		}

		iterator &operator++()
		{
			current++;
			return *this;
		}

		bool operator!=(const iterator &other) const
		{
			// In usual forward iteration, other->current == entities + count => end
			return current != other.current;
		}
	};

	const fck_entity *entities;
	T *data;
	fckc_u32 count;

	iterator begin() const
	{
		return {entities + 0, data};
	}
	iterator end() const
	{
		return {entities + count, data};
	}
};


template <typename T>
static fck_component_view<T> fck_make_view(fck_ec *state, fck_ec_api *ec, const char *component_name)
{
	const fck_component_id comp_id = ec->registry->id(*state, component_name);
	const fck_entity *entities;
	const fckc_u32 count = ec->component->dense(*state, comp_id, &entities);
	T *data = reinterpret_cast<T *>(ec->component->buffer(*state, comp_id));

	return fck_component_view<T>{entities, data, count};
}

static auto fck_make_entity_view(fck_ec *state, fck_ec_api *ec, const char *component_name)
{
	const fck_component_id comp_id = ec->registry->id(*state, component_name);
	const fck_entity *entities;
	const fckc_u32 count = ec->component->dense(*state, comp_id, &entities);

	return fck_component_entity_view{entities, count};
}

static int fck_bird_game_tick(fck_gameloop loop, const fck_gameloop_tick_parameters *params)
{
	fck_bird_game *game = fck_to_bird_game(loop);
	fck_ec *state = params->state;
	fck_ec_api *ec = params->ec;

	for (auto [entity, id] : fck_make_view<fck_sprite_id>(params->state, params->ec, "sprite"))
	{
		os->io->log("%d %d", id.batch.value, id.entry.value);
	}

	for (auto id : fck_make_component_view<fck_sprite_id>(params->state, params->ec, "sprite"))
	{
		os->io->log("%d %d", id.batch.value, id.entry.value);
	}

	for (auto entity : fck_make_entity_view(params->state, params->ec, "sprite"))
	{
		os->io->log("%d %d", entity.generation, entity.index);
	}

	std::vector<int> vec;
	std::vector<float> data;

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
