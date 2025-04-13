#include "ecs/fck_ecs.h"
#include "ecs/snapshot/fck_ecs_delta.h"
#include "ecs/snapshot/fck_ecs_snapshot.h"
#include "ecs/snapshot/fck_ecs_timeline.h"
#include "lz4.h"

#if defined(__SSE__) || defined(__AVX2__)
#include <immintrin.h>
#endif

#if defined(__ARM_NEON__)
#include <arm_neon.h>
#endif

void fck_ecs_snapshot_diff(uint8_t const *baseline, uint8_t const *current, uint8_t *delta, size_t count);

void fck_ecs_snapshot_diff8(uint8_t const *baseline, uint8_t const *current, uint8_t *delta, size_t count)
{
	for (size_t index = 0; index < count; index++)
	{
		delta[index] = current[index] ^ baseline[index];
	}
}
void fck_ecs_snapshot_diff16(uint8_t const *baseline, uint8_t const *current, uint8_t *delta, size_t byte_count)
{
	const size_t word_size = 2;
	const size_t word_count = byte_count / word_size;
	const size_t slack_count = byte_count % word_size;

	uint16_t const *baseline_as_16 = (uint16_t const *)baseline;
	uint16_t const *current_as_16 = (uint16_t const *)current;
	uint16_t *delta_as_16 = (uint16_t *)delta;

	for (size_t index = 0; index < word_count; index++)
	{
		delta_as_16[index] = current_as_16[index] ^ baseline_as_16[index];
	}

	size_t offset = word_count * word_size;
	uint8_t const *baseline_next = baseline + offset;
	uint8_t const *current_next = current + offset;
	uint8_t *delta_next = delta + offset;
	fck_ecs_snapshot_diff(baseline_next, current_next, delta_next, slack_count);
}
void fck_ecs_snapshot_diff32(uint8_t const *baseline, uint8_t const *current, uint8_t *delta, size_t byte_count)
{
	const size_t word_size = 4;
	const size_t word_count = byte_count / word_size;
	const size_t slack_count = byte_count % word_size;

	uint32_t const *baseline_as_32 = (uint32_t const *)baseline;
	uint32_t const *current_as_32 = (uint32_t const *)current;
	uint32_t *delta_as_32 = (uint32_t *)delta;

	for (size_t index = 0; index < word_count; index++)
	{
		delta_as_32[index] = current_as_32[index] ^ baseline_as_32[index];
	}

	size_t offset = word_count * word_size;
	uint8_t const *baseline_next = baseline + offset;
	uint8_t const *current_next = current + offset;
	uint8_t *delta_next = delta + offset;
	fck_ecs_snapshot_diff(baseline_next, current_next, delta_next, slack_count);
}
void fck_ecs_snapshot_diff64(uint8_t const *baseline, uint8_t const *current, uint8_t *delta, size_t byte_count)
{
	const size_t word_size = 8;
	const size_t word_count = byte_count / word_size;
	const size_t slack_count = byte_count % word_size;

	uint64_t const *baseline_as_64 = (uint64_t const *)baseline;
	uint64_t const *current_as_64 = (uint64_t const *)current;
	uint64_t *delta_as_64 = (uint64_t *)delta;

	for (size_t index = 0; index < word_count; index++)
	{
		delta_as_64[index] = current_as_64[index] ^ baseline_as_64[index];
	}

	size_t offset = word_count * word_size;
	uint8_t const *baseline_next = baseline + offset;
	uint8_t const *current_next = current + offset;
	uint8_t *delta_next = delta + offset;
	fck_ecs_snapshot_diff(baseline_next, current_next, delta_next, slack_count);
}

