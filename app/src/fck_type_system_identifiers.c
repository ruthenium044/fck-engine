
#include "fck_type_system.inl"

#include "fck_serialiser_vt.h"

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_stdinc.h>

#include <kll.h>
#include <kll_heap.h>
#include <kll_malloc.h>

#include <fck_hash.h>

#include "fck_set.h"

typedef struct fck_identifier_info
{
	char *str;
} fck_identifier_info;

fck_identifier fck_identifier_null(void)
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
	fckc_size_t has = fck_set_find(identifier.identifiers->info, identifier.hash);
	if (has)
	{
		fck_identifier_info *entry = &identifier.identifiers->info[has - 1];
		return entry->str;
	}
	return NULL;
}

void fck_identifiers_alloc(struct fck_identifiers* identifiers, struct fck_assembly* assembly, fckc_size_t capacity)
{
	// Do we need to guard for capacity == 0?
	// SDL_assert(capacity > 0);
	SDL_assert(identifiers);
	SDL_assert(assembly);

	identifiers->info = fck_set_new(fck_identifier_info, kll_heap, capacity);
	identifiers->assembly = assembly;
}

void fck_identifiers_free(struct fck_identifiers *ptr)
{
	SDL_assert(ptr);

	fck_set_destroy(ptr->info);
}

fck_identifier fck_identifiers_add(struct fck_identifiers *identifiers, fck_identifier_desc desc)
{
	SDL_assert(identifiers != NULL);

	fck_hash_int hash = fck_hash(desc.name, strlen(desc.name));
	fckc_size_t has = fck_set_find(identifiers->info, hash);
	if (has)
	{
		fck_identifier_info *entry = &identifiers->info[has - 1];
		SDL_assert(SDL_strcmp(desc.name, entry->str) == 0);
		return (fck_identifier){identifiers, hash};
	}

	fck_identifier_info *entry = &fck_set_at(identifiers->info, hash);
	entry->str = SDL_strdup(desc.name);
	return (fck_identifier){identifiers, hash};
}

fck_identifier fck_identifiers_find_from_hash(struct fck_identifiers *identifiers, fckc_u64 hash)
{
	SDL_assert(identifiers != NULL);

	fckc_size_t has = fck_set_find(identifiers->info, hash);
	if (has)
	{
		return (fck_identifier){identifiers, hash};
	}
	return fck_identifier_null();
}

fck_identifier fck_identifiers_find_from_string(struct fck_identifiers *identifiers, const char *str)
{
	SDL_assert(identifiers != NULL);

	fck_hash_int hash = fck_hash(str, strlen(str));
	return fck_identifiers_find_from_hash(identifiers, hash);
}

void fck_serialise_identifiers(struct fck_serialiser *serialiser, struct fck_serialiser_params *params, fck_identifiers *v, fckc_size_t c)
{
	if (v == NULL)
	{
		return;
	}

	for (fckc_size_t i = 0; i < c; i++)
	{
		fck_identifiers *identifiers = v + i;

		fckc_size_t at = fck_set_begin(identifiers->info);
		while (fck_set_next(identifiers->info, at))
		{
			fck_identifier_info* entry = identifiers->info + at;
			struct fck_set_key* key = fck_set_keys_at(identifiers->info, at);
			fckc_u64 hash = fck_set_key_resolve(key);

			params->name = "hash";
			serialiser->vt->u64(serialiser, params, &hash, 1);

			// No clue if that will work lol
			fckc_size_t len = SDL_strlen(entry->str);
			fckc_u64 change = (fckc_u64)len;
			params->name = "strlen";
			serialiser->vt->u64(serialiser, params, &change, 1);

			if (change > len)
			{
				kll_free(kll_heap, entry->str);
				entry->str = kll_malloc(kll_heap, change);
			}
			params->name = "str";
			serialiser->vt->string(serialiser, params, &entry->str, 1);
		}
	}
}