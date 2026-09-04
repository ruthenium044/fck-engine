
#include "fck_db_object_page_table.h"

#include "fck_db_core.inl"

#include <kll.h>
#include <kll_malloc.h>

#include <fckc_assert.h>
#include <fckc_inttypes.h>

#include <string.h>

#define fck_db_object_child_capacity 255
#define fck_db_object_bitset_capacity 4
#define fck_db_object_bitset_chunk_capacity 64

union fck_db_object_page;
typedef union fck_db_object_page {
	struct
	{
		fckc_u64 ok[fck_db_object_bitset_capacity];
		union fck_db_object_page *children;
	} dir;

	fck_db_object object;
} fck_db_object_page;

typedef struct fck_db_object_page_table
{
	struct kll_allocator *allocator;
	fck_db_object_page root;

	// Later.
	// fck_db_guid_id_map guids;
} fck_db_object_page_table;

void fck_db_object_page_id_extract(fckc_u32 id, fckc_u8 *e0, fckc_u8 *e1, fckc_u8 *e2, fckc_u8 *e3)
{
	if (e0)
	{
		*e0 = (fckc_u8)((id >> 24) & 0xFF);
	}
	if (e1)
	{
		*e1 = (fckc_u8)((id >> 16) & 0xFF);
	}
	if (e2)
	{
		*e2 = (fckc_u8)((id >> 8) & 0xFF);
	}
	if (e3)
	{
		*e3 = (fckc_u8)(id & 0xFF);
	}
}

#define fck_db_object_page_id_is_ok(e0, e1, e2, e3) (((e0) != 0xFF) && ((e1) != 0xFF) && ((e2) != 0xFF) && ((e3) != 0xFF))
// static int fck_db_object_page_id_is_ok(fckc_u8 e0, fckc_u8 e1, fckc_u8 e2, fckc_u8 e3)
//{
//	if (e0 == 0xFF)
//	{
//		return 0;
//	}
//	if (e1 == 0xFF)
//	{
//		return 0;
//	}
//	if (e2 == 0xFF)
//	{
//		return 0;
//	}
//	if (e3 == 0xFF)
//	{
//		return 0;
//	}
//	return 1;
// }

static void fck_db_object_page_bit_set_ok(fck_db_object_page *page, const fckc_u8 subid)
{
	const fckc_u8 chunk = subid / fck_db_object_bitset_chunk_capacity;
	const fckc_u8 local = subid % fck_db_object_bitset_chunk_capacity;
	page->dir.ok[chunk] = page->dir.ok[chunk] | (1LLU << local);
}

static int fck_db_object_page_bit_is_ok(const fck_db_object_page *page, const fckc_u8 subid)
{
	const fckc_u8 chunk = subid / fck_db_object_bitset_chunk_capacity;
	const fckc_u8 local = subid % fck_db_object_bitset_chunk_capacity;
	return (page->dir.ok[chunk] & (1LLU << local)) == (1LLU << local);
}

static void fck_db_object_page_bit_clear_ok(fck_db_object_page *page, const fckc_u8 subid)
{
	const fckc_u8 chunk = subid / fck_db_object_bitset_chunk_capacity;
	const fckc_u8 local = subid % fck_db_object_bitset_chunk_capacity;
	page->dir.ok[chunk] = page->dir.ok[chunk] & ~(1LLU << local);
}

static int fck_db_object_page_empty(fck_db_object_page *dir)
{
	for (fckc_size_t chunk = 0; chunk < fck_arraysize(dir->dir.ok); chunk++)
	{
		if (dir->dir.ok[chunk])
		{
			return 0;
		}
	}
	return 1;
}

static int fck_db_object_page_ctz64(fckc_u64 v)
{
	static const int bit_positions[64] = {
		0,  1, 2,  7,  3,  13, 8,  19, 4,  25, 14, 28, 9,  34, 20, 40, 5,  17, 26, 38, 15, 46, 29, 48, 10, 31, 35, 54, 21, 50, 41, 57,
		63, 6, 12, 18, 24, 27, 33, 39, 16, 37, 45, 47, 30, 53, 49, 56, 62, 11, 23, 32, 36, 44, 52, 55, 61, 22, 43, 51, 60, 42, 59, 58,
	};
	return bit_positions[(to_u64((v & (0ULL - v)) * 0x0218A392CD3D5DBFULL)) >> 58];
}

static void fck_db_object_page_free(kll_allocator *allocator, fck_db_object_page *page, int level)
{
	if (level == 3)
	{
		// We are on a leaf
		return;
	}

	// Kill all children.
	for (fckc_size_t chunk_index = 0; chunk_index < fck_arraysize(page->dir.ok); chunk_index++)
	{
		const int chunk_offset = chunk_index * fck_db_object_bitset_chunk_capacity;
		fckc_u64 current = page->dir.ok[chunk_index];
		while (current)
		{
			const int index = fck_db_object_page_ctz64(current);
			const int child_index = index + chunk_offset;
			fck_db_object_page *child = page->dir.children + child_index;
			fck_db_object_page_free(allocator, child, level + 1);
			current = current & (current - 1ULL);
			kll_free(allocator, child);
		}
	}
}

fck_db_object_page_table *fck_db_object_page_table_alloc(struct kll_allocator *allocator)
{
	fck_db_object_page_table *table = (fck_db_object_page_table *)kll_malloc(allocator, sizeof(*table));
	memset(table, 0, sizeof(*table));
	table->allocator = allocator;

	return table;
}

