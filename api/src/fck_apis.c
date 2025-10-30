#include "fck_apis.h"

#include <fck_hash.h>
#include <fck_os.h>

#include <assert.h>

#include <kll.h>
#include <kll_heap.h>
#include <kll_malloc.h>

#include <SDL3/SDL_log.h>

typedef struct fck_apis_node
{
	fckc_u64 hash;
	const char *name;
	void *api;

	struct fck_apis_node *next;
} fck_apis_node;

#define fck_apis_hash_map_capacity 256

typedef struct fck_apis_hash_map
{
	fck_apis_node *tails[fck_apis_hash_map_capacity];
	fck_apis_node heads[fck_apis_hash_map_capacity];
} fck_apis_hash_map;

static fck_apis_hash_map fck_apis_storage;

static void fck_apis_add(const char *name, void *api)
{
	fck_hash_int hash = fck_hash(name, os->str->unsafe->len(name));
	fck_hash_int slot = hash % fck_apis_hash_map_capacity;

	fck_apis_node *current = &fck_apis_storage.heads[slot];
	fck_apis_node *tail = fck_apis_storage.tails[slot];
	if (tail != NULL)
	{
		tail->next = (fck_apis_node *)kll_malloc(kll_heap, sizeof(*current));
		current = tail->next;
	}

	current->hash = hash;
	current->next = NULL;
	current->name = name;
	current->api = api;
	fck_apis_storage.tails[slot] = current;
}

static int fck_apis_remove(const char *name)
{
	fck_hash_int hash = fck_hash(name, os->str->unsafe->len(name));
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
		os->mem->cpy(current, next, sizeof(*current));
		kll_free(kll_heap, next);
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
	fck_hash_int hash = fck_hash(name, os->str->unsafe->len(name));
	void *api = fck_apis_find_from_hash(hash);
	return api;
}

static void *fck_apis_next(void *prev)
{
	assert(0 && "NOT IMPLEMENTED");
	// TODO: We can do this one later...
	return NULL;
}

fck_api_registry fck_apis_runtime_state = {
	.add = fck_apis_add,
	.get = fck_apis_find_from_hash,
	.find = fck_apis_find_from_string,
	.remove = fck_apis_remove,
	.next = fck_apis_next,
};

FCK_EXPORT_API fck_api_registry *fck_main(fck_api_registry *api, fck_apis_init *init)
{
	api = &fck_apis_runtime_state;

	fck_apis_manifest *manifest = init->manifest;
	fckc_size_t count = init->count;
	for (fckc_size_t index = 0; index < count; index++)
	{
		fck_apis_manifest *current = &manifest[index];
		fck_shared_object api_so = os->so->load(current->name);
		fck_main_func *main_so = (fck_main_func *)os->so->symbol(api_so, FCK_ENTRY_POINT);
		*current->api = main_so(api, current->params);
		SDL_Log("Library Loaded: %s", current->name);
	}

	return api;
}
