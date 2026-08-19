
#include "fck_db_id_set.h"

#include "fck_db.h"
#include "fck_db_core.inl"

#include <fck_hash.h>
#include <kll.h>
#include <kll_malloc.h>

#include <fckc_assert.h>
#include <fckc_inttypes.h>

#include <string.h>

#define fck_db_id_tombstone_type to_u16(65535)

static fck_db_id_set *fck_db_id_set_api_create(struct kll_allocator *allocator, fckc_size_t capacity)
{
	const fckc_size_t total = offsetof(fck_db_id_set, values[capacity]);
	fck_db_id_set *result = (fck_db_id_set *)kll_malloc(allocator, total);
	memset(result, 0, total);
	result->capacity = capacity;
	return result;
}

static void fck_db_id_set_api_destroy(struct kll_allocator *allocator, fck_db_id_set *set)
{
	kll_free(allocator, set);
}

static int fck_db_id_set_api_contains(fck_db_id_set *set, fck_db_id id)
{
	const fck_hash_int hash = fck_hash((const char *)&id, sizeof(id));
	for (fckc_size_t index = 0; index < set->capacity; index++)
	{
		const fckc_size_t slot = (index + hash) % set->capacity;
		const fck_db_id *item = set->values + slot;
		const fckc_u16 type = item->type;
		if (type == fck_db_type_none)
		{
			// We can stop
			return 0;
		}
		if (type != fck_db_id_tombstone_type)
		{
			if (item->value == id.value)
			{
				return 1;
			}
		}
	}
	// This should actually never really happen
	return 0;
}

static int fck_db_id_set_api_add_no_expand(fck_db_id_set *set, fck_db_id id)
{
	const fck_hash_int hash = fck_hash((const char *)&id, sizeof(id));
	for (fckc_size_t index = 0; index < set->capacity; index++)
	{
		const fckc_size_t slot = (index + hash) % set->capacity;
		fck_db_id *item = set->values + slot;
		const fckc_u16 type = item->type;
		if (type == fck_db_type_none || type == fck_db_id_tombstone_type)
		{
			// This one only ever works well if we ACTUALLY always guard calling it with a contains check
			set->count = set->count + 1;
			*item = id;
			return 1;
		}
	}
	return 0;
}

static int fck_db_id_set_api_add(struct kll_allocator *allocator, fck_db_id_set **pset, fck_db_id id)
{
	if (fck_db_id_set_api_contains(*pset, id))
	{
		return 0;
	}

	if ((*pset)->count >= (*pset)->capacity / 2)
	{
		// Rehash and realloc
		const fckc_size_t capacity = (*pset)->capacity * 2;
		fck_db_id_set *set = fck_db_id_set_api_create(allocator, capacity);

		for (fckc_size_t index = 0; index < (*pset)->capacity; index++)
		{
			const fck_db_id *id = (*pset)->values + index;
			if (id->type != fck_db_type_none && id->type != fck_db_id_tombstone_type)
			{
				const int added = fck_db_id_set_api_add_no_expand(set, *id);
				fck_assert(added);
			}
		}

		// Reallocated and rehashed!
		fck_db_id_set_api_destroy(allocator, *pset);
		*pset = set;
	}

	fck_db_id_set_api_add_no_expand(*pset, id);
	return 1;
}

static int fck_db_id_set_api_remove(fck_db_id_set *set, fck_db_id id)
{
	const fck_hash_int hash = fck_hash((const char *)&id, sizeof(id));
	for (fckc_size_t index = 0; index < set->capacity; index++)
	{
		const fckc_size_t slot = (index + hash) % set->capacity;
		fck_db_id *item = set->values + slot;
		const fckc_u16 type = item->type;
		if (type == fck_db_type_none)
		{
			return 0;
		}
		if (type != fck_db_id_tombstone_type)
		{
			if (item->value == id.value)
			{
				item->value = 0;
				item->type = fck_db_id_tombstone_type;
				return 1;
			}
		}
	}
	return 0;
}

static const fck_db_id *fck_db_id_set_api_iterate(fck_db_id_set *set, fck_db_id **it)
{
	if (!set || set->capacity == 0)
	{
		return NULL;
	}

	fck_db_id *current = (*it == NULL) ? set->values : *it + 1;

	const fck_db_id *end = set->values + set->capacity;
	for (; current != end; current++)
	{
		if (current->type != fck_db_type_none && current->type != fck_db_id_tombstone_type)
		{
			*it = current;
			return current;
		}
	}
	return NULL;
}

static fck_db_id_set_api db_id_set_api = {
	.create = fck_db_id_set_api_create,
	.destroy = fck_db_id_set_api_destroy,
	.add = fck_db_id_set_api_add,
	.contains = fck_db_id_set_api_contains,
	.remove = fck_db_id_set_api_remove,
	.iterate = fck_db_id_set_api_iterate,
};

fck_db_id_set_api *db_id_set = &db_id_set_api;