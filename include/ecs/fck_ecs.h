#ifndef FCK_ECS_INCLUDED
#define FCK_ECS_INCLUDED

#include <SDL3/SDL_assert.h>

#include "fck_checks.h"
#include "fck_sparse_array.h"
#include "fck_sparse_arrays.h"
#include "fck_sparse_list.h"
#include "fck_system.h"
#include "fck_tuple.h"
#include "fck_type_pack.h"
#include "fck_uniform_buffer.h"

// Primary template: Assumes fck_serialise does not exist for T
template <typename index_type>
using fck_sparse_array_void_type = fck_sparse_array<index_type, void>;

template <typename value_type>
using fck_ecs_component_serialise = void (*)(value_type *, size_t);

using fck_ecs_component_serialise_void = fck_ecs_component_serialise<void>;

template <typename value_type>
using fck_ecs_component_cleanup = void (*)(value_type *);

using fck_ecs_component_cleanup_void = fck_ecs_component_cleanup<void>;

template <typename value_type>
struct fck_components_interface
{
	fck_ecs_component_cleanup<value_type> destructor;
	fck_ecs_component_serialise<value_type> serialise;
};

using fck_components_interface_void = fck_components_interface<void>;

template <typename value_type, typename = void>
struct fck_ecs_free_trait : fck_false_type
{
};

template <typename value_type>
struct fck_ecs_free_trait<value_type, fck_void<decltype(fck_free((value_type *)nullptr))>> : fck_true_type
{
};

struct fck_components_header
{
	size_t size;
	fck_components_interface_void interface;
};

struct fck_ecs
{
	using entity_type = uint32_t;
	using component_id = uint16_t;
	using system_id = uint16_t;
	using sparse_array_void = fck_sparse_array_void_type<entity_type>;
	using scheduler_type = fck_systems_scheduler<system_id>;

	template <typename value_type>
	using sparse_array = fck_sparse_array<entity_type, value_type>;

	template <typename value_type>
	using dense_list = fck_dense_list<entity_type, value_type>;
	using entity_list = dense_list<entity_type>;

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

	// Duplicate count and capacity (overhead)
	fck_sparse_array<component_id, fck_components_header> component_headers;
	fck_sparse_array<component_id, sparse_array_void> components;
	entity_type capacity;

	fck_sparse_lookup<system_id, fck_ecs_system_state> update_system_states;
	// probably no scheduler... more like buckets atm, hehe
	fck_systems_scheduler<system_id> system_scheduler;
};

struct fck_ecs_snapshot
{
	uint8_t *data;
	size_t count;
	size_t capacity;
	size_t index;
};

inline void fck_ecs_snapshot_alloc(fck_ecs_snapshot *blob)
{
	SDL_assert(blob != nullptr);
	SDL_zerop(blob);

	constexpr size_t dafault_capacity = 1024;
	blob->data = (uint8_t *)SDL_malloc(dafault_capacity);
	blob->capacity = dafault_capacity;
	blob->count = 0;
	blob->index = 0;
}

inline void fck_ecs_snapshot_free(fck_ecs_snapshot *blob)
{
	SDL_assert(blob != nullptr);
	SDL_free(blob->data);
	SDL_zerop(blob);
}

inline void fck_ecs_snapshot_maybe_realloc(fck_ecs_snapshot *blob, size_t slack_count)
{
	SDL_assert(blob != nullptr);

	// Fix it with a smart calculation :D
	// Just lazy at the moment
	while (blob->capacity < blob->count + slack_count)
	{
		const size_t new_capacity = blob->capacity * 2;
		uint8_t *old_mem = blob->data;
		uint8_t *new_mem = (uint8_t *)SDL_realloc(blob->data, new_capacity);

		if (new_mem != nullptr)
		{
			SDL_LogCritical(0, "Failed to reallocate memory!");
			blob->data = new_mem;
		}

		blob->capacity = new_capacity;
	}
}

inline void fck_ecs_snapshot_serialise(fck_ecs_snapshot *snapshot, fck_components_header *header, fck_ecs::sparse_array_void *opaque_array)
{
	SDL_assert(snapshot != nullptr);

	const size_t count_size = sizeof(opaque_array->owner.count);
	const size_t full_dense_data_size = header->size * opaque_array->dense.count;
	const size_t full_dense_owner_size = sizeof(*opaque_array->owner.data) * opaque_array->dense.count;

	fck_ecs_snapshot_maybe_realloc(snapshot, full_dense_data_size + full_dense_owner_size);

	uint8_t *at = snapshot->data + snapshot->count;
	SDL_memcpy(at, &opaque_array->owner.count, count_size);

	at = at + count_size;
	SDL_memcpy(at, opaque_array->owner.data, full_dense_owner_size);
	at = at + full_dense_owner_size;

	SDL_memcpy(at, opaque_array->dense.data, full_dense_data_size);
	snapshot->count = snapshot->count + count_size + full_dense_owner_size + full_dense_data_size;
}