void fck_db_object_page_table_free(fck_db_object_page_table *table)
{
	fck_db_object_page_free(table->allocator, &table->root, 0);
}

fck_db_object *fck_db_object_page_table_resolve(fck_db_object_page_table *table, fckc_u32 id)
{
	fckc_u8 e[4];
	fck_db_object_page_id_extract(id, &e[0], &e[1], &e[2], &e[3]);
	const int is_ok = fck_db_object_page_id_is_ok(e[0], e[1], e[2], e[3]);
	if (!is_ok)
	{
		return NULL;
	}

	fck_db_object_page *current = &table->root;
	for (fckc_size_t index = 0; index < fck_arraysize(e); index++)
	{
		if (current->dir.children == NULL)
		{
			return NULL;
		}
		const fckc_u8 subid = e[index];
		fck_assert(subid != 0xFF);

		fck_db_object_page *next = current->dir.children + subid;
		current = next;
	}

	return &current->object;
}

fck_db_object *fck_db_object_page_table_ensure(fck_db_object_page_table *table, fckc_u32 id)
{
	fckc_u8 e[4];
	const fckc_size_t indirections = fck_arraysize(e);
	fck_db_object_page_id_extract(id, &e[0], &e[1], &e[2], &e[3]);
	const int is_ok = fck_db_object_page_id_is_ok(e[0], e[1], e[2], e[3]);
	if (!is_ok)
	{
		return NULL;
	}

	kll_allocator *allocator = table->allocator;
	fck_db_object_page *parents[fck_arraysize(e)];

	fck_db_object_page *current = &table->root;
	for (fckc_size_t index = 0; index < indirections; index++)
	{
		const fckc_u8 subid = e[index];
		fck_assert(subid != 0xFF);

		if (current->dir.children == NULL)
		{
			const fckc_size_t total = fck_db_object_child_capacity * sizeof(*current->dir.children);
			current->dir.children = (fck_db_object_page *)kll_malloc(allocator, total);
			memset(current->dir.children, 0, total);
		}

		parents[index] = current;
		fck_db_object_page *next = current->dir.children + subid;
		current = next;
	}

	for (fckc_size_t index = 0; index < indirections; index++)
	{
		const fckc_u8 subid = e[index];
		fck_db_object_page *dir = parents[index];
		fck_db_object_page_bit_set_ok(dir, subid);
	}

	return &current->object;
}

fck_db_object *fck_db_object_page_table_add(fck_db_object_page_table *table, fckc_u32 id)
{
	fckc_u8 e[4];
	const fckc_size_t indirections = fck_arraysize(e);
	fck_db_object_page_id_extract(id, &e[0], &e[1], &e[2], &e[3]);
	const int is_ok = fck_db_object_page_id_is_ok(e[0], e[1], e[2], e[3]);
	if (!is_ok)
	{
		return NULL;
	}

	kll_allocator *allocator = table->allocator;
	fck_db_object_page *parents[fck_arraysize(e)];

	fck_db_object_page *current = &table->root;
	for (fckc_size_t index = 0; index < indirections; index++)
	{
		const fckc_u8 subid = e[index];
		fck_assert(subid != 0xFF);

		if (current->dir.children == NULL)
		{
			const fckc_size_t total = fck_db_object_child_capacity * sizeof(*current->dir.children);
			current->dir.children = (fck_db_object_page *)kll_malloc(allocator, total);
			memset(current->dir.children, 0, total);
		}

		parents[index] = current;
		fck_db_object_page *next = current->dir.children + subid;
		current = next;
	}

	{
		const fckc_u8 subid = e[indirections - 1];
		fck_db_object_page *dir = parents[indirections - 1];
		if (fck_db_object_page_bit_is_ok(dir, subid))
		{
			// Already added
			return NULL;
		}
	}

	for (fckc_size_t index = 0; index < indirections; index++)
	{
		const fckc_u8 subid = e[index];
		fck_db_object_page *dir = parents[index];
		fck_db_object_page_bit_set_ok(dir, subid);
	}

	return &current->object;
}

int fck_db_object_page_table_remove(fck_db_object_page_table *table, fckc_u32 id)
{
	fckc_u8 e[4];
	const fckc_size_t indirections = fck_arraysize(e);
	fck_db_object_page_id_extract(id, &e[0], &e[1], &e[2], &e[3]);
	const int is_ok = fck_db_object_page_id_is_ok(e[0], e[1], e[2], e[3]);
	if (!is_ok)
	{
		return 0;
	}

	kll_allocator *allocator = table->allocator;

	fck_db_object_page *parents[fck_arraysize(e)];
	fck_db_object_page *current = &table->root;
	for (fckc_size_t index = 0; index < indirections; index++)
	{
		const fckc_u8 subid = e[index];
		fck_assert(subid != 0xFF);

		if (!fck_db_object_page_bit_is_ok(current, subid))
		{
			return 0;
		}
		fck_assert(current->dir.children);

		parents[index] = current;
		fck_db_object_page *next = current->dir.children + subid;
		current = next;
	}

	for (fckc_size_t index = 0; index < indirections; index++)
	{
		const fckc_size_t inverse = indirections - index - 1;
		const fckc_u8 subid = e[inverse];
		fck_db_object_page *dir = parents[inverse];
		fck_db_object_page_bit_clear_ok(dir, subid);
		if (fck_db_object_page_empty(dir))
		{
			kll_free(allocator, dir->dir.children);
			dir->dir.children = NULL;
			continue;
		}
		break;
	}
	return 1;
}
