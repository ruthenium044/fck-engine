#include "ecs/fck_ecs.h"

static void fck_serialiser_assert_allocated(fck_serialiser const *serialiser)
{
	SDL_assert(serialiser != nullptr);
	SDL_assert(serialiser->data != nullptr);
}

static void fck_serialiser_maybe_resize(fck_serialiser *serialiser, size_t required_size)
{
	SDL_assert(serialiser != nullptr);

	// Fix it with a smart calculation :D
	// Just lazy at the moment
	if (serialiser->capacity < required_size)
	{
		uint8_t *old_mem = serialiser->data;
		uint8_t *new_mem = (uint8_t *)SDL_realloc(serialiser->data, required_size);

		if (new_mem != nullptr)
		{
			serialiser->data = new_mem;
		}
		else
		{
			SDL_LogCritical(0, "Failed to reallocate memory!");
		}

		serialiser->capacity = required_size;
	}
}

void fck_ecs_system_add(fck_ecs *ecs, fck_system_once on_once)
{
	fck_systems_scheduler_push(&ecs->system_scheduler, FCK_ECS_SYSTEM_TYPE_ONCE, (fck_system_generic)on_once);
}

void fck_ecs_flush_system_once(fck_ecs *ecs)
{
	// Pop em once called baddies
	{
		fck_ecs::scheduler_type::systems_type *systems = fck_systems_scheduler_view(&ecs->system_scheduler, FCK_ECS_SYSTEM_TYPE_ONCE);
		for (fck_system *system : systems)
		{
			fck_system_once_info info;
			fck_system_once on_once = (fck_system_once)system->system_function;
			on_once(ecs, &info);
		}
		// POP EM ALL
		systems->count = 0;
	}
}

void fck_ecs_tick(fck_ecs *ecs)
{
	fck_ecs_flush_system_once(ecs);

	// Tick em update baddies
	{
		fck_ecs::scheduler_type::systems_type *systems = fck_systems_scheduler_view(&ecs->system_scheduler, FCK_ECS_SYSTEM_TYPE_UPDATE);
		for (size_t index = 0; index < systems->count; index++)
		{
			fck_ecs_system_state *state = fck_sparse_lookup_view(&ecs->update_system_states, index);
			// We could use an enum mapping based on the state to get a bucket
			// But this is good enough for now!
			if (*state == FCK_ECS_SYSTEM_STATE_DEAD)
			{
				continue;
			}
			fck_system_update_info info{*state};
			fck_system *system = systems->data + index;
			fck_system_update on_update = (fck_system_update)system->system_function;
			on_update(ecs, &info);
			if (info.state == FCK_ECS_SYSTEM_STATE_KILL)
			{
				*state = FCK_ECS_SYSTEM_STATE_DEAD;
			}
			*state = info.state;
		}
	}
}

void fck_ecs_system_add(fck_ecs *ecs, fck_system_update on_update)
{
	fck_systems_scheduler_push(&ecs->system_scheduler, FCK_ECS_SYSTEM_TYPE_UPDATE, (fck_system_generic)on_update);
}

void fck_ecs_snapshot_serialise(fck_serialiser *serialiser, fck_components_header *header, fck_ecs::sparse_array_void *sparse_array)
{
	SDL_assert(serialiser != nullptr);
	if (!header->interface.is_serialise_on)
	{
		return;
	}

	fck_serialise(serialiser, &sparse_array->owner.count);

	fck_serialise(serialiser, sparse_array->owner.data, sparse_array->owner.count);
	if (header->interface.serialise != nullptr)
	{
		header->interface.serialise(serialiser, sparse_array->dense.data, sparse_array->dense.count);
	}
	else
	{
		fck_serialise(serialiser, (uint8_t *)sparse_array->dense.data, sparse_array->dense.count * header->size);
	}
}