inline void fck_ecs_snapshot_deserialise(fck_ecs_snapshot *snapshot, fck_components_header *header,
                                         fck_ecs::sparse_array_void *opaque_array)
{
	SDL_assert(snapshot != nullptr);

	const size_t count_size = sizeof(opaque_array->owner.count);

	fck_sparse_array_clear(opaque_array);

	uint8_t *at = snapshot->data + snapshot->index;
	fck_ecs::entity_type count = *(fck_ecs::entity_type *)at;
	at = at + count_size;

	const size_t full_dense_data_size = header->size * count;
	const size_t full_dense_owner_size = sizeof(*opaque_array->owner.data) * count;

	fck_ecs::entity_type *owners = (fck_ecs::entity_type *)at;
	at = at + full_dense_owner_size;

	uint8_t *data = at;

	SDL_memcpy(opaque_array->owner.data, owners, full_dense_owner_size);
	SDL_memcpy(opaque_array->dense.data, data, full_dense_data_size);

	opaque_array->owner.count = count;
	opaque_array->dense.count = count;

	for (fck_ecs::entity_type index = 0; index < count; index++)
	{
		fck_ecs::entity_type *data_owner = owners + index;
		fck_sparse_lookup_set(&opaque_array->sparse, *data_owner, &index);
	}
	snapshot->index = snapshot->index + count_size + full_dense_data_size + full_dense_owner_size;
}

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

	for (fck_item<fck_ecs::component_id, fck_components_header> item : &ecs->component_headers)
	{
		fck_ecs::component_id *id = item.index;
		fck_components_header *header = item.value;
		fck_ecs::sparse_array_void *components = fck_sparse_array_view(&ecs->components, *id);
		fck_components_interface_void *interface = &header->interface;
		if (interface->destructor != nullptr)
		{
			for (fck_ecs::entity_type entity = 0; entity < components->dense.count; entity++)
			{
				void *data = fck_dense_list_view_raw(&components->dense, entity, header->size);
				interface->destructor(data);
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
type *fck_ecs_unique_set_empty(fck_ecs *ecs, fck_uniform_buffer_cleanup<type> destructor = nullptr)
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

	ecs->entity_counter++;
	return fck_sparse_list_add(&ecs->entities, &ecs->entity_counter);
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
		ecs->entity_counter++;
		fck_sparse_list_emplace(&ecs->entities, entity, &ecs->entity_counter);
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
			if (interface->destructor != nullptr)
			{
				void *data = fck_sparse_array_view_raw(components, entity, header->size);
				interface->destructor(data);
			}
			fck_sparse_array_remove_raw(components, entity, header->size);
		}
	}

	ecs->entity_counter--;
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
		if (interface->destructor != nullptr)
		{
			for (fck_ecs::entity_type entity = 0; entity < components->dense.count; entity++)
			{
				void *data = fck_dense_list_view_raw(&components->dense, entity, header->size);
				interface->destructor(data);
			}
		}

		fck_sparse_array_clear(components);
	}

	ecs->entity_counter = 0;
	fck_sparse_list_clear(&ecs->entities);
}

inline void fck_ecs_snapshot_store(fck_ecs *ecs, fck_ecs_snapshot *snapshot)
{
	SDL_assert(ecs != nullptr);

	fck_ecs_snapshot_alloc(snapshot);

	for (fck_item<fck_ecs::component_id, fck_components_header> item : &ecs->component_headers)
	{
		fck_ecs::component_id *id = item.index;
		fck_components_header *header = item.value;
		fck_ecs::sparse_array_void *components = fck_sparse_array_view(&ecs->components, *id);
		fck_components_interface_void *interface = &header->interface;
		if (interface->serialise != nullptr)
		{
			interface->serialise(components->dense.data, components->dense.count);
		}
		fck_ecs_snapshot_serialise(snapshot, header, components);
	}
}

inline void fck_ecs_snapshot_load(fck_ecs *ecs, fck_ecs_snapshot *snapshot)
{
	SDL_assert(ecs != nullptr);

	// Not optimal. We might destroy something we want to keep! :((
	for (fck_item<fck_ecs::component_id, fck_components_header> item : &ecs->component_headers)
	{
		fck_ecs::component_id *id = item.index;
		fck_components_header *header = item.value;
		fck_ecs::sparse_array_void *components = fck_sparse_array_view(&ecs->components, *id);

		fck_ecs_snapshot_deserialise(snapshot, header, components);
	}
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
		if constexpr (fck_ecs_free_trait<type>::value)
		{
			void fck_free(type *);
			interface.destructor = fck_free;
		}
		// TODO: More traits!!
		fck_ecs_component_interface_override(ecs, &interface);

		fck_ecs::sparse_array<type> components;
		fck_sparse_array_alloc(&components, ecs->capacity);
		fck_ecs::sparse_array_void *as_void = (fck_ecs::sparse_array_void *)&components;
		out_ptr = fck_sparse_array_emplace(&ecs->components, component_id, as_void);
	}
	return (fck_ecs::sparse_array<type> *)out_ptr;
}

template <typename type>
type *fck_ecs_component_set_empty(fck_ecs *ecs, fck_ecs::entity_type index)
{
	// Make this function the center point so we do not have the same code in different bodies lying around!!

	// Very similar to set, might be useful to refactor this into one internal set... but also, not having so much noise is nice
	SDL_assert(ecs != nullptr);
	fck_ecs::sparse_array<type> *components = fck_ecs_component_register<type>(ecs);

	// emplacing and checking is so cheap, we can just do it again
	// The laternative is checking whether it exists and throw or return if not...
	// But in this case we still access the same data and have the same overhead
	fck_ecs_entity_emplace(ecs, index);

	fck_sparse_array_emplace_empty(components, index);
	return fck_sparse_array_view(components, index);
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