
#include "fck_apis.h"
#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_stdinc.h>

#include <fck_hash.h>

typedef struct fck_apis_node
{
	fckc_u64 hash;
	const char* name;
	void *api;

	struct fck_apis_node *next;
} fck_apis_node;

const fckc_size_t fck_apis_hash_map_capacity = 256;
typedef struct fck_apis_hash_map
{
	fck_apis_node *tails[fck_apis_hash_map_capacity];
	fck_apis_node heads[fck_apis_hash_map_capacity];
} fck_apis_hash_map;

static fck_apis_hash_map fck_apis_storage;

static void fck_apis_add(const char *name, void *api)
{
	fck_hash_int hash = fck_hash_unsafe(name);
	fck_hash_int slot = hash % fck_apis_hash_map_capacity;

	fck_apis_node *current = &fck_apis_storage.heads[slot];
	fck_apis_node *tail = fck_apis_storage.tails[slot];
	if (tail != NULL)
	{
		tail->next = (fck_apis_node *)SDL_malloc(sizeof(*current));
		current = tail->next;
	}

	current->hash = hash;
	current->next = NULL;
	current->name = name;
	current->api = api;
	fck_apis_storage.tails[slot] = current;
}

static fck_api_bool fck_apis_remove(const char *name)
{
	fck_hash_int hash = fck_hash_unsafe(name);
	fck_hash_int slot = hash % fck_apis_hash_map_capacity;

	fck_apis_node *current = &fck_apis_storage.heads[slot];
	fck_apis_node *tail = fck_apis_storage.tails[slot];
	if (tail == NULL)
	{
		return 0;
	}

	// This should work
	fck_apis_node *next = current->next;
	current->api = NULL;
	current->hash = 0;
	current->next = NULL;
	current->name = NULL;

	if (next != NULL)
	{
		SDL_memcpy(current, next, sizeof(*current));
		SDL_free(next);
	}

	if (tail == current)
	{
		tail = NULL;
	}
	return 1;
}

static void *fck_apis_find_from_hash(fckc_u64 hash)
{
	fck_hash_int slot = hash % fck_apis_hash_map_capacity;
	fck_apis_node *current = &fck_apis_storage.heads[slot];
	for (;;)
	{
		if (current->hash == hash)
		{
			return current->api;
		}
		current = current->next;
		if (current == NULL)
		{
			return NULL;
		}
	}
}

static void *fck_apis_find_from_string(const char *name)
{
	fck_hash_int hash = fck_hash_unsafe(name);
	void *api = fck_apis_find_from_hash(hash);
	return api;
}

static void *fck_apis_next(void *prev)
{
	SDL_assert(0 && "NOT IMPLEMENTED");
	// TODO: We can do this one later...
}

fck_apis fck_apis_runtime_state = {
	.add = fck_apis_add,
	.find = fck_apis_find_from_hash,
	.find_from_string = fck_apis_find_from_string,
	.remove = fck_apis_remove,
	.next = fck_apis_next,
};

// TODO: DLL
fck_apis *fck_apis_load(void)
{
	//// Setup linkedlist buckets...
	// for(fckc_size_t index = 0; index < fck_apis_hash_map_capacity; index++)
	//{
	//	fck_apis_storage.nodes[index].tail = &fck_apis_storage.nodes[index];
	// }

	return &fck_apis_runtime_state;
}

void fck_apis_unload(fck_apis *)
{
	// lol
}