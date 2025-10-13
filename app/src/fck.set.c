#include "fck_set.h"

#include <kll.h>
#include <kll_malloc.h>

#include <SDL3/SDL_assert.h>

// Worse-case, align to sizeof - Good enough!
#define fck_set_alignof(x) sizeof(x)

typedef enum fck_set_key_state
{
	FCK_SET_KEY_EMPTY = 0, // 00
	FCK_SET_KEY_TAKEN = 1, // 01
	FCK_SET_KEY_STALE = 2, // 10 (11 is unsed... - 1 wasted bit)
} fck_set_key_state;

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

fck_set_info * fck_opaque_set_inspect(void *ptr, fckc_size_t align)
{
	fck_set_info *info;

	const fckc_size_t info_size = sizeof(*info);
	fckc_size_t info_offset = fck_set_align(0, fck_set_alignof(*info));
	fckc_size_t data_offset = fck_set_align(info_offset + info_size, align);
	return (fck_set_info *)((fckc_u8 *)(ptr)-data_offset);
}

void *fck_opaque_set_alloc(fck_set_info info)
{
	SDL_assert(info.el_size);
	SDL_assert(info.el_align);
	SDL_assert(info.allocator);
	// SDL_assert(data);

	fck_set_info *total;
	const fckc_size_t info_size = sizeof(*total);
	const fckc_size_t data_size = info.el_size * info.capacity;
	const fckc_size_t key_size = sizeof(*total->keys) * info.capacity;
	// NOTE: Not so pretty implementation detail
	const fckc_size_t state_size = sizeof(*total->states) * ((info.capacity / 32) + 1); // Maybe do a shift (>> 4)
	// kll_malloc(info.allocator, key_size + state_size);

	fckc_size_t info_offset = fck_set_align(0, fck_set_alignof(*total));
	fckc_size_t data_offset = fck_set_align(info_offset + info_size, info.el_align);
	fckc_size_t keys_offset = fck_set_align(data_offset + data_size, fck_set_alignof(*total->keys));
	fckc_size_t states_offset = fck_set_align(keys_offset + key_size, fck_set_alignof(*total->states));

	fckc_u8 *mem = (fckc_u8 *)kll_malloc(info.allocator, states_offset + state_size);
	total = (fck_set_info *)mem;

	SDL_memcpy(total, &info, sizeof(*total));
	// We just remember it for debugging
	total->keys = (fck_set_key *)(mem + keys_offset);
	total->states = (fck_set_state *)(mem + states_offset);

	SDL_memset(total->states, 0, state_size);

	return (void *)(mem + data_offset);
}

