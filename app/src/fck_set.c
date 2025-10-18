#include "fck_set.h"

#include <kll.h>
#include <kll_malloc.h>

#include <SDL3/SDL_assert.h>

#define fck_set_pointer_offset(type, ptr, offset) ((type *)(((fckc_u8 *)(ptr)) + (offset)))


// typedef enum fck_set_key_state
//{
//	FCK_SET_KEY_EMPTY = 0LLU, // 00
//	FCK_SET_KEY_TAKEN = 1LLU, // 01
//	FCK_SET_KEY_STALE = 2LLU, // 10 (11 is unsed... - 1 wasted bit)
// } fck_set_key_state;

#define FCK_SET_KEY_COOKIE ((fckc_size_t)0xDEAD537)
#define FCK_SET_KEY_EMPTY 0LLU
#define FCK_SET_KEY_TAKEN 1LLU
#define FCK_SET_KEY_STALE 2LLU

typedef struct fck_set_state
{
	fckc_u64 mask; // Bits for 16 elements...
} fck_set_state;

typedef struct fck_set_key
{
	fckc_u64 hash; // Good enough - TODO: Maybe a string representation
} fck_set_key;

static fckc_size_t fck_set_align(fckc_size_t offset, fckc_size_t align)
{
	return (offset + align - 1) & ~(align - 1);
}

fckc_size_t fck_set_suggested_align(fckc_size_t x)
{
	// TODO: Fix up for 32-bit... We only do 64-bit for now anyway
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	x |= x >> 32;
	x = x - (x >> 1);
	return x;
}

fck_set_info *fck_opaque_set_inspect(void const *ptr)
{
	fck_set_info *info;
	info = fck_set_pointer_offset(fck_set_info, ptr, -sizeof(*info));
	SDL_assert(info->cookie == FCK_SET_KEY_COOKIE);
	return info;
}

void *fck_opaque_set_alloc(fck_set_info input)
{
	SDL_assert(input.el_size);
	// SDL_assert(info.el_align);
	SDL_assert(input.allocator);
	// SDL_assert(data);

	fck_set_info *info;
	const fckc_size_t info_size = sizeof(*info);
	const fckc_size_t data_size = input.el_size * input.capacity;
	const fckc_size_t key_size = sizeof(*info->keys) * input.capacity;
	// NOTE: Not so pretty implementation detail
	const fckc_size_t state_size = sizeof(*info->states) * ((input.capacity / 32) + 1);

	fckc_size_t info_offset = fck_set_align(0, sizeof(*info));
	fckc_size_t data_offset = info_offset + info_size; // We piggyback on the alignment of info... somehow., idk
	fckc_size_t keys_offset = fck_set_align(data_offset + data_size, sizeof(fck_set_key));
	fckc_size_t states_offset = fck_set_align(keys_offset + key_size, sizeof(fck_set_state));
	info = (fck_set_info *)kll_malloc(input.allocator, states_offset + state_size);

	SDL_memcpy(info, &input, sizeof(*info));
	// We just remember it for debugging
	info->keys = fck_set_pointer_offset(fck_set_key, info, keys_offset);
	info->states = fck_set_pointer_offset(fck_set_state, info, states_offset);
	info->stale = 0;
	SDL_memset(info->states, 0, state_size);
	info->cookie = FCK_SET_KEY_COOKIE;

	return fck_set_pointer_offset(void, info, data_offset);
}

void fck_opaque_set_free(void *ptr)
{
	SDL_assert(ptr);
	fck_set_info *info = fck_opaque_set_inspect(ptr);
	kll_free(info->allocator, info);
}

void fck_opaque_set_clear(void *ptr)
{
	SDL_assert(ptr);
	fck_set_info *info = fck_opaque_set_inspect(ptr);
	const fckc_size_t state_size = sizeof(*info->states) * ((info->capacity / 32) + 1);
	SDL_memset(info->states, 0, state_size);
	info->size = 0;
	info->stale = 0;
}

static void fck_opaque_set_rehash(fck_set_info **info_ref, void **ptr)
{
	fck_set_info *info = *info_ref;
	if (info->size + info->stale >= (info->capacity >> 1))
	{
		// Expand and rehash
		fck_set_info input_info = *info;
		input_info.keys = NULL;
		input_info.states = NULL;

		input_info.capacity = info->capacity + 1;
		input_info.capacity--;
		input_info.capacity = input_info.capacity | (input_info.capacity >> 1);
		input_info.capacity = input_info.capacity | (input_info.capacity >> 2);
		input_info.capacity = input_info.capacity | (input_info.capacity >> 4);
		input_info.capacity = input_info.capacity | (input_info.capacity >> 8);
		input_info.capacity = input_info.capacity | (input_info.capacity >> 16);
		input_info.capacity = input_info.capacity | (input_info.capacity >> 32);
		input_info.capacity++;

		void *old = *ptr;
		void *new = fck_opaque_set_alloc(input_info);

		fck_set_info *new_info = fck_opaque_set_inspect(new);
		fck_set_key *new_keys = new_info->keys;

		for (fckc_size_t index = 0; index < info->capacity; index++)
		{
			fck_set_state *old_state = info->states + (index / 32);
			fckc_u64 old_mask = FCK_SET_KEY_TAKEN << ((index % 32) * 2);
			int has_value = (old_state->mask & old_mask) == old_mask;
			if (!has_value)
			{
				continue;
			}

			fck_set_key *old_key = info->keys + index;
			fckc_size_t at = fck_opaque_set_strong_add(&new, old_key->hash);
			new_info = fck_opaque_set_inspect(new);
			fckc_u8 *dst = fck_set_pointer_offset(fckc_u8, new, info->el_size * at);
			fckc_u8 *src = fck_set_pointer_offset(fckc_u8, old, info->el_size * index);
			SDL_memcpy(dst, src, info->el_size);
		}

		new_info->size = info->size;
		fck_opaque_set_free(*ptr);

		// Refresh all pointers...
		*ptr = new;
		*info_ref = new_info;
	}
}

