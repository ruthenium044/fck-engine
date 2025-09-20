#include "fck_type_system.inl"

#include "SDL3/SDL_assert.h"
#include <SDL3/SDL_stdinc.h>

#include <fck_hash.h>

#include "fck_serialiser.h"

typedef struct fck_type_registry
{
	fckc_size_t count;
	fckc_size_t capacity;

	struct fck_identifiers *identifiers;

	// Open-addressed so we can iterate through the dense part!!
	fck_type_info info[1]; // info is plural...
} fck_type_registry;

typedef struct fck_types
{
	struct fck_type_registry *value;
} fck_types;

typedef struct fck_types_it
{
	struct fck_type_info *current;
} fck_types_it;

fck_types_it fck_types_it_begin(fck_types types)
{
	fck_types_it it = {.current = types.value->info};
	it.current--;
	return it;
}

int fck_types_it_next(fck_types *types, fck_types_it *it)
{
	// Always go next!
	it->current++;

	const fck_type_info *end = types->value->info + types->value->capacity;
	for (fck_type_info *entry = it->current; entry != end; entry++)
	{
		if (fck_identifier_is_null(entry->identifier))
		{
			continue;
		}
		it->current = entry;
		return 1;
	}
	return 0;
}

static fckc_u64 fck_type_registry_add_next_capacity(fckc_u64 n)
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

fck_type fck_type_null()
{
	return (fck_type){NULL, 0};
}

int fck_type_is_null(fck_type type)
{
	return type.types == NULL;
}

int fck_type_is(fck_type a, const char *str)
{
	return a.types != NULL && (a.hash == fck_hash(str, SDL_strlen(str)));
}

int fck_type_is_same(fck_type a, fck_type b)
{
	return a.types == b.types && a.hash == b.hash;
}

fck_type_info *fck_type_resolve(fck_type handle)
{
	fckc_size_t index = handle.hash % handle.types->value->capacity;
	while (1)
	{
		fck_type_info *entry = &handle.types->value->info[index];

		if (entry->hash == handle.hash)
		{
			return entry;
		}
		if (fck_identifier_is_null(entry->identifier))
		{
			return NULL;
		}
		index = (index + 1) % handle.types->value->capacity;
	}
}

fck_identifier fck_type_info_identify(struct fck_type_info *info)
{
	SDL_assert(info);
	return info->identifier;
}

fck_member fck_type_info_first_member(struct fck_type_info *info)
{
	SDL_assert(info);
	return info->first_member;
}

struct fck_type_registry *fck_type_registry_alloc(struct fck_identifiers *identifiers, fckc_size_t capacity)
{
	SDL_assert(identifiers);
	fckc_size_t size = offsetof(fck_type_registry, info[capacity]);
	fck_type_registry *registry = (fck_type_registry *)SDL_malloc(size);
	for (fckc_size_t index = 0; index < capacity; index++)
	{
		fck_type_info *entry = &registry->info[index];
		entry->hash = 0;
		entry->identifier = fck_identifier_null();
	}

	registry->count = 0;
	registry->capacity = capacity;
	registry->identifiers = identifiers;
	return registry;
}

struct fck_types *fck_types_alloc(struct fck_identifiers *identifiers, fckc_size_t capacity)
{
	fck_types *types = SDL_malloc(sizeof(*types));
	types->value = fck_type_registry_alloc(identifiers, capacity);
	return types;
}

static void fck_type_registry_free(struct fck_type_registry *ptr)
{
	SDL_assert(ptr);
	SDL_free(ptr);
}

void fck_types_free(struct fck_types *ptr)
{
	SDL_assert(ptr);
	fck_type_registry_free(ptr->value);
	SDL_free(ptr);
}