void fck_ecs_snapshot_deserialise(fck_serialiser *serialiser, fck_components_header *header, fck_ecs::sparse_array_void *sparse_array)
{
	SDL_assert(serialiser != nullptr);
	if (!header->interface.is_serialise_on)
	{
		return;
	}

	fck_serialise(serialiser, &sparse_array->owner.count);

	fck_serialise(serialiser, sparse_array->owner.data, sparse_array->owner.count);
	sparse_array->dense.count = sparse_array->owner.count;

	if (header->interface.serialise != nullptr)
	{
		header->interface.serialise(serialiser, sparse_array->dense.data, sparse_array->dense.count);
	}
	else
	{
		fck_serialise(serialiser, (uint8_t *)sparse_array->dense.data, sparse_array->dense.count * header->size);
	}
	// THIS will break - it will break so absolutely fucking badly... It is sad
	// This stupid fucking sparse free list...
	// TODO: Fix this
	fck_sparse_lookup_clear(&sparse_array->sparse);
	for (fck_ecs::entity_type index = 0; index < sparse_array->owner.count; index++)
	{
		fck_ecs::entity_type *owner = fck_dense_list_view(&sparse_array->owner, index);
		fck_sparse_lookup_set(&sparse_array->sparse, *owner, &index);
	}
}

void fck_ecs_snapshot_store(fck_ecs *ecs, fck_serialiser *serialiser)
{
	SDL_assert(ecs != nullptr);
	SDL_assert(serialiser != nullptr);

	uint64_t ecs_id = (uint64_t)ecs;
	fck_serialise(serialiser, &ecs_id);

	fck_serialise(serialiser, &ecs->entities.owner.count);
	fck_serialise(serialiser, ecs->entities.owner.data, ecs->entities.owner.count);

	for (fck_item<fck_ecs::component_id, fck_components_header> item : &ecs->component_headers)
	{
		fck_ecs::component_id *id = item.index;
		fck_components_header *header = item.value;
		fck_ecs::sparse_array_void *components = fck_sparse_array_view(&ecs->components, *id);

		fck_ecs_snapshot_serialise(serialiser, header, components);
	}
}

void fck_ecs_snapshot_store_partial(fck_ecs *ecs, fck_serialiser *serialiser, fck_ecs::dense_list<fck_ecs::entity_type> *entities)
{
	SDL_assert(ecs != nullptr);
	SDL_assert(serialiser != nullptr);

	uint64_t ecs_id = (uint64_t)ecs;
	fck_serialise(serialiser, &ecs_id);

	fck_serialise(serialiser, &entities->count);
	fck_serialise(serialiser, entities->data, entities->count);

	for (fck_item<fck_ecs::component_id, fck_components_header> item : &ecs->component_headers)
	{
		fck_ecs::component_id *id = item.index;
		fck_components_header *header = item.value;
		fck_ecs::sparse_array_void *components = fck_sparse_array_view(&ecs->components, *id);

		if (!header->interface.is_serialise_on)
		{
			return;
		}

		fck_ecs::entity_type total = 0;
		size_t total_at = serialiser->at;
		fck_serialise(serialiser, &total);

		for (fck_ecs::entity_type *entity : entities)
		{
			if (fck_sparse_array_exists(components, *entity))
			{
				total = total + 1;
				fck_serialise(serialiser, entity);

				void *data = fck_sparse_array_view_raw(components, *entity, header->size);
				if (header->interface.serialise != nullptr)
				{
					header->interface.serialise(serialiser, data, 1);
				}
				else
				{
					fck_serialise(serialiser, (uint8_t *)data, header->size);
				}
			}
		}

		// hacky overwrite of previously written data
		size_t at = serialiser->at;
		serialiser->at = total_at;
		fck_serialise(serialiser, &total);
		serialiser->at = at;
	}
}

void fck_ecs_snapshot_load(fck_ecs *ecs, fck_serialiser *serialiser)
{
	SDL_assert(ecs != nullptr);

	uint64_t ecs_id = 0;
	fck_serialise(serialiser, &ecs_id);
	SDL_assert(ecs_id != (uint64_t)ecs);

	fck_sparse_list_clear(&ecs->entities);
	// Create a page to temp store entities up to N
	constexpr fck_ecs::entity_type entity_page_size = 32;
	fck_ecs::entity_type entities[entity_page_size];

	// Deserialise the required count
	fck_ecs::entity_type total_entities;
	fck_serialise(serialiser, &total_entities);

	// Cut of slack so we can align to N (32)
	fck_ecs::entity_type slack = total_entities % entity_page_size;
	fck_serialise(serialiser, entities, slack);
	for (fck_ecs::entity_type index = 0; index < slack; index++)
	{
		fck_ecs::entity_type entity = entities[index];
		fck_sparse_list_emplace(&ecs->entities, entity, &entity);
	}

	// Read the remaining data in 32 pages
	for (fck_ecs::entity_type current = slack; current < total_entities; current += entity_page_size)
	{
		fck_serialise(serialiser, entities, entity_page_size);
		for (fck_ecs::entity_type index = 0; index < entity_page_size; index++)
		{
			fck_ecs::entity_type entity = entities[index];
			fck_sparse_list_emplace(&ecs->entities, entity, &entity);
		}
	}

	// This will also break fucking badly. FUCK LOL
	// We need to have some sort of shared manifest OR rely on both sides having a deterministic flow
	// Is it smart that every fucking client needs to manage their own fucking free list? I do not know... Ok ye, it is
	// TODO: Fix this shit too
	for (fck_item<fck_ecs::component_id, fck_components_header> item : &ecs->component_headers)
	{
		fck_ecs::component_id *id = item.index;
		fck_components_header *header = item.value;
		fck_ecs::sparse_array_void *components = fck_sparse_array_view(&ecs->components, *id);

		fck_ecs_snapshot_deserialise(serialiser, header, components);
	}
}

