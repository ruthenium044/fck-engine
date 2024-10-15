#ifndef FCK_SYSTEM_INCLUDED
#define FCK_SYSTEM_INCLUDED

#include "fck_iterator.h"
#include "fck_template_utility.h"

// This is all we provide for now
// Timed systems, conditional systems, and so on
// Implement them youselves in application layer
enum fck_ecs_system_type
{
	FCK_ECS_SYSTEM_TYPE_UPDATE,
	FCK_ECS_SYSTEM_TYPE_ONCE,
	FCK_ECS_SYSTEM_TYPE_COUNT
};

enum fck_ecs_system_state
{
	FCK_ECS_SYSTEM_STATE_RUN,
	FCK_ECS_SYSTEM_STATE_WAIT, // NOT IMPLEMENTED
	FCK_ECS_SYSTEM_STATE_KILL,
	FCK_ECS_SYSTEM_STATE_DEAD
};

struct fck_ecs;

struct fck_system_update_info
{
	fck_ecs_system_state state;
};

struct fck_system_once_info
{
};

using fck_system_generic = void (*)(void);
using fck_system_update = void (*)(fck_ecs *ecs, fck_system_update_info *);
using fck_system_once = void (*)(fck_ecs *ecs, fck_system_once_info *);

struct fck_system
{
	fck_system_generic system_function;
};

template <typename index_type>
struct fck_systems
{
	using index_info = fck_indexer_info<index_type>;
	static_assert(index_info::is_implemented, "Index type not implemented");

	fck_system *data;

	index_type capacity;
	index_type count;
};

template <typename index_type>
struct fck_systems_scheduler
{
	using systems_type = fck_systems<index_type>;

	fck_systems<index_type> systems_buckets[FCK_ECS_SYSTEM_TYPE_COUNT];
};

template <typename index_type>
fck_iterator<fck_system> begin(fck_systems<index_type> *systems)
{
	return {systems->data};
}

template <typename index_type>
fck_iterator<fck_system> end(fck_systems<index_type> *systems)
{
	return {systems->data + systems->count};
}

template <typename index_type>
void fck_systems_alloc(fck_systems<index_type> *systems, size_t capacity)
{
	SDL_assert(systems != nullptr);
	SDL_zerop(systems);

	systems->data = (fck_system *)SDL_malloc(sizeof(*systems->data) * capacity);
	systems->capacity = capacity;
}

template <typename index_type>
void fck_systems_push(fck_systems<index_type> *systems, fck_system_generic update)
{
	SDL_assert(systems != nullptr);
	SDL_assert(systems->count < systems->capacity);

	fck_system *system = systems->data + systems->count;
	system->system_function = update;

	systems->count = systems->count + 1;
}

template <typename index_type>
void fck_systems_free(fck_systems<index_type> *systems)
{
	SDL_assert(systems != nullptr);

	SDL_free(systems->data);
	SDL_zerop(systems);
}

template <typename index_type>
void fck_systems_scheduler_alloc(fck_systems_scheduler<index_type> *system_scheduler,
                                 typename fck_ignore_deduction<index_type>::type capacity)
{
	SDL_assert(system_scheduler != nullptr);
	using system_type = typename fck_systems_scheduler<index_type>::systems_type;

	for (size_t index = 0; index < FCK_ECS_SYSTEM_TYPE_COUNT; index++)
	{
		system_type *systems = &system_scheduler->systems_buckets[index];
		fck_systems_alloc(systems, capacity);
	}
}

template <typename index_type>
void fck_systems_scheduler_push(fck_systems_scheduler<index_type> *system_scheduler, fck_ecs_system_type type, fck_system_generic function)
{
	using system_type = typename fck_systems_scheduler<index_type>::systems_type;

	// Maybe other stuff - we will see
	// TODO: good ol fashioned concurrency and scheduling
	system_type *systems = &system_scheduler->systems_buckets[type];
	fck_systems_push(systems, (fck_system_generic)function);
}

template <typename index_type>
fck_systems<index_type> *fck_systems_scheduler_view(fck_systems_scheduler<index_type> *system_scheduler, fck_ecs_system_type type)
{
	using system_type = typename fck_systems_scheduler<index_type>::systems_type;
	return &system_scheduler->systems_buckets[type];
}

template <typename index_type>
void fck_systems_scheduler_free(fck_systems_scheduler<index_type> *system_scheduler)
{
	SDL_assert(system_scheduler != nullptr);
	using system_type = typename fck_systems_scheduler<index_type>::systems_type;

	for (size_t index = 0; index < FCK_ECS_SYSTEM_TYPE_COUNT; index++)
	{
		system_type *systems = &system_scheduler->systems_buckets[index];
		fck_systems_free(systems);
	}
	SDL_zerop(system_scheduler->systems_buckets);
}
#endif // FCK_SYSTEM_INCLUDED