fck_type fck_types_add(struct fck_types *types, fck_type_desc desc)
{
	SDL_assert(types != NULL);

	if (types->value->count >= (types->value->capacity >> 1))
	{
		// Realloc if required
		fckc_size_t next_capacity = fck_type_registry_add_next_capacity(types->value->capacity + 1); // + 1... I think
		fck_type_registry *result = fck_type_registry_alloc(types->value->identifiers, next_capacity);

		for (fckc_size_t index = 0; index < types->value->capacity; index++)
		{
			fck_type_info *entry = &types->value->info[index];
			if (fck_identifier_is_null(entry->identifier))
			{
				continue;
			}

			fckc_size_t new_index = entry->hash % result->capacity;
			while (1)
			{
				fck_type_info *new_entry = &result->info[new_index];
				if (fck_identifier_is_null(new_entry->identifier))
				{
					SDL_memcpy(new_entry, entry, sizeof(*entry));
					break;
				}
				new_index = (new_index + 1) % result->capacity;
			}
		}

		result->capacity = next_capacity;
		result->count = types->value->count;
		fck_type_registry_free(types->value);
		types->value = result;
	}

	fck_type_registry *head = types->value;

	fck_identifier_desc identifier_desc;
	identifier_desc.name = desc.name;

	SDL_assert(head->identifiers != NULL);
	fck_identifier identifier = fck_identifiers_add(head->identifiers, identifier_desc);
	SDL_assert(head->identifiers != NULL);

	const char *str = fck_identifier_resolve(identifier);
	const fck_hash_int hash = fck_hash(str, SDL_strlen(str));
	fckc_size_t index = ((fckc_size_t)hash) % types->value->capacity;

	while (1)
	{
		// No need for safe iteration. ONE element IS empty for sure due to pre-condition
		const fck_type_info *current = &types->value->info[index];
		if (fck_identifier_is_null(current->identifier))
		{
			break;
		}
		if (fck_identifier_is_same(current->identifier, identifier))
		{
			// TODO: Make assert!
			// ALREADY ADDED MAYBE ERROR?! WHAT THE ACUTAL FUCK
			return (fck_type){types, hash};
		}
		index = (index + 1) % types->value->capacity;
	}

	// After the while loop above we know index "points" to a valid, empty slot
	fck_type_info *info = types->value->info + index;
	info->hash = hash;
	info->identifier = identifier;
	info->first_member = fck_member_null();
	info->last_member = fck_member_null();
	types->value->count = types->value->count + 1;
	return (fck_type){types, hash};
}

fck_type fck_types_find_from_hash(struct fck_types *types, fckc_u64 hash)
{
	fckc_size_t index = ((fckc_size_t)hash) % types->value->capacity;
	fck_type handle = (fck_type){types, hash};
	while (1)
	{
		// No need for safe iteration. ONE element IS empty for sure due to pre-condition
		const fck_type_info *current = &types->value->info[index];
		if (fck_identifier_is_null(current->identifier))
		{
			return fck_type_null();
		}
		if (current->hash == hash)
		{
			// SDL_assert(SDL_strcmp(name, fck_identifier_resolve(current->identifier)) == 0);
			return (fck_type){types, hash};
			// ALREADY ADDED MAYBE ERROR?! WHAT THE ACUTAL FUCK
		}
		index = (index + 1) % types->value->capacity;
	}

	return handle;
}

fck_type fck_types_find_from_string(struct fck_types *types, const char *name)
{
	const fck_hash_int hash = fck_hash(name, SDL_strlen(name));
	return fck_types_find_from_hash(types, hash);
}

int fck_types_iterate(struct fck_types *types, fck_type *type)
{
	fckc_size_t index = 0;
	if (fck_type_is_null(*type))
	{
		for (; index < types->value->capacity; index++)
		{
			const fck_type_info *current = &types->value->info[index];
			if (!fck_identifier_is_null(current->identifier))
			{
				type->types = types;
				type->hash = current->hash;
				return 1;
			}
		}
		return 0;
	}

	SDL_assert(type->types == types);

	index = ((fckc_size_t)type->hash) % types->value->capacity;
	for (; index < types->value->capacity; index++)
	{
		const fck_type_info *current = &types->value->info[index];
		if (current->hash == type->hash)
		{
			for (index = index + 1; index < types->value->capacity; index++)
			{
				current = &types->value->info[index];
				if (!fck_identifier_is_null(current->identifier))
				{
					type->types = types;
					type->hash = current->hash;
					return 1;
				}
			}
		}
	}

	return 0;
}