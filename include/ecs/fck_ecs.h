#ifndef FCK_ECS_INCLUDED
#define FCK_ECS_INCLUDED

#include <SDL3/SDL_assert.h>

#include "fck_checks.h"
#include "fck_ecs_component_traits.h"
#include "fck_serialiser.h"
#include "fck_sparse_array.h"
#include "fck_sparse_arrays.h"
#include "fck_sparse_list.h"
#include "fck_system.h"
#include "fck_tuple.h"
#include "fck_type_pack.h"
#include "fck_uniform_buffer.h"

template <typename value_type>
struct fck_components_interface
{
	fck_ecs_component_free<value_type> free;
	fck_ecs_component_serialise<value_type> serialise;

	bool is_serialise_on;
};

using fck_components_interface_void = fck_components_interface<void>;

struct fck_components_header
{
	size_t size;
	fck_components_interface_void interface;
};

template <typename index_type>
using fck_sparse_array_void_type = fck_sparse_array<index_type, void>;

struct fck_ecs_alloc_info
{
	using entity_type = uint32_t;
	using component_id = uint16_t;
	using system_id = uint16_t;

	entity_type entity_capacity;
	component_id component_capacity;
	system_id system_capacity;
};

struct fck_ecs
{
	using entity_type = fck_ecs_alloc_info::entity_type;
	using component_id = fck_ecs_alloc_info::component_id;
	using system_id = fck_ecs_alloc_info::system_id;

	using sparse_array_void = fck_sparse_array_void_type<entity_type>;
	using scheduler_type = fck_systems_scheduler<system_id>;

	template <typename value_type>
	using sparse_array = fck_sparse_array<entity_type, value_type>;

	template <typename value_type>
	using dense_list = fck_dense_list<entity_type, value_type>;
	using entity_list = dense_list<entity_type>;

	fck_ecs_alloc_info alloc_info;

	// Unique type data - Singleton ECS data
	fck_uniform_buffer uniform_buffer;

	// The value type should and could become somthing more useful!
	// Entities created through this functionality might receive some extras
	// I do not know - We now have two concepts:
	// - Internally created entities
	// - Externally created entities... oh boy
	// Oh what a fucking nightmare this is...
	// If user emplaces entities externally just for fun and then uses the entity create function
	// the user will just overwrite previously written data... Well, we can flag the interface at one point
	// that way the user must declare how the interactions with the ECS should look like
	fck_sparse_list<entity_type, entity_type> entities;

	// Duplicate count and capacity (overhead)
	fck_sparse_array<component_id, fck_components_header> component_headers; // owner plus content serialisation
	fck_sparse_array<component_id, sparse_array_void> components;            // owner plus content serialisation

	// probably no scheduler... more like buckets atm, hehe
	fck_sparse_lookup<system_id, fck_ecs_system_state> update_system_states;
	fck_systems_scheduler<system_id> system_scheduler;
};

inline void fck_ecs_snapshot_serialise(fck_serialiser *serialiser, fck_components_header *header, fck_ecs::sparse_array_void *sparse_array)
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

