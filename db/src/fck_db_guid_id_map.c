
#include "fck_db_guid_id_map.h"

#include "fck_db.h"

#include <fckc_assert.h>
#include <fckc_atomic.h>
#include <fckc_inttypes.h>

#include <fck_hash.h>

#include <kll.h>
#include <kll_malloc.h>

#include <fck_os.h>

#include <string.h>

typedef struct fck_db_guid_key
{
	fckc_u64 value : 62;
	fckc_u64 ok : 1;
	fckc_u64 tomb : 1;
} fck_db_guid_key;

typedef struct fck_db_guid_key_value
{
	fck_db_guid_key state;
	fck_db_guid guid;
	fck_db_id id;
} fck_db_guid_key_value;

static int fck_db_guid_store(fck_db_guid *guid, fckc_u64 time, fckc_u32 rand, fckc_u32 signal)
{
	if (signal == 0)
	{
		return 0;
	}
	guid->time = time;
	guid->rand = rand;
	const fckc_u32 old = fckc_u32_cas(&guid->signal, 0, signal);

	if (old == 0)
	{
		return 1;
	}
	return 0;
}

static int fck_db_guid_load(fck_db_guid *src, fckc_u64 *time, fckc_u32 *rand, fckc_u32 *signal)
{
	const fckc_u32 sig = fckc_u32_load(&src->signal);
	if (sig == 0)
	{
		return 0;
	}

	*time = src->time;
	*rand = src->rand;
	*signal = sig;
	return 1;
}

static int fck_db_guid_is_ok(fck_db_guid *guid)
{
	return fckc_u32_load(&guid->signal);
}

static fckc_u32 fck_xorshift32(fckc_u32 *state)
{
	fckc_u32 x = *state;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	*state = x;
	fck_assert(x);
	return x;
}

static void fck_db_guid_generate(fckc_u64 *time, fckc_u32 *rand, fckc_u32 *signal)
{
	fckc_u32 rng_state = (fckc_u32)os->chrono->now() ^ (fckc_u32)(fckc_uintptr)time;
	*time = (fckc_u64)os->chrono->now();
	*rand = ((fckc_u64)fck_xorshift32(&rng_state) << 32) | (fckc_u64)fck_xorshift32(&rng_state);
	*signal = fck_xorshift32(&rng_state);
	// fckc_spin(!fck_db_guid_store(slot, time, rand_val, signal));
}

static fckc_u64 fck_db_guid_raw_hash(fckc_u64 time, fckc_u32 rand, fckc_u32 signal)
{
	fckc_u64 hash = fck_hash_combine(time, (to_u64(rand) << 32) | to_u64(signal));
	// Extract two bits for the state...
	hash = hash & to_u64(~0LLU >> 2);
	return hash;
}

static int fck_db_guid_equals(fck_db_guid *guid, fckc_u64 time, fckc_u32 rand, fckc_u32 signal)
{
	fckc_u64 t;
	fckc_u32 r;
	fckc_u32 s;
	if (fck_db_guid_load(guid, &t, &r, &s))
	{
		return time == t && rand == r && signal == s;
	}
	return 0;
}

static fckc_u64 fck_db_guid_id_map_probe(fck_db_guid_id_map *map, fckc_u64 hash)
{
	for (fckc_size_t index = 0; index < map->capacity; index++)
	{
		const fckc_size_t slot = (hash + index) % map->capacity;
		fck_db_guid_key_value *item = map->values + slot;
		if (!item->state.ok || item->state.tomb)
		{
			return slot + 1;
		}
	}
	fck_assert(0 && "out of capacity - this should NOT happen");
	return 0;
}

static fckc_u64 fck_db_guid_id_map_find(fck_db_guid_id_map *map, fckc_u64 time, fckc_u32 rand, fckc_u32 signal)
{
	const fckc_u64 hash = fck_db_guid_raw_hash(time, rand, signal);

	for (fckc_size_t index = 0; index < map->capacity; index++)
	{
		const fckc_size_t slot = (hash + index) % map->capacity;
		fck_db_guid_key_value *item = map->values + slot;
		if (!item->state.ok)
		{
			break;
		}
		if (!item->state.tomb && item->state.value == hash)
		{
			if (fck_db_guid_equals(&item->guid, time, rand, signal))
			{
				return slot + 1;
			}
		}
	}
	return 0;
}

static fckc_u64 fck_db_guid_id_map_next(fck_db_guid_id_map *map, fckc_u64 *value)
{
	for (fckc_size_t index = *value; index < map->capacity; index++)
	{
		const fck_db_guid_key_value *item = map->values + index;
		if (item->state.ok)
		{
			// Asset that item->state.tomb is 0?
			*value = index + 1;
			return *value;
		}
	}
	return 0;
}

static fckc_u64 fck_db_guid_id_map_add(fck_db_guid_id_map *map, fckc_u64 hash, fckc_u64 time, fckc_u32 rand, fckc_u32 signal, fck_db_id id)
{
	const fckc_u64 result = fck_db_guid_id_map_probe(map, hash);
	if (result)
	{
		fck_db_guid_key_value *item = map->values + result - 1;
		item->state.ok = 1;
		item->state.tomb = 0;
		item->state.value = hash;
		item->id = id;
		// When we lock the map later, we should maintain exclusive access here!
		fck_db_guid_store(&item->guid, time, rand, signal);
		map->count = map->count + 1;
	}
	return result;
}

static fckc_u64 fck_db_guid_id_map_grow_add(fck_db_guid_id_map *map, fckc_u64 time, fckc_u32 rand, fckc_u32 signal, fck_db_id id)
{
	{
		const fckc_u64 result = fck_db_guid_id_map_find(map, time, rand, signal);
		if (result)
		{
			return result;
		}
	}

	// TODO: Locking! :-D
	if (map->count > map->capacity / 2)
	{
		const fckc_size_t capacity = map->capacity ? 32 : map->capacity * 4;
		const fckc_size_t total = sizeof(*map->values) * capacity;
		fck_db_guid_key_value *values = (fck_db_guid_key_value *)kll_malloc(kll->system, total);
		memset(values, 0, total);

		fck_db_guid_id_map next = {.values = values, .capacity = capacity};
		fckc_u64 it = 0;
		while (fck_db_guid_id_map_next(map, &it))
		{
			fck_db_guid_key_value *item = map->values + it - 1;
			if (item->state.ok)
			{
				fckc_u64 t;
				fckc_u32 r;
				fckc_u32 s;
				if (fck_db_guid_load(&item->guid, &t, &r, &s))
				{
					fck_db_guid_id_map_add(&next, item->state.value, t, r, s, item->id);
				}
			}
		}

		kll_free(kll->system, map->values);
		*map = next;
	}

	{
		const fckc_u64 hash = fck_db_guid_raw_hash(time, rand, signal);
		const fckc_u64 result = fck_db_guid_id_map_add(map, hash, time, rand, signal, id);
		return result;
	}
}