fckc_size_t fck_opaque_set_weak_add(void **ptr, fckc_u64 hash)
{
	SDL_assert(ptr);
	SDL_assert(*ptr);

	fck_set_info *info = fck_opaque_set_inspect(*ptr);
	fck_opaque_set_rehash(&info, ptr);

	fckc_size_t at = hash % info->capacity;
	for (;;)
	{
		fck_set_state *state = info->states + (at / 32);
		fckc_u64 taken_mask = FCK_SET_KEY_TAKEN << ((at % 32) * 2);
		fckc_u64 stale_mask = FCK_SET_KEY_STALE << ((at % 32) * 2);

		fck_set_key *key = info->keys + at;
		int has_value = (state->mask & taken_mask) == taken_mask;
		int was_stale = (state->mask & stale_mask) == stale_mask;

		if (!has_value)
		{
			if (was_stale)
			{
				info->stale = info->stale - 1;
			}
			info->size = info->size + 1;
			key->hash = hash;
			state->mask = state->mask & ~stale_mask;
			state->mask = state->mask | taken_mask;
			return at;
		}
		if (key->hash == hash)
		{
			return at;
		}
		at = (at + 1) % info->capacity;
	}
}

fckc_size_t fck_opaque_set_strong_add(void **ptr, fckc_u64 hash)
{
	SDL_assert(ptr);
	SDL_assert(*ptr);

	fckc_size_t has = fck_opaque_set_find(*ptr, hash);
	SDL_assert(!has);
	fckc_size_t at = fck_opaque_set_weak_add(ptr, hash);
	return at;
}

void fck_opaque_set_remove(void *ptr, fckc_u64 hash)
{
	SDL_assert(ptr);

	fck_set_info *info = fck_opaque_set_inspect(ptr);

	fckc_size_t has = fck_opaque_set_find(ptr, hash);
	if (has)
	{
		fckc_size_t at = has - 1;
		fck_set_state *state = info->states + (at / 32);
		fck_set_key *key = info->keys + at;

		fckc_u64 stale_mask = FCK_SET_KEY_STALE << ((at % 32) * 2);
		fckc_u64 taken_mask = FCK_SET_KEY_TAKEN << ((at % 32) * 2);
		state->mask = state->mask & ~taken_mask;
		state->mask = state->mask | stale_mask;
		info->size = info->size - 1;
		info->stale = info->stale + 1;
	}
}

fckc_size_t fck_opaque_set_find(void *ptr, fckc_u64 hash)
{
	SDL_assert(ptr);

	fck_set_info *info = fck_opaque_set_inspect(ptr);
	fckc_size_t at = hash % info->capacity;
	for (;;)
	{
		fck_set_state *state = info->states + (at / 32);
		fckc_u64 stale_mask = FCK_SET_KEY_STALE << ((at % 32) * 2);
		fckc_u64 taken_mask = FCK_SET_KEY_TAKEN << ((at % 32) * 2);
		fckc_u64 mask = stale_mask | taken_mask;
		fck_set_key *key = info->keys + at;

		fckc_u64 current = state->mask & mask;
		int is_done = (current) == 0;
		if (is_done)
		{
			return 0;
		}

		int has_value = (state->mask & taken_mask) == taken_mask;
		if (has_value)
		{
			if (key->hash == hash)
			{
				return at + 1;
			}
		}
		at = (at + 1) % info->capacity;
	}
}

fckc_u64 fck_set_key_resolve(fck_set_key *key)
{
	SDL_assert(key);
	return key->hash;
}

int fck_opaque_set_valid_at(void const *ptr, fckc_size_t at)
{
	SDL_assert(ptr);

	fck_set_info* info = fck_opaque_set_inspect(ptr);

	fck_set_state *state = info->states + (at / 32);
	fckc_u64 taken_mask = FCK_SET_KEY_TAKEN << ((at % 32) * 2);
	int has_value = (state->mask & taken_mask) == taken_mask;
	return has_value;
}

fck_set_key *fck_opaque_set_keys_at(void const *ptr, fckc_size_t at)
{
	SDL_assert(ptr);

	fck_set_info* info = fck_opaque_set_inspect(ptr);

	return info->keys + at;
}

fckc_size_t fck_opaque_set_begin(void const *ptr)
{
	SDL_assert(ptr);
	return (fckc_size_t)(-1LLU);
}

int fck_opaque_set_next(void const *ptr, fckc_size_t *index)
{
	SDL_assert(ptr);

	fck_set_info *info = fck_opaque_set_inspect(ptr);

	*index = *index + 1;
	for (;;)
	{
		if (*index >= info->capacity)
		{
			return 0;
		}

		fckc_u64 taken_mask = FCK_SET_KEY_TAKEN << ((*index % 32) * 2);
		fck_set_state *state = info->states + (*index / 32);
		int has_value = (state->mask & taken_mask) == taken_mask;
		if (has_value)
		{
			return 1;
		}
		*index = *index + 1;
	}
}