inline void fck_ecs_snapshot_deserialise(fck_serialiser *serialiser, fck_components_header *header,
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

inline void fck_ecs_alloc(fck_ecs *ecs, fck_ecs_alloc_info const *info)
{
	SDL_assert(ecs != nullptr);
	SDL_zerop(ecs);

	ecs->alloc_info = *info;

	// This should grow by itself as needed!!
	fck_uniform_buffer_alloc_info buffer_alloc_info = {4096, 32};
	fck_uniform_buffer_alloc(&ecs->uniform_buffer, &buffer_alloc_info);

	fck_sparse_array_alloc(&ecs->component_headers, info->component_capacity);
	fck_sparse_array_alloc(&ecs->components, info->component_capacity);
	fck_sparse_list_alloc(&ecs->entities, info->entity_capacity);
	fck_sparse_lookup_alloc(&ecs->update_system_states, info->system_capacity, FCK_ECS_SYSTEM_STATE_RUN);

	fck_systems_scheduler_alloc(&ecs->system_scheduler, info->system_capacity);
}

inline void fck_ecs_free(fck_ecs *ecs)
{
	SDL_assert(ecs != nullptr);

	for (fck_item<fck_ecs::component_id, fck_components_header> item : &ecs->component_headers)
	{
		fck_ecs::component_id *id = item.index;
		fck_components_header *header = item.value;
		fck_ecs::sparse_array_void *components = fck_sparse_array_view(&ecs->components, *id);
		fck_components_interface_void *interface = &header->interface;
		if (interface->free != nullptr)
		{
			for (fck_ecs::entity_type entity = 0; entity < components->dense.count; entity++)
			{
				void *data = fck_dense_list_view_raw(&components->dense, entity, header->size);
				interface->free(data);
			}
		}

		fck_sparse_array_free(components);
	}

	fck_uniform_buffer_free(&ecs->uniform_buffer);
	fck_sparse_array_free(&ecs->component_headers);
	fck_sparse_array_free(&ecs->components);
	fck_sparse_list_free(&ecs->entities);
	fck_sparse_lookup_free(&ecs->update_system_states);

	fck_systems_scheduler_free(&ecs->system_scheduler);

	SDL_zerop(ecs);
}

template <typename type>
type *fck_ecs_unique_set(fck_ecs *ecs, type const *value, fck_uniform_buffer_cleanup<type> destructor = nullptr)
{
	SDL_assert(ecs != nullptr);
	return fck_uniform_buffer_set(&ecs->uniform_buffer, value, destructor);
}

template <typename type>
type *fck_ecs_unique_create(fck_ecs *ecs, fck_uniform_buffer_cleanup<type> destructor = nullptr)
{
	SDL_assert(ecs != nullptr);
	return fck_uniform_buffer_set_empty<type>(&ecs->uniform_buffer, destructor);
}

template <typename type>
type *fck_ecs_unique_view(fck_ecs *ecs)
{
	SDL_assert(ecs != nullptr);
	return fck_uniform_buffer_view<type>(&ecs->uniform_buffer);
}

inline void fck_ecs_unique_clear_all(fck_ecs *ecs)
{
	fck_uniform_buffer_clear(&ecs->uniform_buffer);
}

inline fck_ecs::entity_type fck_ecs_entity_create(fck_ecs *ecs)
{
	SDL_assert(ecs != nullptr);

	fck_ecs::entity_type entity = fck_sparse_list_add(&ecs->entities, &ecs->entities.dense.count);

	return entity;
}

inline bool fck_ecs_entity_exists(fck_ecs *ecs, fck_ecs::entity_type entity)
{
	return fck_sparse_list_exists(&ecs->entities, entity);
}

inline void fck_ecs_entity_emplace(fck_ecs *ecs, fck_ecs::entity_type entity)
{
	SDL_assert(ecs != nullptr);

	if (!fck_ecs_entity_exists(ecs, entity))
	{
		fck_sparse_list_emplace(&ecs->entities, entity, &ecs->entities.dense.count);
	}
}

inline void fck_ecs_entity_destroy(fck_ecs *ecs, fck_ecs::entity_type entity)
{
	SDL_assert(ecs != nullptr);

	if (!fck_ecs_entity_exists(ecs, entity))
	{
		return;
	}

	for (fck_item<fck_ecs::component_id, fck_components_header> item : &ecs->component_headers)
	{
		fck_ecs::component_id *id = item.index;
		fck_components_header *header = item.value;
		fck_ecs::sparse_array_void *components = fck_sparse_array_view(&ecs->components, *id);
		fck_components_interface_void *interface = &header->interface;
		if (fck_sparse_array_exists(components, entity))
		{
			if (interface->free != nullptr)
			{
				void *data = fck_sparse_array_view_raw(components, entity, header->size);
				interface->free(data);
			}
			fck_sparse_array_remove_raw(components, entity, header->size);
		}
	}

	fck_sparse_list_remove(&ecs->entities, entity);
}

inline void fck_ecs_entity_destroy_all(fck_ecs *ecs)
{
	SDL_assert(ecs != nullptr);

	for (fck_item<fck_ecs::component_id, fck_components_header> item : &ecs->component_headers)
	{
		fck_ecs::component_id *id = item.index;
		fck_components_header *header = item.value;
		fck_ecs::sparse_array_void *components = fck_sparse_array_view(&ecs->components, *id);
		fck_components_interface_void *interface = &header->interface;
		if (interface->free != nullptr)
		{
			for (fck_ecs::entity_type entity = 0; entity < components->dense.count; entity++)
			{
				void *data = fck_dense_list_view_raw(&components->dense, entity, header->size);
				interface->free(data);
			}
		}

		fck_sparse_array_clear(components);
	}

	fck_sparse_list_clear(&ecs->entities);
}

inline void fck_ecs_snapshot_store(fck_ecs *ecs, fck_serialiser *serialiser)
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

inline void fck_ecs_snapshot_store_partial(fck_ecs *ecs, fck_serialiser *serialiser, fck_ecs::dense_list<fck_ecs::entity_type> *entities)
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

inline void fck_ecs_snapshot_load(fck_ecs *ecs, fck_serialiser *serialiser)
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

inline void fck_ecs_snapshot_load_partial(fck_ecs *ecs, fck_serialiser *serialiser)
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

inline void fck_ecs_snapshot_free(fck_serialiser *serialiser)
{
	SDL_assert(serialiser != nullptr);

	fck_serialiser_free(serialiser);
}

template <typename type>
uint16_t fck_unique_id_get()
{
	constexpr uint16_t unique_id = uint16_t(~0u);
	static uint16_t value = fck_count_up_and_get<unique_id>();
	return value;
}

template <typename type>
void fck_ecs_component_remove(fck_ecs *ecs, fck_ecs::entity_type index)
{
	SDL_assert(ecs != nullptr);

	fck_ecs::component_id component_id = fck_unique_id_get<type>();
	fck_ecs::sparse_array_void *out_ptr;
	if (!fck_sparse_array_try_view(&ecs->components, component_id, &out_ptr))
	{
		return;
	}

	fck_ecs::sparse_array<type> *components = (fck_ecs::sparse_array<type> *)out_ptr;
	fck_sparse_array_remove(components, index);
}

template <typename type>
inline void fck_ecs_component_clear(fck_ecs *ecs)
{
	SDL_assert(ecs != nullptr);

	fck_ecs::component_id component_id = fck_unique_id_get<type>();
	fck_components_header *header;
	if (!fck_sparse_array_try_view(&ecs->component_headers, component_id, &header))
	{
		return;
	}

	fck_ecs::sparse_array_void *out_ptr;
	if (!fck_sparse_array_try_view(&ecs->components, component_id, &out_ptr))
	{
		return;
	}

	fck_ecs::sparse_array<type> *components = (fck_ecs::sparse_array<type> *)out_ptr;

	if (header->interface.free != nullptr)
	{
		for (type *component : &components->dense)
		{
			header->interface.free(component);
		}
	}

	fck_sparse_array_clear(components);
}

template <typename type>
void fck_ecs_component_interface_override(fck_ecs *ecs, fck_components_interface<type> const *interface)
{
	fck_ecs::component_id component_id = fck_unique_id_get<type>();
	fck_components_header *out_header;
	if (!fck_sparse_array_try_view(&ecs->component_headers, component_id, &out_header))
	{
		fck_components_header header;
		SDL_zero(header);
		header.size = sizeof(type);
		out_header = fck_sparse_array_emplace(&ecs->component_headers, component_id, &header);
	}

	out_header->interface = *(fck_components_interface_void const *)interface;
}

inline fck_ecs::entity_type fck_ecs_entities_capacity(fck_ecs const *ecs)
{
	SDL_assert(ecs != nullptr);
	return ecs->entities.dense.capacity;
}

template <typename type>
fck_ecs::sparse_array<type> *fck_ecs_component_register(fck_ecs *ecs)
{
	SDL_assert(ecs != nullptr);

	fck_ecs::component_id component_id = fck_unique_id_get<type>();
	fck_ecs::sparse_array_void *out_ptr;
	if (!fck_sparse_array_try_view(&ecs->components, component_id, &out_ptr))
	{
		fck_components_interface<type> interface;
		SDL_zero(interface);
		interface.free = fck_ecs_free_trait<type>::template fetch<type>();
		interface.serialise = fck_ecs_serialise_trait<type>::template fetch<type>();
		interface.is_serialise_on = !fck_ecs_serialise_is_on<type>::value;
		// TODO: More traits!!

		fck_ecs_component_interface_override(ecs, &interface);

		fck_ecs::sparse_array<type> components;
		fck_sparse_array_alloc(&components, fck_ecs_entities_capacity(ecs));
		fck_ecs::sparse_array_void *as_void = (fck_ecs::sparse_array_void *)&components;
		out_ptr = fck_sparse_array_emplace(&ecs->components, component_id, as_void);
	}
	return (fck_ecs::sparse_array<type> *)out_ptr;
}

template <typename type>
type *fck_ecs_component_create(fck_ecs *ecs, fck_ecs::entity_type index)
{
	// Make this function the center point so we do not have the same code in different bodies lying around!!

	// Very similar to set, might be useful to refactor this into one internal set... but also, not having so much noise is nice
	SDL_assert(ecs != nullptr);
	fck_ecs::sparse_array<type> *components = fck_ecs_component_register<type>(ecs);

	// emplacing and checking is so cheap, we can just do it again
	// The alternative is checking whether it exists and throw or return if not...
	// But in this case we still access the same data and have the same overhead
	fck_ecs_entity_emplace(ecs, index);

	fck_sparse_array_emplace_empty(components, index);
	return fck_sparse_array_view(components, index);
}

template <typename type>
bool fck_ecs_component_exists(fck_ecs *ecs, fck_ecs::entity_type index)
{
	SDL_assert(ecs != nullptr);

	fck_ecs::sparse_array<type> *components = fck_ecs_component_register<type>(ecs);

	fck_ecs_entity_emplace(ecs, index);

	return fck_sparse_array_exists(components, index);
}

template <typename type>
void fck_ecs_component_set(fck_ecs *ecs, fck_ecs::entity_type index, type const *value)
{
	SDL_assert(ecs != nullptr);

	fck_ecs::sparse_array<type> *components = fck_ecs_component_register<type>(ecs);

	fck_ecs_entity_emplace(ecs, index);

	fck_sparse_array_emplace(components, index, value);
}

template <typename type>
void fck_ecs_fetch_single(fck_ecs *table, fck_ecs::sparse_array<type> **out_array_ptr)
{
	fck_ecs::component_id component_id = fck_unique_id_get<type>();
	fck_ecs::sparse_array_void *out_ptr;

	*out_array_ptr = nullptr;
	if (fck_sparse_array_try_view(&table->components, component_id, &out_ptr))
	{
		*out_array_ptr = (fck_ecs::sparse_array<type> *)out_ptr;
	}
}

inline void fck_ecs_fetch_multi(fck_ecs *table, fck_tuple<> *tuple)
{
}

template <typename head_type, typename... types>
void fck_ecs_fetch_multi(
	fck_ecs *table,
	fck_tuple<fck_sparse_array<fck_ecs::entity_type, head_type> *, fck_sparse_array<fck_ecs::entity_type, types> *...> *tuple)
{

	fck_ecs::sparse_array<head_type> *current;
	fck_ecs_fetch_single<head_type>(table, &current);
	tuple->value = current;
	fck_ecs_fetch_multi(table, (fck_tuple<fck_sparse_array<fck_ecs::entity_type, types> *...> *)tuple);
}

template <typename... types>
fck_sparse_arrays<fck_ecs::entity_type, types...> fck_ecs_view(fck_ecs *table)
{
	fck_sparse_arrays<fck_ecs::entity_type, types...> result;
	SDL_zero(result);

	fck_ecs_fetch_multi(table, &result.tuple);

	return result;
}

template <typename type>
fck_ecs::sparse_array<type> *fck_ecs_view_single(fck_ecs *table)
{
	fck_ecs::sparse_array<type> *result;
	SDL_zero(result);

	fck_ecs_fetch_single(table, &result);

	return result;
}

inline void fck_ecs_system_add(fck_ecs *ecs, fck_system_update on_update)
{
	fck_systems_scheduler_push(&ecs->system_scheduler, FCK_ECS_SYSTEM_TYPE_UPDATE, (fck_system_generic)on_update);
}

inline void fck_ecs_system_add(fck_ecs *ecs, fck_system_once on_once)
{
	fck_systems_scheduler_push(&ecs->system_scheduler, FCK_ECS_SYSTEM_TYPE_ONCE, (fck_system_generic)on_once);
}

inline void fck_ecs_flush_system_once(fck_ecs *ecs)
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

inline void fck_ecs_tick(fck_ecs *ecs)
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

template <typename... types>
fck_sparse_arrays<fck_ecs::entity_type, types...> fck_ecs_view_from_type_pack(fck_ecs *ecs, fck_type_pack<types...>)
{
	return fck_ecs_view<types...>(ecs);
}

// If passing down a member function falls, wrap a lambda around it
// By design, member functions do not exist in this API
// but we have lambdas and (static) free-functions!
template <typename function>
void fck_ecs_apply(fck_ecs *ecs, function func)
{
	SDL_assert(ecs != nullptr);

	using traits = fck_function_traits<function>;

	constexpr bool has_enough_args = traits::argument_count > 0;
	constexpr bool has_legal_return_type = fck_is_same<typename traits::return_type, void>::value;
	static_assert(has_enough_args, "Cannot invoke with zero function arguments");
	static_assert(has_legal_return_type, "Return type must be void!");

	if constexpr (has_enough_args && has_legal_return_type)
	{
		auto arrays = fck_ecs_view_from_type_pack(ecs, typename traits::arguments_no_pointer{});

		fck_sparse_arrays_apply(&arrays, func);
	}
}

template <typename function>
void fck_ecs_apply_with_entity(fck_ecs *table, function func)
{
	using traits = fck_function_traits<function>;

	constexpr bool has_enough_args = traits::argument_count > 1;
	constexpr bool has_legal_return_type = fck_is_same<typename traits::return_type, void>::value;
	static_assert(has_enough_args, "Cannot invoke with zero function arguments");
	static_assert(has_legal_return_type, "Return type must be void!");

	constexpr bool is_first_arg_index = fck_is_same<typename traits::fck_function_traits::arguments::type, fck_ecs::entity_type>::value;
	static_assert(is_first_arg_index, "First argument must be ecs index type!");

	// Prevent cascading errors to keep the error log more concise
	if constexpr (has_enough_args && has_legal_return_type && is_first_arg_index)
	{
		auto arrays = fck_ecs_view_from_type_pack(table, (typename fck_pop_front<typename traits::arguments_no_pointer>::result){});

		fck_sparse_arrays_apply_with_entity(&arrays, func);
	}
}
#endif // FCK_ECS_INCLUDED