#ifndef FCK_ECS_INCLUDED
#define FCK_ECS_INCLUDED

#include <SDL3/SDL_assert.h>

#include "fck_sparse_array.h"
#include "fck_sparse_arrays.h"
#include "fck_sparse_list.h"
#include "fck_system.h"
#include "fck_tuple.h"
#include "fck_type_pack.h"
#include "fck_uniform_buffer.h"

template <typename index_type>
using fck_sparse_array_void_type = fck_sparse_array<index_type, void>;

struct fck_component_element_header
{
	size_t size;
};

struct fck_ecs
{
	using entity_type = uint32_t;
	using component_id = uint16_t;
	using system_id = uint16_t;
	using sparse_array_void_type = fck_sparse_array_void_type<entity_type>;
	using scheduler_type = fck_systems_scheduler<system_id>;
	fck_uniform_buffer uniform_buffer;

	// The value type should and could become somthing more useful!
	// Entities created through this functionality might receive some extras
	// I do not know - We now have two concepts:
	// - Internally created entities
	// - Externally created entities... oh boy
	// Oh what a fucking nightmare this is...
	// If user emplaces entities externally just for frun and then uses the entity create function
	// the user will just overwrite previously written data... Well, we can flag the interface at one point
	// that way the user must declare how the interactions with the ECS should look like
	fck_sparse_list<entity_type, entity_type> entities;
	entity_type entity_counter;

	template <typename value_type>
	using sparse_array = fck_sparse_array<entity_type, value_type>;

	template <typename value_type>
	using dense_list = fck_dense_list<entity_type, value_type>;

	using entity_list = dense_list<entity_type>;

	fck_sparse_array<component_id, fck_component_element_header> component_headers;
	fck_sparse_array<component_id, sparse_array_void_type> components;
	entity_type capacity;

	fck_sparse_lookup<system_id, fck_ecs_system_state> update_system_states;
	// probably no scheduler... more like buckets atm, hehe
	fck_systems_scheduler<system_id> system_scheduler;
};

struct fck_ecs_alloc_info
{
	fck_ecs::entity_type entity_capacity;
	fck_ecs::component_id component_capacity;
	fck_ecs::system_id system_capacity;
};

inline void fck_ecs_alloc(fck_ecs *ecs, fck_ecs_alloc_info const *info)
{
	SDL_assert(ecs != nullptr);
	SDL_zerop(ecs);

	ecs->capacity = info->entity_capacity;

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

	fck_uniform_buffer_free(&ecs->uniform_buffer);
	fck_sparse_array_free(&ecs->component_headers);
	fck_sparse_array_free(&ecs->components);
	fck_sparse_list_free(&ecs->entities);
	fck_sparse_lookup_free(&ecs->update_system_states);

	fck_systems_scheduler_free(&ecs->system_scheduler);

	SDL_zerop(ecs);
}

template <typename type>
type *fck_ecs_unique_set(fck_ecs *ecs, type const *value)
{
	SDL_assert(ecs != nullptr);
	return fck_uniform_buffer_set(&ecs->uniform_buffer, value);
}

template <typename type>
type *fck_ecs_unique_set_empty(fck_ecs *ecs)
{
	SDL_assert(ecs != nullptr);
	return fck_uniform_buffer_set_empty<type>(&ecs->uniform_buffer);
}

template <typename type>
type *fck_ecs_unique_view(fck_ecs *ecs)
{
	SDL_assert(ecs != nullptr);
	return fck_uniform_buffer_view<type>(&ecs->uniform_buffer);
}

inline fck_ecs::entity_type fck_ecs_entity_create(fck_ecs *ecs)
{
	SDL_assert(ecs != nullptr);

	ecs->entity_counter++;
	return fck_sparse_list_add(&ecs->entities, &ecs->entity_counter);
}

inline void fck_ecs_entity_emplace(fck_ecs *ecs, fck_ecs::entity_type entity)
{
	SDL_assert(ecs != nullptr);

	if (!fck_sparse_list_exists(&ecs->entities, entity))
	{
		ecs->entity_counter++;
		fck_sparse_list_emplace(&ecs->entities, entity, &ecs->entity_counter);
	}
}