#if defined(__SSE__)
#define FCK_SNAPSHOT_HAS_128
void fck_ecs_snapshot_diff128(uint8_t const *baseline, uint8_t const *current, uint8_t *delta, size_t byte_count)
{
	const size_t word_size = 16;
	const size_t word_count = byte_count / word_size;
	const size_t slack_count = byte_count % word_size;
	for (size_t index = 0; index < word_count; index++)
	{
		size_t at = word_size * index;
		__m128i base = _mm_loadu_si128((__m128i *)(baseline + at));
		__m128i curr = _mm_loadu_si128((__m128i *)(current + at));
		__m128i target = _mm_xor_si128(curr, base);
		_mm_storeu_si128((__m128i *)(delta + at), target);
	}
	size_t offset = word_count * word_size;
	uint8_t const *baseline_next = baseline + offset;
	uint8_t const *current_next = current + offset;
	uint8_t *delta_next = delta + offset;

	fck_ecs_snapshot_diff(baseline_next, current_next, delta_next, slack_count);
}
#endif // __SSE__

#if defined(__AVX2__)
#define FCK_SNAPSHOT_HAS_256
void fck_ecs_snapshot_diff256(uint8_t const *baseline, uint8_t const *current, uint8_t *delta, size_t byte_count)
{
	const size_t word_size = 32;
	const size_t word_count = byte_count / word_size;
	const size_t slack_count = byte_count % word_size;
	for (size_t index = 0; index < word_count; index++)
	{
		size_t at = word_size * index;
		__m256i base = _mm256_loadu_si256((__m256i *)(baseline + at));
		__m256i curr = _mm256_loadu_si256((__m256i *)(current + at));
		__m256i target = _mm256_xor_si256(curr, base);
		_mm256_storeu_si256((__m256i *)(delta + at), target);
	}
	size_t offset = word_count * word_size;
	uint8_t const *baseline_next = baseline + offset;
	uint8_t const *current_next = current + offset;
	uint8_t *delta_next = delta + offset;

	fck_ecs_snapshot_diff(baseline_next, current_next, delta_next, slack_count);
}
#endif // __AVX2__

#if defined(__ARM_NEON__)
#define FCK_SNAPSHOT_HAS_128
void fck_ecs_snapshot_diff128(uint8_t const *baseline, uint8_t const *current, uint8_t *delta, size_t byte_count)
{
	const size_t word_size = 16;
	const size_t word_count = byte_count / word_size;
	const size_t slack_count = byte_count % word_size;
	for (size_t index = 0; index < word_count; index++)
	{
		size_t at = word_size * index;
		uint8x16_t base = vld1q_u8(baseline + at);
		uint8x16_t curr = vld1q_u8(current + at);
		uint8x16_t target = veorq_u8(curr, base);
		vst1q_u8(delta + at, target);
	}
	size_t offset = word_count * word_size;
	uint8_t const *baseline_next = baseline + offset;
	uint8_t const *current_next = current + offset;
	uint8_t *delta_next = delta + offset;

	fck_ecs_snapshot_diff(baseline_next, current_next, delta_next, slack_count);
}
#endif // !__ARM_NEON__

#if defined(FCK_SNAPSHOT_HAS_128) || defined(FCK_SNAPSHOT_HAS_256)
#define FCK_SNAPSHOT_HAS_128_OR_256
#endif // FCK_SNAPSHOT_HAS_128 || FCK_SNAPSHOT_HAS_256

void fck_ecs_snapshot_diff(uint8_t const *baseline, uint8_t const *current, uint8_t *delta, size_t byte_count)
{
	if (byte_count == 0)
	{
		return;
	}
	if (byte_count < 2)
	{
		fck_ecs_snapshot_diff8(baseline, current, delta, byte_count);
		return;
	}
	if (byte_count < 4)
	{
		fck_ecs_snapshot_diff16(baseline, current, delta, byte_count);
		return;
	}
	if (byte_count < 8)
	{
		fck_ecs_snapshot_diff32(baseline, current, delta, byte_count);
		return;
	}

#if defined(FCK_SNAPSHOT_HAS_256)
	if (byte_count >= 32)
	{
		fck_ecs_snapshot_diff256(baseline, current, delta, byte_count);
		return;
	}
#endif
#if defined(FCK_SNAPSHOT_HAS_128)
	if (byte_count >= 16)
	{
		fck_ecs_snapshot_diff128(baseline, current, delta, byte_count);
		return;
	}
#endif

	fck_ecs_snapshot_diff64(baseline, current, delta, byte_count);
}

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