void fck_ecs_snapshot_load_partial(fck_ecs *ecs, fck_serialiser *serialiser)
{
	SDL_assert(ecs != nullptr);
	SDL_assert(serialiser != nullptr);

	uint64_t ecs_id = (uint64_t)ecs;
	fck_serialise(serialiser, &ecs_id);

	// Entities
	// Create a page to temp store entities up to N
	constexpr fck_ecs::entity_type entity_page_size = 32;
	fck_ecs::entity_type entities[entity_page_size];

	// Deserialise the required count
	fck_ecs::entity_type total_entities;
	fck_serialise(serialiser, &total_entities);

	// Cut of slack so we can align to N (32)
	fck_ecs::entity_type slack = total_entities % entity_page_size;
	fck_serialise(serialiser, entities, slack);
	for (fck_ecs::entity_type index = 0; index < slack; index++)
	{
		fck_ecs::entity_type entity = entities[index];
		fck_sparse_list_emplace(&ecs->entities, entity, &entity);
	}

	// Read the remaining data in 32 pages
	for (fck_ecs::entity_type current = slack; current < total_entities; current += entity_page_size)
	{
		fck_serialise(serialiser, entities, entity_page_size);
		for (fck_ecs::entity_type index = 0; index < entity_page_size; index++)
		{
			fck_ecs::entity_type entity = entities[index];
			fck_sparse_list_emplace(&ecs->entities, entity, &entity);
		}
	}

	// Data
	for (fck_item<fck_ecs::component_id, fck_components_header> item : &ecs->component_headers)
	{
		fck_ecs::component_id *id = item.index;
		fck_components_header *header = item.value;
		fck_ecs::sparse_array_void *components = fck_sparse_array_view(&ecs->components, *id);

		if (!header->interface.is_serialise_on)
		{
			return;
		}

		fck_ecs::entity_type total = 0;
		fck_serialise(serialiser, &total);

		for (size_t index = 0; index < total; index++)
		{
			fck_ecs::entity_type entity;
			fck_serialise(serialiser, &entity);

			fck_sparse_array_reserve_raw(components, entity, header->size);

			void *data = fck_sparse_array_view_raw(components, entity, header->size);
			if (header->interface.serialise != nullptr)
			{
				header->interface.serialise(serialiser, data, 1);
			}
			else
			{
				fck_serialise(serialiser, (uint8_t *)data, header->size);
			}
		}
	}
}

// TODO: Make the following typed, nice and dandy in ECS
// Baselines
struct fck_ecs_snapshot
{
	fck_serialiser serialiser;
};

// Deltas
struct fck_ecs_snapshot_delta
{
	fck_serialiser serialiser;
};
// TODO: Make snapshot and delta function interface

void fck_ecs_snapshot_alloc(fck_ecs_snapshot *snapshot)
{
	SDL_assert(snapshot != nullptr);

	fck_serialiser_alloc(&snapshot->serialiser);
}

void fck_ecs_snapshot_free(fck_ecs_snapshot *snapshot)
{
	SDL_assert(snapshot != nullptr);

	fck_serialiser_free(&snapshot->serialiser);
}

void fck_ecs_snapshot_delta_alloc(fck_ecs_snapshot_delta *delta)
{
	SDL_assert(delta != nullptr);

	fck_serialiser_alloc(&delta->serialiser);
}

void fck_ecs_snapshot_delta_free(fck_ecs_snapshot_delta *delta)
{
	SDL_assert(delta != nullptr);

	fck_serialiser_free(&delta->serialiser);
}