void fck_opaque_set_free(void *ptr, fckc_size_t align)
{
	SDL_assert(ptr);

	fck_set_info *info;
	fckc_u8 *mem = (fckc_u8 *)(ptr);

	const fckc_size_t info_size = sizeof(*info);
	fckc_size_t info_offset = fck_set_align(0, fck_set_alignof(*info));
	fckc_size_t data_offset = fck_set_align(info_offset + info_size, align);

	info = (fck_set_info *)(mem - data_offset);
	kll_free(info->allocator, info);
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

fckc_size_t fck_opaque_set_weak_add(void **ptr, fckc_u64 hash, fckc_size_t align)
{
	SDL_assert(ptr);
	SDL_assert(*ptr);

	fck_set_info *info;

	const fckc_size_t info_size = sizeof(*info);
	fckc_size_t info_offset = fck_set_align(0, fck_set_alignof(*info));
	fckc_size_t data_offset = fck_set_align(info_offset + info_size, align);
	info = (fck_set_info *)(((fckc_u8 *)(*ptr)) - data_offset);

	if (info->size >= info->capacity)
	{
		// Expand and rehash
		fck_set_info input_info = *info;
		input_info.keys = NULL;
		input_info.states = NULL;
		input_info.capacity = info->size + 1;
		if (info->size != 0)
		{
			input_info.capacity--;
			input_info.capacity |= input_info.capacity >> 1;
			input_info.capacity |= input_info.capacity >> 2;
			input_info.capacity |= input_info.capacity >> 4;
			input_info.capacity |= input_info.capacity >> 8;
			input_info.capacity |= input_info.capacity >> 16;
			input_info.capacity |= input_info.capacity >> 32;
			input_info.capacity++;
		}

		fckc_u8 *old_mem = (fckc_u8 *)(*ptr);
		fckc_u8 *new_mem = fck_opaque_set_alloc(input_info);

		fck_set_info *new_info = (fck_set_info *)(new_mem - data_offset);
		fck_set_key *new_keys = new_info->keys;
		fck_set_state *new_states = new_info->states;

		for (fckc_size_t index = 0; index < info->capacity; index++)
		{
			fck_set_state old_state = info->states[index >> 4];
			fckc_u64 old_mask = FCK_SET_KEY_TAKEN << ((index % 32) * 2);
			int has_value = (old_state.mask & old_mask) == old_mask;
			if (!has_value)
			{
				continue;
			}

			fck_set_key *old_key = info->keys + index;
			fckc_size_t at = old_key->hash % new_info->capacity;
			for (;;)
			{
				fck_set_state *state = new_info->states + (at >> 4);
				fckc_u64 mask = FCK_SET_KEY_TAKEN << ((at % 32) * 2);
				int is_taken = (state->mask & mask) == mask;
				if (!is_taken)
				{
					new_keys[at] = *old_key;
					state->mask = state->mask | mask;

					fckc_u8 *dst = new_mem + (info->el_size * at);
					fckc_u8 *src = old_mem + (info->el_size * index);
					SDL_memcpy(dst, src, info->el_size);
					break;
				}
				at = (at + 1) % info->capacity;
			}
		}

		new_info->size = info->size;
		fck_opaque_set_free(*ptr, info->el_align);

		// Refresh all pointers...
		*ptr = (void *)new_mem;
		info = (fck_set_info *)(((fckc_u8 *)(*ptr)) - data_offset);
	}

	fckc_size_t at = hash % info->capacity;
	for (;;)
	{
		fck_set_state *state = info->states + (at >> 4);
		fckc_u64 mask = FCK_SET_KEY_TAKEN << ((at % 32) * 2);
		fck_set_key *key = info->keys + at;
		int has_value = (state->mask & mask) == mask;

		if (!has_value)
		{
			info->size = info->size + 1;
			key->hash = hash;
			state->mask = state->mask | mask;
			fckc_u8 *mem = (fckc_u8 *)(*ptr);
			return (at);
		}
		if (key->hash == hash)
		{
			fckc_u8 *mem = (fckc_u8 *)(*ptr);
			return (at);
		}

		at = (at + 1) % info->capacity;
	}
}

void fck_opaque_set_remove(void **ptr, fckc_u64 hash, fckc_size_t align)
{
	SDL_assert(ptr);
	SDL_assert(*ptr);

	fck_set_info *info;

	fckc_u8 *mem = (fckc_u8 *)(*ptr);
	const fckc_size_t info_size = sizeof(*info);
	fckc_size_t info_offset = fck_set_align(0, fck_set_alignof(*info));
	fckc_size_t data_offset = fck_set_align(info_offset + info_size, align);
	info = (fck_set_info *)(mem - data_offset);

	fckc_size_t at = hash % info->capacity;
	for (;;)
	{
		fck_set_state *state = info->states + (at >> 4);
		fckc_u64 stale_mask = FCK_SET_KEY_STALE << ((at % 32) * 2);
		fckc_u64 taken_mask = FCK_SET_KEY_TAKEN << ((at % 32) * 2);
		fckc_u64 mask = stale_mask | taken_mask;
		fck_set_key *key = info->keys + at;

		int is_done = (state->mask & mask) == 0;
		if (is_done)
		{
			return;
		}

		int has_value = (state->mask & taken_mask) == taken_mask;
		if (key->hash == hash)
		{
			// Shift the mask from FCK_SET_KEY_TAKEN (0x01) to FCK_SET_KEY_STALE (0x02)
			state->mask = state->mask & ~taken_mask;
			state->mask = state->mask | stale_mask;
			info->size = info->size - 1;
			// Maybe debug clear memory?!
			// key->hash = (fckc_u64)0xDEADFACEDEADFACE;
			return;
		}

		at = (at + 1) % info->capacity;
	}
}