static void fck_ecs_component_array_serialise(fck_serialiser *serialiser, fck_components_header *header,
                                              fck_ecs::sparse_array_void *sparse_array)
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

static void fck_ecs_component_array_deserialise(fck_serialiser *serialiser, fck_components_header *header,
                                                fck_ecs::sparse_array_void *sparse_array)
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

void fck_ecs_serialise(fck_ecs *ecs, fck_serialiser *serialiser)
{
	SDL_assert(ecs != nullptr);
	SDL_assert(serialiser != nullptr);
	SDL_assert(fck_serialiser_is_byte_writer(serialiser));

	uint64_t ecs_id = (uint64_t)ecs;
	fck_serialise(serialiser, &ecs_id);

	fck_serialise(serialiser, &ecs->entities.owner.count);
	fck_serialise(serialiser, ecs->entities.owner.data, ecs->entities.owner.count);

	for (fck_item<fck_ecs::component_id, fck_components_header> item : &ecs->component_headers)
	{
		fck_ecs::component_id *id = item.index;
		fck_components_header *header = item.value;
		fck_ecs::sparse_array_void *components = fck_sparse_array_view(&ecs->components, *id);

		fck_ecs_component_array_serialise(serialiser, header, components);
	}
}
// Serialise partial
void fck_ecs_serialise_partial(fck_ecs *ecs, fck_serialiser *serialiser, fck_ecs::dense_list<fck_ecs::entity_type> *entities)
{
	SDL_assert(ecs != nullptr);
	SDL_assert(serialiser != nullptr);
	SDL_assert(fck_serialiser_is_byte_writer(serialiser));

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

void fck_ecs_deserialise(fck_ecs *ecs, fck_serialiser *serialiser)
{
	SDL_assert(ecs != nullptr);
	SDL_assert(serialiser != nullptr);
	SDL_assert(fck_serialiser_is_byte_reader(serialiser));

	uint64_t ecs_id = 0;
	fck_serialise(serialiser, &ecs_id);
	// SDL_assert(ecs_id != (uint64_t)ecs);

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

		fck_ecs_component_array_deserialise(serialiser, header, components);
	}
}

