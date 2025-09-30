#include "fck_type_system.inl"

#include "SDL3/SDL_assert.h"
#include <SDL3/SDL_stdinc.h>

#include <fck_hash.h>

struct fck_assembly;

typedef struct fck_member_registry
{
	struct fck_assembly *assembly;

	fckc_size_t count;
	fckc_size_t capacity;

	struct fck_identifiers *identifiers;

	fck_member_info info[1];
} fck_member_registry;

static fckc_u64 fck_member_registry_add_next_capacity(fckc_u64 n)
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

fck_member fck_member_null()
{
	return (fck_member){NULL, 0};
}

int fck_member_is_null(fck_member member)
{
	return member.members == NULL;
}

int fck_member_is_same(fck_member a, fck_member b)
{
	return a.members == b.members && a.hash == b.hash;
}

fck_member_info *fck_member_resolve(fck_member member)
{
	if (fck_member_is_null(member))
	{
		return NULL;
	}
	fckc_size_t index = member.hash % member.members->value->capacity;
	for (;;)
	{
		fck_member_info *entry = &member.members->value->info[index];

		if (entry->hash == member.hash)
		{
			return entry;
		}
		if (fck_identifier_is_null(entry->identifier))
		{
			return NULL;
		}
		index = (index + 1) % member.members->value->capacity;
	}
}

fck_identifier fck_member_info_identify(struct fck_member_info *info)
{
	return info->identifier;
}

fck_type fck_member_info_owner(struct fck_member_info *info)
{
	return info->owner;
}

fck_type fck_member_info_type(struct fck_member_info *info)
{
	return info->type;
}

fck_member fck_member_info_next(struct fck_member_info *info)
{
	return info->next;
}

fckc_size_t fck_member_info_stride(struct fck_member_info *info)
{
	return info->stride;
}

fckc_size_t fck_member_info_count(struct fck_member_info *info)
{
	return info->extra_count + 1;
}

fck_member_registry *fck_member_registry_alloc(struct fck_assembly *assembly, struct fck_identifiers *identifiers, fckc_size_t capacity)
{
	SDL_assert(identifiers);

	fckc_size_t size = offsetof(fck_member_registry, info[capacity]);
	fck_member_registry *registry = (fck_member_registry *)SDL_malloc(size);
	for (fckc_size_t index = 0; index < capacity; index++)
	{
		fck_member_info *entry = &registry->info[index];
		entry->hash = 0;
		entry->identifier = fck_identifier_null();
	}

	registry->assembly = assembly;
	registry->count = 0;
	registry->capacity = capacity;
	registry->identifiers = identifiers;
	return registry;
}

void fck_member_registry_free(fck_member_registry *ptr)
{
	SDL_assert(ptr);
	SDL_free(ptr);
}

void fck_members_free(struct fck_members *ptr)
{
	SDL_assert(ptr);
	fck_member_registry_free(ptr->value);
	SDL_free(ptr);
}

static void fck_type_info_add_member(fck_type type_handle, fck_member member_handle)
{
	struct fck_type_info *info = fck_type_resolve(type_handle);
	fck_member_info *member = fck_member_resolve(member_handle);

	member->owner = type_handle;
	member->next = fck_member_null();
	if (fck_member_is_null(info->first_member))
	{
		info->first_member = member_handle;
		info->last_member = member_handle;
		return;
	}
	// else
	fck_member_info *last_member = fck_member_resolve(info->last_member);
	last_member->next = member_handle;
	info->last_member = member_handle;
};

fck_member fck_members_add(fck_members *members, fck_type owner, fck_member_desc desc)
{
	// Maybe resize
	if (members->value->count >= (members->value->capacity >> 1))
	{
		fckc_size_t next = (fckc_size_t)fck_member_registry_add_next_capacity(members->value->capacity + 1);
		fck_member_registry *result = fck_member_registry_alloc(members->value->assembly, members->value->identifiers, next);
		for (fckc_size_t index = 0; index < members->value->capacity; index++)
		{
			fck_member_info *entry = &members->value->info[index];
			if (fck_identifier_is_null(entry->identifier))
			{
				continue;
			}

			// Re-probe that bad boy
			fckc_size_t new_index = entry->hash % result->capacity;
			for (;;)
			{
				fck_member_info *new_entry = &result->info[new_index];
				if (fck_identifier_is_null(new_entry->identifier))
				{
					SDL_memcpy(new_entry, entry, sizeof(*entry));
					break;
				}
				new_index = (new_index + 1) % result->capacity;
			}
		}

		result->count = members->value->count;
		result->capacity = next;
		fck_member_registry_free(members->value);
		members->value = result;
	}

	fck_member_registry *head = members->value;
	struct fck_type_info *owner_info = fck_type_resolve(owner);

	fck_identifier_desc identifier_desc;
	identifier_desc.name = desc.name;

	fck_identifier identifier = fck_identifiers_add(head->identifiers, identifier_desc);
	const char *str = fck_identifier_resolve(identifier);

	const char *owner_str = fck_identifier_resolve(owner_info->identifier);

	fckc_size_t required_size = SDL_strlen(str) + SDL_strlen(owner_str) + 64;
	char *buffer = (char *)SDL_malloc(required_size);
	fckc_size_t offset = SDL_snprintf(buffer, required_size, "%s::%s", str, owner_str);

	const fck_hash_int hash = fck_hash(buffer, offset);
	fckc_size_t index = ((fckc_size_t)hash) % members->value->capacity;
	SDL_free(buffer);

	for (;;)
	{
		// No need for safe iteration. ONE element IS empty for sure due to pre-condition
		const fck_member_info *current = &members->value->info[index];
		if (fck_identifier_is_null(current->identifier))
		{
			break;
		}
		SDL_assert(!(fck_identifier_is_same(current->identifier, identifier) && fck_type_is_same(current->owner, owner)));
		index = (index + 1) % members->value->capacity;
	}

	// After the while loop above we know index "points" to a valid, empty slot
	fck_member_info *info = members->value->info + index;
	info->hash = hash;
	info->identifier = identifier;
	info->next = fck_member_null();
	info->owner = owner;
	info->type = desc.type;
	info->stride = desc.stride;
	info->extra_count = desc.extra_count;
	members->value->count = members->value->count + 1;
	fck_member member = {members, hash};
	fck_type_info_add_member(owner, member);
	return member;
}

void fck_serialise_members(struct fck_serialiser *serialiser, struct fck_serialiser_params *params, fck_members *v, fckc_size_t c)
{
	if (v == NULL)
	{
		return;
	}

	for (fckc_size_t i = 0; i < c; i++)
	{
		fck_members *members = v + i;

		for (fckc_size_t index = 0; index < members->value->capacity; index++)
		{
			fck_member_info *entry = &members->value->info[index];
			if (!fck_identifier_is_null(entry->identifier))
			{
				fck_serialise_member_info(serialiser, params, entry, 1);
			}
		}
	}
}