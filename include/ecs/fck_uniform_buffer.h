#ifndef FCK_UNIFORM_BUFFER_INCLUDED
#define FCK_UNIFORM_BUFFER_INCLUDED

#include "fck_sparse_lookup.h"
#include <SDL3/SDL_assert.h>

template <typename value_type>
using fck_uniform_buffer_cleanup = void (*)(value_type *);

using fck_uniform_buffer_cleanup_void = fck_uniform_buffer_cleanup<void>;

struct fck_uniform_element_header
{
	size_t offset;
	size_t stride;
	fck_uniform_buffer_cleanup_void destructor;
};

struct fck_uniform_element_debug_info
{
	uint16_t type_id;
};

struct fck_uniform_buffer
{
	fck_sparse_lookup<uint16_t, fck_uniform_element_header> headers;
	fck_sparse_lookup<uint16_t, fck_uniform_element_debug_info> debug_info;
	fck_dense_list<uint16_t, uint16_t> used_slots;
	// Need to page data so we do not accidentally destabilise pointers
	// Paging fucking bytes will be an absolute pain
	// What if a data type doesn't fit into one page? Variable sized pages? Fucking christ
	// What if we have 64 bytes left in page A and then we emplace 128 bytes?
	// We would need to go accross pages, what is impossible since we cannot provide views then
	// So we need to discard the 64 bytes, maybe put them in a free list to use later?
	// The simplest solution is to just provide large enough capacity. Assert, go next.
	uint8_t *data;

	size_t capacity;
	size_t count;
};

struct fck_uniform_buffer_alloc_info
{
	size_t capacity_in_bytes;
	uint16_t type_capacity;
};

template <typename type>
uint16_t fck_unique_slot_get()
{
	constexpr uint16_t unique_id = uint16_t(~0u) - 1;
	static uint16_t value = fck_count_up_and_get<1>();
	return value;
}

inline void fck_uniform_buffer_alloc(fck_uniform_buffer *buffer, fck_uniform_buffer_alloc_info const *info)
{
	SDL_assert(buffer != nullptr);
	SDL_zerop(buffer);

	buffer->data = (uint8_t *)SDL_malloc(sizeof(*buffer->data) * info->capacity_in_bytes);
	buffer->capacity = info->capacity_in_bytes;

	fck_dense_list_alloc(&buffer->used_slots, info->type_capacity);
	fck_sparse_lookup_alloc(&buffer->headers, info->type_capacity);
	fck_sparse_lookup_alloc(&buffer->debug_info, info->type_capacity, {fck_indexer_info<uint16_t>::invalid});
}

inline void fck_uniform_buffer_clear(fck_uniform_buffer *buffer)
{
	SDL_assert(buffer != nullptr);

	uint16_t last = buffer->used_slots.count - 1;
	for (uint16_t index = 0; index < buffer->used_slots.count; index++)
	{
		uint16_t reverse_index = last - index;
		uint16_t *slot = fck_dense_list_view(&buffer->used_slots, reverse_index);
		fck_uniform_element_header *header = fck_sparse_lookup_view(&buffer->headers, *slot);
		if (header->destructor != nullptr)
		{
			uint8_t *data_at = buffer->data + header->offset;
			header->destructor(data_at);
		}
		SDL_zerop(header);
	}
	fck_dense_list_clear(&buffer->used_slots);
}

inline void fck_uniform_buffer_free(fck_uniform_buffer *buffer)
{
	SDL_assert(buffer != nullptr);

	// let's clear before we free!
	fck_uniform_buffer_clear(buffer);

	SDL_free(buffer->data);

	fck_dense_list_free(&buffer->used_slots);
	fck_sparse_lookup_free(&buffer->headers);
	fck_sparse_lookup_free(&buffer->debug_info);

	SDL_zerop(buffer);
}

template <typename type>
type *fck_uniform_buffer_ensure_buffer_and_get(fck_uniform_buffer *buffer, fck_uniform_buffer_cleanup<type> destructor)
{
	SDL_assert(buffer != nullptr);

	uint16_t slot_index = fck_unique_slot_get<type>();

	// Unsexu debug section
	fck_uniform_element_debug_info *debug_info = fck_sparse_lookup_view(&buffer->debug_info, slot_index);
	constexpr uint16_t tombstone = fck_indexer_info<uint16_t>::invalid;
	SDL_assert(debug_info->type_id == tombstone || debug_info->type_id == slot_index);
	debug_info->type_id = slot_index;

	// It becomes sexy again
	fck_uniform_element_header *header = fck_sparse_lookup_view(&buffer->headers, slot_index);
	bool is_already_set = header->stride != 0;
	if (!is_already_set)
	{
		header->offset = buffer->count;
		header->stride = sizeof(type);
		buffer->count = buffer->count + header->stride;
		fck_dense_list_add(&buffer->used_slots, &slot_index);
	}

	SDL_assert(buffer->data + header->offset + header->stride < buffer->data + buffer->capacity &&
	           "Out of memory, trying to access uniform buffer memory that doesn't exist. Give buffer more capacity!"
	           "Look at the callstack to see what type is causing it");

	// In any case, let's overwrite the destructor. It might be a new one ;)
	header->destructor = (fck_uniform_buffer_cleanup_void)destructor;

	return (type *)(buffer->data + header->offset);
}

template <typename type>
type *fck_uniform_buffer_set(fck_uniform_buffer *buffer, type const *value, fck_uniform_buffer_cleanup<type> destructor = nullptr)
{
	type *value_in_buffer = fck_uniform_buffer_ensure_buffer_and_get<type>(buffer, destructor);
	*value_in_buffer = *value;
	return value_in_buffer;
}

template <typename type>
type *fck_uniform_buffer_set_empty(fck_uniform_buffer *buffer, fck_uniform_buffer_cleanup<type> destructor = nullptr)
{
	type *value_in_buffer = fck_uniform_buffer_ensure_buffer_and_get<type>(buffer, destructor);
	SDL_zerop(value_in_buffer);
	return value_in_buffer;
}

template <typename type>
type *fck_uniform_buffer_view(fck_uniform_buffer *buffer)
{
	SDL_assert(buffer != nullptr);

	uint16_t slot_index = fck_unique_slot_get<type>();

	// Unsexu debug section
	fck_uniform_element_debug_info *debug_info = fck_sparse_lookup_view(&buffer->debug_info, slot_index);
	constexpr uint16_t tombstone = fck_indexer_info<uint16_t>::invalid;
	SDL_assert(debug_info->type_id == tombstone || debug_info->type_id == slot_index);

	// It becomes sexy again
	fck_uniform_element_header *header = fck_sparse_lookup_view(&buffer->headers, slot_index);
	bool is_already_set = header->stride != 0;
	if (!is_already_set)
	{
		return nullptr;
	}

	return (type *)(buffer->data + header->offset);
}

#endif // FCK_UNIFORM_BUFFER_INCLUDED