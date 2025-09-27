
#include "fck_type_system.inl"

#include "SDL3/SDL_assert.h"
#include <SDL3/SDL_stdinc.h>

#include <fck_hash.h>

typedef struct fck_identifier_registry_entry
{
	fck_hash_int hash;
	char *str;
} fck_identifier_registry_entry;

typedef struct fck_identifier_registry
{
	struct fck_assembly *assembly;

	fckc_size_t count;
	fckc_size_t capacity;

	fck_identifier_registry_entry identifiers[1];
} fck_identifier_registry;

static fckc_u64 fck_identifier_registry_add_next_capacity(fckc_u64 n)
{
	if (n == 0)
		return 1;

	n--;
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	n |= n >> 32;
	n++;

	return n;
}

fck_identifier fck_identifier_null()
{
	return (fck_identifier){NULL, 0};
}

int fck_identifier_is_null(fck_identifier identifier)
{
	return identifier.identifiers == NULL;
}

int fck_identifier_is_same(fck_identifier a, fck_identifier b)
{
	return a.identifiers == b.identifiers && a.hash == b.hash;
}

const char *fck_identifier_resolve(fck_identifier identifier)
{
	fckc_size_t index = identifier.hash % identifier.identifiers->value->capacity;
	while (1)
	{
		fck_identifier_registry_entry *entry = &identifier.identifiers->value->identifiers[index];
		const char *str = entry->str;

		if (entry->hash == identifier.hash)
		{
			return str;
		}
		if (str == NULL)
		{
			return NULL;
		}
		index = (index + 1) % identifier.identifiers->value->capacity;
	}
}

fck_identifier_registry *fck_identifier_registry_alloc(struct fck_assembly *assembly, fckc_size_t capacity)
{
	// Do we need to guard for capacity == 0?
	// SDL_assert(capacity > 0);

	const fckc_size_t size = offsetof(fck_identifier_registry, identifiers[capacity]);
	fck_identifier_registry *registry = (fck_identifier_registry *)SDL_malloc(size);

	for (fckc_size_t index = 0; index < capacity; index++)
	{
		fck_identifier_registry_entry *entry = &registry->identifiers[index];
		entry->hash = 0;
		entry->str = NULL;
	}

	registry->assembly = assembly;
	registry->capacity = capacity;
	registry->count = 0;
	return registry;
}

void fck_identifier_registry_free(struct fck_identifier_registry *ptr)
{
	SDL_assert(ptr);

	SDL_free(ptr);
}

void fck_identifiers_free(struct fck_identifiers *ptr)
{
	SDL_assert(ptr);

	fck_identifier_registry_free(ptr->value);
	SDL_free(ptr);
}

fck_identifier fck_identifiers_add(struct fck_identifiers *identifiers, fck_identifier_desc desc)
{
	SDL_assert(identifiers != NULL);

	if (identifiers->value->count >= (identifiers->value->capacity >> 1))
	{
		// Realloc if required
		fckc_size_t next = (fckc_size_t)fck_identifier_registry_add_next_capacity(identifiers->value->capacity + 1); // + 1... I think
		fck_identifier_registry *result = fck_identifier_registry_alloc(identifiers->value->assembly, next);

		for (fckc_size_t index = 0; index < identifiers->value->capacity; index++)
		{
			fck_identifier_registry_entry *entry = &identifiers->value->identifiers[index];
			if (entry->str == NULL)
			{
				continue;
			}

			fckc_size_t new_index = entry->hash % result->capacity;
			while (1)
			{
				fck_identifier_registry_entry *new_entry = &result->identifiers[new_index];
				if (new_entry->str == NULL)
				{
					SDL_memcpy(new_entry, entry, sizeof(*entry));
					break;
				}
				new_index = (new_index + 1) % result->capacity;
			}
		}

		result->capacity = next;
		result->count = identifiers->value->count;
		fck_identifier_registry_free(identifiers->value);
		identifiers->value = result;
	}

	{
		// Add or get...
		fck_identifier_registry *registry = identifiers->value;

		fck_hash_int hash = fck_hash(desc.name, strlen(desc.name));
		fckc_size_t index = hash % registry->capacity;

		// That this while loop breaks is enforced through the realloc and size check
		while (1)
		{
			fck_identifier_registry_entry *entry = &registry->identifiers[index];
			const char *string = entry->str;
			if (entry->hash == hash)
			{
				SDL_assert(SDL_strcmp(desc.name, entry->str) == 0);
				return (fck_identifier){identifiers, hash};
			}
			if (string == NULL)
			{
				// Newly added
				entry->hash = hash;
				entry->str = SDL_strdup(desc.name);
				identifiers->value->count = identifiers->value->count + 1;
				return (fck_identifier){identifiers, hash};
			}
			index = index + 1;
		}
	}
}

fck_identifier fck_identifiers_find_from_hash(struct fck_identifiers *identifiers, fckc_u64 hash)
{
	SDL_assert(identifiers != NULL);

	fckc_size_t index = hash % identifiers->value->capacity;

	// That this while loop breaks is enforced through the realloc and size check
	while (1)
	{
		fck_identifier_registry_entry *entry = &identifiers->value->identifiers[index];
		const char *string = entry->str;
		if (entry->hash == hash)
		{
			return (fck_identifier){identifiers, hash};
		}
		if (string == NULL)
		{
			return (fck_identifier){NULL, 0};
		}
		index = index + 1;
	}
}

fck_identifier fck_identifiers_find_from_string(struct fck_identifiers *identifiers, const char *str)
{
	SDL_assert(identifiers != NULL);

	fck_hash_int hash = fck_hash(str, strlen(str));
	return fck_identifiers_find_from_hash(identifiers, hash);
}