inline void fck_ecs_entity_destroy(fck_ecs *ecs, fck_ecs::entity_type entity)
{
	SDL_assert(ecs != nullptr);

	for (fck_item<fck_ecs::component_id, fck_component_element_header> item : &ecs->component_headers)
	{
		fck_ecs::component_id *id = item.index;
		fck_component_element_header *header = item.value;
		fck_ecs::sparse_array_void_type *components = fck_sparse_array_view(&ecs->components, *id);
		if (fck_sparse_array_exists(components, entity))
		{
			fck_sparse_array_remove(components, entity, header->size);
		}
	}

	ecs->entity_counter--;
	return fck_sparse_list_remove(&ecs->entities, entity);
}

template <typename type>
uint16_t fck_unique_id_get()
{
	constexpr uint16_t unique_id = uint16_t(~0u);
	static uint16_t value = fck_count_up_and_get<unique_id>();
	return value;
}

template <typename type>
void fck_ecs_component_set(fck_ecs *table, fck_ecs::entity_type index, type const *value)
{
	SDL_assert(table != nullptr);

	uint16_t component_id = fck_unique_id_get<type>();
	fck_ecs::sparse_array_void_type *out_ptr;
	if (!fck_sparse_array_try_view(&table->components, component_id, &out_ptr))
	{
		fck_component_element_header header;
		header.size = sizeof(*value);
		fck_sparse_array_emplace(&table->component_headers, component_id, &header);

		fck_ecs::sparse_array<type> components;
		fck_sparse_array_alloc(&components, table->capacity);

		fck_ecs::sparse_array_void_type *as_void = (fck_ecs::sparse_array_void_type *)&components;
		fck_sparse_array_emplace(&components, index, value);
		fck_sparse_array_emplace(&table->components, component_id, as_void);
		// fck_sparse_array_emplace(&table->data, component_id, );
		return;
	}
	fck_ecs::sparse_array<type> *components = (fck_ecs::sparse_array<type> *)out_ptr;
	fck_sparse_array_emplace(components, index, value);
}

template <typename type>
type *fck_ecs_component_set_empty(fck_ecs *table, fck_ecs::entity_type index)
{
	SDL_assert(table != nullptr);

	uint16_t component_id = fck_unique_id_get<type>();
	fck_ecs::sparse_array_void_type *out_ptr;
	if (!fck_sparse_array_try_view(&table->components, component_id, &out_ptr))
	{
		fck_component_element_header header;
		header.size = sizeof(type);
		fck_sparse_array_emplace(&table->component_headers, component_id, &header);

		fck_ecs::sparse_array<type> components;
		fck_sparse_array_alloc(&components, table->capacity);

		fck_ecs::sparse_array_void_type *as_void = (fck_ecs::sparse_array_void_type *)&components;
		fck_sparse_array_emplace(&table->components, component_id, as_void);
		out_ptr = fck_sparse_array_view(&table->components, component_id);
	}

	fck_ecs::sparse_array<type> *components = (fck_ecs::sparse_array<type> *)out_ptr;
	fck_sparse_array_emplace_empty(components, index);
	return fck_sparse_array_view(components, index);
}

template <typename type>
void fck_ecs_fetch_single(fck_ecs *table, fck_ecs::sparse_array<type> **out_array_ptr)
{

	uint16_t component_id = fck_unique_id_get<type>();
	fck_ecs::sparse_array_void_type *out_ptr;

	*out_array_ptr = nullptr;
	if (fck_sparse_array_try_view(&table->components, component_id, &out_ptr))
	{
		*out_array_ptr = (fck_ecs::sparse_array<type> *)out_ptr;
	}
}

template <typename... types>
void fck_ecs_fetch_multi(fck_ecs *table, fck_tuple<fck_sparse_array<fck_ecs::entity_type, types> *...> *tuple)
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
	fck_ecs_fetch_multi<types...>(table, (fck_tuple<fck_sparse_array<fck_ecs::entity_type, types> *...> *)tuple);
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
fck_ecs::sparse_array<type> fck_ecs_view_single(fck_ecs *table)
{
	fck_ecs::sparse_array<type> result;
	SDL_zero(result);

	fck_ecs_fetch_single(table, &result.tuple);

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
		auto arrays = fck_ecs_view_from_type_pack(ecs, (typename traits::arguments_no_pointer){});

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