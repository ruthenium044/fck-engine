
#include "fck_apis.h"

#include "fckc_apidef.h"
#include <fck_hash.h>

#include <string.h>

#define fck_apis_hash_map_capacity 256
#define fck_apis_hash_map_bucket_capacity 16
#define fck_apis_name_lookup_capacity fck_apis_hash_map_capacity *fck_apis_hash_map_bucket_capacity

typedef struct fck_apis_bucket
{
	fck_hash_int hash;
	char name[260];
	fckc_size_t count;

	void *implementations[fck_apis_hash_map_bucket_capacity];
} fck_apis_bucket;

typedef struct fck_apis_hash_map
{
	fck_apis_bucket buckets[fck_apis_hash_map_capacity];
} fck_apis_hash_map;

typedef struct fck_apis_name_lookup_entry
{
	void *implementation;
	const char *name;
} fck_apis_name_lookup_entry;

typedef struct fck_apis_name_lookup
{
	fck_apis_name_lookup_entry entries[fck_apis_name_lookup_capacity];
} fck_apis_name_lookup;

void fck_apis_name_lookup_add(fck_apis_name_lookup *lookup, void *implementation, const char *name)
{
	const fck_hash_int hash = fck_hash((const char*)implementation, sizeof(implementation));
	fck_hash_int slot = hash % fck_apis_hash_map_bucket_capacity;
	for (fckc_size_t index = 0; index < fck_apis_hash_map_bucket_capacity; index++)
	{
		fck_apis_name_lookup_entry *current = &lookup->entries[slot];
		if (current->implementation == NULL && current->name == NULL)
		{
			current->implementation = implementation;
			current->name = name;
			return;
		}
		if (current->implementation == implementation && strcmp(current->name, name) == 0)
		{
			return;
		}
		slot = (slot + 1) % fck_apis_hash_map_bucket_capacity;
	}
	return;
}

const char* fck_apis_name_lookup_find(fck_apis_name_lookup *lookup, void *implementation)
{
	const fck_hash_int hash = fck_hash((const char*)implementation, sizeof(implementation));

	fck_hash_int slot = hash % fck_apis_hash_map_bucket_capacity;
	for (fckc_size_t index = 0; index < fck_apis_hash_map_bucket_capacity; index++)
	{
		fck_apis_name_lookup_entry *current = &lookup->entries[slot];
		if (current->implementation == NULL)
		{
			return NULL;
		}
		if (current->implementation == implementation)
		{
			return current->name;
		}
		slot = (slot + 1) % fck_apis_hash_map_bucket_capacity;
	}
	return NULL;
}

static fck_apis_hash_map fck_apis_storage = {0};
static fck_apis_name_lookup fck_apis_names = {0};

static int fck_apis_add(const char *name, void *api)
{
	const fck_hash_int hash = fck_hash(name, strlen(name));
	fck_hash_int slot = hash % fck_apis_hash_map_capacity;

	for (fckc_size_t index = 0; index < fck_apis_hash_map_capacity; index++)
	{
		fck_apis_bucket *current = &fck_apis_storage.buckets[slot];
		if (current->hash == 0)
		{
			current->hash = hash;
			int len = strlen(name);
			memcpy(current->name, name, len);
			current->name[len] = '\0';
			current->count = 0;
		}

		if (current->hash == hash)
		{
			if (current->count == fck_arraysize(current->implementations))
			{
				return 0;
			}
			fck_apis_name_lookup_add(&fck_apis_names, api, current->name);

			current->implementations[current->count] = api;
			current->count = current->count + 1;
			return current->count;
		}
		slot = (slot + 1) % fck_apis_hash_map_capacity;
	}
	return 0;
}

static int fck_apis_remove(const char *name, void *api)
{
	const fck_hash_int hash = fck_hash(name, strlen(name));
	fck_hash_int slot = hash % fck_apis_hash_map_capacity;

	for (fckc_size_t index = 0; index < fck_apis_hash_map_capacity; index++)
	{
		fck_apis_bucket *current = &fck_apis_storage.buckets[slot];
		if (current->hash == 0)
		{
			return 0;
		}

		if (current->hash == hash)
		{
			if (current->count == 0)
			{
				return 0;
			}

			for (fckc_size_t implementation_index = 0; implementation_index < current->count; implementation_index++)
			{
				void **impl = current->implementations + implementation_index;
				if (*impl == api)
				{
					const fckc_size_t last = current->count - 1;
					*impl = current->implementations[last];
					current->implementations[last] = NULL;

					current->count = current->count - 1;
					return current->count;
				}
			}
			return 0;
		}
		slot = (slot + 1) % fck_apis_hash_map_capacity;
	}

	return 0;
}

static fckc_size_t fck_apis_implementations(const char *name, void ***apis)
{
	const fck_hash_int hash = fck_hash(name, strlen(name));
	fck_hash_int slot = hash % fck_apis_hash_map_capacity;
	for (fckc_size_t index = 0; index < fck_apis_hash_map_capacity; index++)
	{
		fck_apis_bucket *current = &fck_apis_storage.buckets[slot];
		if (current->hash == 0)
		{
			return 0;
		}

		if (current->hash == hash)
		{
			*apis = current->implementations;
			return current->count;
		}
		slot = (slot + 1) % fck_apis_hash_map_capacity;
	}
	return 0;
}

static void *fck_apis_find(const char *name)
{
	void **apis;
	const fckc_size_t count = fck_apis_implementations(name, &apis);
	if (count)
	{
		return apis[0];
	}
	return NULL;
}

static const char* fck_apis_nameof(void* api)
{
	return fck_apis_name_lookup_find(&fck_apis_names, api);
}

static fck_api_registry fck_apis_runtime_state = {
	.add = fck_apis_add,
	.implementations = fck_apis_implementations,
	.find = fck_apis_find,
	.remove = fck_apis_remove,
	.nameof = fck_apis_nameof,
};

FCK_EXPORT_API fck_api_registry *fck_api_load(fck_api_registry *registry, void *params)
{
	(void)registry;
	(void)params;
	return &fck_apis_runtime_state;
}