void fck_ecs_deserialise_partial(fck_ecs *ecs, fck_serialiser *serialiser)
{
	SDL_assert(ecs != nullptr);
	SDL_assert(serialiser != nullptr);
	SDL_assert(fck_serialiser_is_byte_reader(serialiser));

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

void fck_ecs_snapshot_free(fck_serialiser *serialiser)
{
	SDL_assert(serialiser != nullptr);

	fck_serialiser_free(serialiser);
}

void fck_ecs_snapshot_alloc(fck_ecs_snapshot *snapshot)
{
	SDL_assert(snapshot != nullptr);

	snapshot->seq = 0;
	fck_serialiser_alloc(&snapshot->serialiser);
}

void fck_ecs_snapshot_free(fck_ecs_snapshot *snapshot)
{
	SDL_assert(snapshot != nullptr);

	fck_serialiser_free(&snapshot->serialiser);
}

void fck_ecs_delta_alloc(fck_ecs_delta *delta)
{
	SDL_assert(delta != nullptr);

	fck_serialiser_alloc(&delta->serialiser);
}

void fck_ecs_delta_free(fck_ecs_delta *delta)
{
	SDL_assert(delta != nullptr);

	fck_serialiser_free(&delta->serialiser);
}

void fck_ecs_snapshot_capture_delta(fck_ecs_snapshot const *baseline, fck_ecs_snapshot const *current, fck_ecs_delta *delta)
{
	fck_serialiser_assert_allocated(&baseline->serialiser);
	fck_serialiser_assert_allocated(&current->serialiser);
	fck_serialiser_assert_allocated(&delta->serialiser);

	size_t buffer_count = current->serialiser.at;

	fck_serialiser_maybe_resize(&delta->serialiser, buffer_count);

	uint8_t *buffer = delta->serialiser.data;

	// Current snapshot == latest snapshot
	size_t lower_count = SDL_min(baseline->serialiser.at, current->serialiser.at);
	fck_ecs_snapshot_diff(baseline->serialiser.data, current->serialiser.data, buffer, lower_count);

	int64_t current_snapshot_slack_count = int64_t(current->serialiser.at) - int64_t(baseline->serialiser.at);
	SDL_memcpy(buffer + lower_count, current->serialiser.data + lower_count, current_snapshot_slack_count);

	delta->serialiser.at = buffer_count;
	delta->serialiser.data = buffer;
}

void fck_ecs_snapshot_apply_delta(fck_ecs_snapshot const *baseline, fck_ecs_delta const *delta, fck_ecs_snapshot *result)
{
	fck_serialiser_assert_allocated(&baseline->serialiser);
	fck_serialiser_assert_allocated(&result->serialiser);
	fck_serialiser_assert_allocated(&delta->serialiser);

	size_t buffer_count = delta->serialiser.at;

	fck_serialiser_maybe_resize(&result->serialiser, buffer_count);

	// Get rid of SDL_malloc, maybe? We can also just reserve, but fuck it for now
	// Should be fck_serialiser_reserve
	uint8_t *buffer = result->serialiser.data;

	size_t lower_count = SDL_min(baseline->serialiser.at, delta->serialiser.at);
	fck_ecs_snapshot_diff(baseline->serialiser.data, delta->serialiser.data, buffer, lower_count);

	int64_t delta_snapshot_slack_count = int64_t(delta->serialiser.at) - int64_t(baseline->serialiser.at);
	SDL_memcpy(buffer + lower_count, delta->serialiser.data + lower_count, delta_snapshot_slack_count);

	result->serialiser.at = buffer_count;
	result->serialiser.data = buffer;
}

void fck_ecs_timeline_alloc(fck_ecs_timeline *timeline, size_t snapshot_capacity, size_t delta_capacity)
{
	SDL_assert(timeline != nullptr);

	timeline->snapshots = (fck_ecs_snapshot *)SDL_malloc(snapshot_capacity * sizeof(*timeline->snapshots));
	timeline->snapshot_head = 0;
	timeline->snapshot_capacity = snapshot_capacity;

	timeline->deltas = (fck_ecs_delta *)SDL_malloc(delta_capacity * sizeof(*timeline->deltas));
	timeline->delta_head = 0;
	timeline->delta_capacity = delta_capacity;

	SDL_zero(timeline->protocol);

	for (size_t index = 0; index < timeline->snapshot_capacity; index++)
	{
		fck_ecs_snapshot *snapshot = timeline->snapshots + index;
		fck_ecs_snapshot_alloc(snapshot);
	}

	for (size_t index = 0; index < timeline->delta_capacity; index++)
	{
		fck_ecs_delta *delta = timeline->deltas + index;
		fck_ecs_delta_alloc(delta);
	}
}

void fck_ecs_timeline_free(fck_ecs_timeline *timeline)
{
	SDL_assert(timeline != nullptr);
	SDL_assert(timeline->snapshots != nullptr);

	for (size_t index = 0; index < timeline->delta_capacity; index++)
	{
		fck_ecs_delta *delta = timeline->deltas + index;
		fck_ecs_delta_free(delta);
	}

	for (size_t index = 0; index < timeline->snapshot_capacity; index++)
	{
		fck_ecs_snapshot *snapshot = timeline->snapshots + index;
		fck_ecs_snapshot_free(snapshot);
	}

	SDL_free(timeline->snapshots);

	SDL_zerop(timeline);
}

fck_ecs_snapshot *fck_ecs_timeline_capture(fck_ecs_timeline *timeline, fck_ecs *ecs)
{
	SDL_assert(timeline != nullptr);
	SDL_assert(timeline->snapshots != nullptr);

	uint32_t id = timeline->snapshot_head + 1;
	fck_ecs_snapshot *free = timeline->snapshots + id;
	fck_serialiser *serialiser = &free->serialiser;
	fck_serialiser_reset(serialiser);
	fck_serialiser_byte_writer(&serialiser->self);

	fck_ecs_serialise(ecs, serialiser);

	free->seq = id;

	const size_t capacity_exclude_empty = timeline->snapshot_capacity - 1;
	timeline->snapshot_head = (timeline->snapshot_head + 1) % capacity_exclude_empty;
	return free;
}

fck_ecs_snapshot *fck_ecs_timeline_reserve_empty(fck_ecs_timeline *timeline, size_t seq)
{
	SDL_assert(timeline != nullptr);
	SDL_assert(timeline->snapshots != nullptr);

	size_t offset = seq % timeline->snapshot_capacity;

	fck_ecs_snapshot *free = timeline->snapshots + offset;
	fck_serialiser *serialiser = &free->serialiser;
	fck_serialiser_reset(serialiser);
	fck_serialiser_byte_writer(&serialiser->self);

	free->seq = seq;

	return free;
}

fck_ecs_snapshot *fck_ecs_timeline_snapshot_view(fck_ecs_timeline *timeline, size_t seq)
{
	SDL_assert(timeline != nullptr);
	SDL_assert(timeline->snapshots != nullptr);

	// Should work
	size_t offset = seq % timeline->snapshot_capacity;
	fck_ecs_snapshot *snapshot = timeline->snapshots + offset;

	return snapshot;
}

void fck_ecs_tl_protocol_send(fck_ecs_tl_protocol* protocol, uint32_t send)
{
	protocol->send = send;
}

void fck_ecs_tl_protocol_ack(fck_ecs_tl_protocol* protocol, uint32_t ackd)
{
	protocol->ackd = ackd;
}

fck_ecs_delta* fck_ecs_tl_protocol_capture(fck_ecs_tl_protocol* protocol, fck_ecs_timeline* timeline, fck_ecs* ecs)
{
	SDL_assert(protocol != nullptr);
	SDL_assert(timeline != nullptr);
	SDL_assert(timeline->snapshots != nullptr);
	SDL_assert(ecs != nullptr);

	fck_ecs_snapshot* current = fck_ecs_timeline_capture(timeline, ecs);
	fck_ecs_tl_protocol_send(protocol, current->seq);

	fck_ecs_snapshot* baseline = fck_ecs_timeline_snapshot_view(timeline, protocol->ackd);

	fck_ecs_delta* candidate_delta = timeline->deltas + timeline->delta_head;
	timeline->delta_head = (timeline->delta_head + 1) % timeline->delta_capacity;

	candidate_delta->baseline = baseline->seq;
	candidate_delta->current = current->seq;


	fck_serialiser* serialiser = &candidate_delta->serialiser;
	fck_serialiser_reset(serialiser);
	fck_serialiser_byte_writer(&serialiser->self);
	fck_ecs_snapshot_capture_delta(baseline, current, candidate_delta);

	return candidate_delta;
}

void fck_ecs_tl_protocol_apply(fck_ecs_tl_protocol* protocol, fck_ecs_timeline* timeline, fck_ecs* ecs, fck_ecs_delta const* delta)
{
	SDL_assert(protocol != nullptr);
	SDL_assert(timeline != nullptr);
	SDL_assert(timeline->snapshots != nullptr);
	SDL_assert(ecs != nullptr);
	SDL_assert(delta != nullptr);

	fck_ecs_snapshot* current = fck_ecs_timeline_reserve_empty(timeline, delta->current);
	fck_ecs_tl_protocol_ack(protocol, delta->current);

	fck_ecs_snapshot* baseline = fck_ecs_timeline_snapshot_view(timeline, delta->baseline);

	fck_ecs_snapshot_apply_delta(baseline, delta, current);

	fck_serialiser* serialiser = &current->serialiser;
	fck_serialiser_reset(serialiser);
	fck_serialiser_byte_reader(&serialiser->self);
	fck_ecs_deserialise(ecs, serialiser);
}


void fck_ecs_timeline_delta_capture(fck_ecs_timeline *timeline, fck_ecs *ecs, fck_serialiser * serialiser)
{
	SDL_assert(serialiser != nullptr);
	SDL_assert(fck_serialiser_is_byte_writer(serialiser));
	SDL_assert(timeline != nullptr);
	SDL_assert(timeline->snapshots != nullptr);
	SDL_assert(ecs != nullptr);

	fck_ecs_delta* delta = fck_ecs_tl_protocol_capture(&timeline->protocol, timeline, ecs);
	fck_serialise(serialiser, &delta->current);
	fck_serialise(serialiser, &delta->baseline);
	fck_serialise(serialiser, &delta->serialiser.at);
	fck_serialise(serialiser, &delta->serialiser);
}

void fck_ecs_timeline_delta_apply(fck_ecs_timeline *timeline, fck_ecs *ecs, fck_serialiser * serialiser)
{
	SDL_assert(serialiser != nullptr);
	SDL_assert(fck_serialiser_is_byte_reader(serialiser));
	SDL_assert(timeline != nullptr);
	SDL_assert(timeline->snapshots != nullptr);
	SDL_assert(ecs != nullptr);

	fck_ecs_delta delta;
	fck_serialise(serialiser, &delta.current);
	fck_serialise(serialiser, &delta.baseline);
	fck_serialise(serialiser, &delta.serialiser.at);
	fck_serialiser_create_full(&delta.serialiser, serialiser->data + serialiser->at, delta.serialiser.at);

	fck_ecs_tl_protocol_apply(&timeline->protocol, timeline, ecs, &delta);
}

void fck_ecs_timeline_delta_ack(fck_ecs_timeline *timeline, uint32_t snapshot_index)
{
	timeline->protocol.ackd = snapshot_index;
}

void fck_ecs_delta_example()
{
	fck_ecs ecs;
	fck_ecs_alloc_info ecs_alloc_info{128, 64, 4};
	fck_ecs_alloc(&ecs, &ecs_alloc_info);
	for (int index = 0; index < 64; index++)
	{
		fck_ecs::entity_type entity = fck_ecs_entity_create(&ecs);
		*fck_ecs_component_create<int>(&ecs, entity) = 10;
		*fck_ecs_component_create<float>(&ecs, entity) = 42.0f;
		*fck_ecs_component_create<double>(&ecs, entity) = 69.0;
		*fck_ecs_component_create<char>(&ecs, entity) = 'b';
		*fck_ecs_component_create<bool>(&ecs, entity) = true;
	}

	fck_ecs_timeline timeline;
	fck_ecs_timeline_alloc(&timeline, 8, 4);

	// fck_ecs_delta *delta = fck_ecs_timeline_delta_capture(&timeline, &ecs);
	fck_ecs_snapshot current_send = {};

	fck_ecs_delta delta;
	fck_ecs_delta_alloc(&delta);
	{
		// Last acknowledged baseline of target client
		// The memory-overhead is O(ecs mem * client count)
		// We can account for that by centralising the baseline
		// Every client shares ONE baseline and everybody gets the same delta
		// The baseline that is acked by all is then the candidate
		fck_ecs_snapshot baseline;
		fck_ecs_snapshot_alloc(&baseline);

		// Current ECS state, the snapshot memory usage is equal to the ECS usage
		fck_ecs_snapshot_alloc(&current_send);

		fck_serialiser_byte_writer(&delta.serialiser.self);
		fck_serialiser_byte_writer(&current_send.serialiser.self);

		fck_ecs_serialise(&ecs, &current_send.serialiser);

		fck_ecs_snapshot_capture_delta(&baseline, &current_send, &delta);
	}

	// Send delta through the wire!

	fck_ecs_snapshot current_recv = {};
	// Recv()
	// On client, usually we can als call it consumer
	{
		fck_ecs_snapshot baseline;
		fck_ecs_snapshot_alloc(&baseline);

		fck_ecs_snapshot_alloc(&current_recv);

		fck_ecs_snapshot_apply_delta(&baseline, &delta, &current_recv);

		fck_serialiser_byte_reader(&current_recv.serialiser.self);

		fck_serialiser_reset(&current_recv.serialiser);

		fck_ecs_deserialise(&ecs, &current_recv.serialiser);
		// Notes: We can not say goodbye to a baseline right away
		// The server sends deltas and its baseline for a client is the last acknowledged baseline
		// To account for that, we hug the baseline and apply deltas to compute a new current
		// As soon as we receive a dela for a new baseline, the client copies current to baseline
		// to work on a new baseline
	}
	SDL_assert(current_recv.serialiser.at == current_send.serialiser.at);
}
