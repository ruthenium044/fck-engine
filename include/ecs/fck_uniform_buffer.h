#ifndef FCK_UNIFORM_BUFFER_INCLUDED
#define FCK_UNIFORM_BUFFER_INCLUDED

#include "fck_sparse_lookup.h"
#include <SDL3/SDL_assert.h>

struct fck_uniform_element_header
{
	size_t offset;
	size_t stride;
};

struct fck_uniform_element_debug_info
{
	uint16_t type_id;
};

struct fck_uniform_buffer
{
	fck_sparse_lookup<uint16_t, fck_uniform_element_header> headers;
	fck_sparse_lookup<uint16_t, fck_uniform_element_debug_info> debug_info;

	// Need to page data so we do not accidentally destabilise pointers
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

	fck_sparse_lookup_alloc(&buffer->headers, info->type_capacity);
	fck_sparse_lookup_alloc(&buffer->debug_info, info->type_capacity, {fck_indexer_info<uint16_t>::invalid});
}

inline void fck_uniform_buffer_free(fck_uniform_buffer *buffer)
{
	SDL_assert(buffer != nullptr);

	SDL_free(buffer->data);

	fck_sparse_lookup_free(&buffer->headers);
	fck_sparse_lookup_free(&buffer->debug_info);

	SDL_zerop(buffer);
}

template <typename type>
type *fck_uniform_buffer_ensure_buffer_and_get(fck_uniform_buffer *buffer)
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
	}

	return (type *)(buffer->data + header->offset);
}

template <typename type>
type *fck_uniform_buffer_set(fck_uniform_buffer *buffer, type const *value)
{
	type *value_in_buffer = fck_uniform_buffer_ensure_buffer_and_get<type>(buffer);
	*value_in_buffer = *value;
	return value_in_buffer;
}

template <typename type>
type *fck_uniform_buffer_set_empty(fck_uniform_buffer *buffer)
{
	type *value_in_buffer = fck_uniform_buffer_ensure_buffer_and_get<type>(buffer);
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