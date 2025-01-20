#ifndef FCK_ECS_INCLUDED
#define FCK_ECS_INCLUDED

#include <SDL3/SDL_assert.h>

#include "fck_ecs_component_traits.h"
#include "fck_serialiser.h"
#include "fck_sparse_array.h"
#include "fck_sparse_arrays.h"
#include "fck_sparse_list.h"
#include "fck_system.h"
#include "fck_tuple.h"
#include "fck_type_pack.h"
#include "fck_uniform_buffer.h"
#include "shared/fck_checks.h"

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

// TODO: Maybe separate the data from the functions B)
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

// Serialisation
void fck_ecs_snapshot_serialise(fck_serialiser *serialiser, fck_components_header *header, fck_ecs::sparse_array_void *sparse_array);
void fck_ecs_snapshot_deserialise(fck_serialiser *serialiser, fck_components_header *header, fck_ecs::sparse_array_void *sparse_array);

void fck_ecs_snapshot_store(fck_ecs *ecs, fck_serialiser *serialiser);
void fck_ecs_snapshot_store_partial(fck_ecs *ecs, fck_serialiser *serialiser, fck_ecs::dense_list<fck_ecs::entity_type> *entities);
void fck_ecs_snapshot_load(fck_ecs *ecs, fck_serialiser *serialiser);
void fck_ecs_snapshot_load_partial(fck_ecs *ecs, fck_serialiser *serialiser);

inline void fck_ecs_snapshot_free(fck_serialiser *serialiser)
{
	SDL_assert(serialiser != nullptr);

	fck_serialiser_free(serialiser);
}

// Systems
void fck_ecs_system_add(fck_ecs *ecs, fck_system_once on_once);
void fck_ecs_system_add(fck_ecs *ecs, fck_system_update on_update);
void fck_ecs_flush_system_once(fck_ecs *ecs);
void fck_ecs_tick(fck_ecs *ecs);
#endif // FCK_ECS_INCLUDED