
#include "fck_apis.h"
#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_stdinc.h>

#include <fck_hash.h>

typedef struct fck_apis_node
{
	fckc_u64 hash;

	void *api;

	struct fck_apis_node *next;
} fck_apis_node;

const fckc_size_t fck_apis_hash_map_capacity = 256;
typedef struct fck_apis_hash_map
{
	fck_apis_node *tails[fck_apis_hash_map_capacity];
	fck_apis_node nodes[fck_apis_hash_map_capacity];
} fck_apis_hash_map;

static fck_apis_hash_map fck_apis_storage;

static void fck_apis_add(const char *name, void *api)
{
	fck_hash_int hash = fck_hash_unsafe(name);
	fck_hash_int slot = hash % fck_apis_hash_map_capacity;

	fck_apis_node *current = &fck_apis_storage.nodes[slot];
	fck_apis_node *tail = fck_apis_storage.tails[slot];
	if (tail != NULL)
	{
		tail->next = (fck_apis_node *)SDL_malloc(sizeof(*current));
		current = tail->next;
	}

	current->next->hash = hash;
	current->next->next = NULL;
	current->next->api = api;
	fck_apis_storage.tails[slot] = current;
}

static void fck_apis_remove(const char *name)
{
	SDL_assert(0 && "NOT IMPLEMENTED");
	// TODO: We do not remove yet, so ehhhhh
}

static void *fck_apis_find_from_hash(fckc_u64 hash)
{
	fck_hash_int slot = hash % fck_apis_hash_map_capacity;
	fck_apis_node *current = &fck_apis_storage.nodes[slot];

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
	void* api = fck_apis_find_from_hash(hash);
	return api;
}

static void *fck_apis_next(void *prev)
{
	SDL_assert(0 && "NOT IMPLEMENTED");
	// TODO: We can do this one later...
}

fck_apis fck_apis_runtime_state = {
	.add = fck_apis_add,
	.remove = fck_apis_remove,
	.find_from_hash = fck_apis_find_from_hash,
	.find_from_string = fck_apis_find_from_string,
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