void fck_ecs_snapshot_apply_delta(fck_ecs_snapshot const *baseline, fck_ecs_snapshot_delta const *delta, fck_ecs_snapshot *result)
{
	fck_serialiser_assert_allocated(&baseline->serialiser);
	fck_serialiser_assert_allocated(&result->serialiser);
	fck_serialiser_assert_allocated(&delta->serialiser);

	size_t buffer_count = delta->serialiser.at;

	fck_serialiser_maybe_resize(&result->serialiser, buffer_count);

	// Get rid of SDL_malloc, maybe? We can also just reserve, but fuck it for now
	// Should be fck_serialiser_reserve
	int8_t *buffer = (int8_t *)delta->serialiser.data;

	size_t lower_count = SDL_min(baseline->serialiser.at, delta->serialiser.at);
	for (size_t index = 0; index < lower_count; index++)
	{
		buffer[index] = int8_t(baseline->serialiser.data[index]) + int8_t(delta->serialiser.data[index]);
	}

	int64_t delta_snapshot_slack_count = int64_t(delta->serialiser.at) - int64_t(baseline->serialiser.at);
	for (int64_t index = 0; index < delta_snapshot_slack_count; index++)
	{
		size_t at = lower_count + index;
		buffer[at] = int8_t(delta->serialiser.data[at]);
	}

	fck_serialiser_create(&result->serialiser, (uint8_t *)buffer, buffer_count);
}

void fck_ecs_snapshot_compute_delta(fck_ecs_snapshot const *baseline, fck_ecs_snapshot const *current, fck_ecs_snapshot_delta *delta)
{
	fck_serialiser_assert_allocated(&baseline->serialiser);
	fck_serialiser_assert_allocated(&current->serialiser);
	fck_serialiser_assert_allocated(&delta->serialiser);

	size_t buffer_count = current->serialiser.at;

	fck_serialiser_maybe_resize(&delta->serialiser, buffer_count);

	int8_t *buffer = (int8_t *)delta->serialiser.data;

	// Current snapshot == latest snapshot
	// TODO: SPEED IT UP!!!!!
	size_t lower_count = SDL_min(baseline->serialiser.at, current->serialiser.at);
	for (size_t index = 0; index < lower_count; index++)
	{
		buffer[index] = int8_t(current->serialiser.data[index]) - int8_t(baseline->serialiser.data[index]);
	}

	int64_t current_snapshot_slack_count = int64_t(current->serialiser.at) - int64_t(baseline->serialiser.at);
	for (int64_t index = 0; index < current_snapshot_slack_count; index++)
	{
		size_t at = lower_count + index;
		buffer[at] = int8_t(current->serialiser.data[at]);
	}

	fck_serialiser_create(&delta->serialiser, (uint8_t *)buffer, buffer_count);
}

void fck_ecs_delta_example()
{
	fck_ecs ecs;

	// Send()
	// On server, usually we can also call it producer
	fck_ecs_snapshot_delta delta;
	fck_ecs_snapshot_delta_alloc(&delta);
	{
		// Last acknowledged baseline of target client
		// The memory-overhead is O(ecs mem * client count)
		// We can account for that by centralising the baseline
		// Every client shares ONE baseline and everybody gets the same delta
		// The baseline that is acked by all is then the candidate
		fck_ecs_snapshot baseline;
		fck_ecs_snapshot_alloc(&baseline);

		// Current ECS state, the snapshot memory usage is equal to the ECS usage
		fck_ecs_snapshot current;
		fck_ecs_snapshot_alloc(&current);
		fck_ecs_snapshot_store(&ecs, &current.serialiser);

		fck_ecs_snapshot_compute_delta(&baseline, &current, &delta);
	}

	// Send delta through the wire!

	// Recv()
	// On client, usually we can als call it consumer
	{
		fck_ecs_snapshot baseline;
		fck_ecs_snapshot_alloc(&baseline);

		fck_ecs_snapshot current;
		fck_ecs_snapshot_alloc(&current);

		fck_ecs_snapshot_apply_delta(&baseline, &delta, &current);

		fck_ecs_snapshot_load(&ecs, &current.serialiser);
		// Notes: We can not say goodbye to a baseline right away
		// The server sends deltas and its baseline for a client is the last acknowledged baseline
		// To account for that, we hug the baseline and apply deltas to compute a new current
		// As soon as we receive a dela for a new baseline, the client copies current to baseline
		// to work on a new baseline
	}
}