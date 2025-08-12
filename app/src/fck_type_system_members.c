#include "fck_type_system.inl"

#include "SDL3/SDL_assert.h"
#include <SDL3/SDL_stdinc.h>

#include <fck_hash.h>

typedef struct fck_member_registry
{
	fckc_size_t count;
	fckc_size_t capacity;

	struct fck_identifiers *identifiers;

	fck_member_info info[1];
} fck_member_registry;

typedef struct fck_members
{
	struct fck_member_registry *value;
} fck_members;

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
	fckc_size_t index = member.hash % member.members->value->capacity;
	while (1)
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
		index = index + 1;
	}
}

fck_identifier fck_member_info_identify(struct fck_member_info *info)
{
	return info->identifier;
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

fck_member_registry *fck_member_registry_alloc(struct fck_identifiers *identifiers, fckc_size_t capacity)
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

	registry->count = 0;
	registry->capacity = capacity;
	registry->identifiers = identifiers;
	return registry;
}

struct fck_members *fck_members_alloc(struct fck_identifiers *identifiers, fckc_size_t capacity)
{
	fck_members *members = SDL_malloc(sizeof(*members));
	members->value = fck_member_registry_alloc(identifiers, capacity);
	return members;
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

fck_member fck_members_add(fck_members *registry, fck_member_desc desc)
{
	// TODO: realloc
	fck_member_registry *head = registry->value;
	struct fck_type_info *owner_info = fck_type_resolve(desc.owner);

	fck_identifier_desc identifier_desc;
	identifier_desc.name = desc.name;

	fck_identifier identifier = fck_identifiers_add(head->identifiers, identifier_desc);
	const char *str = fck_identifier_resolve(identifier);

	const char *owner_str = fck_identifier_resolve(owner_info->identifier);

	fckc_size_t required_size = SDL_strlen(str) + SDL_strlen(owner_str) + 64;
	char *buffer = (char *)SDL_malloc(required_size);
	fckc_size_t offset = SDL_snprintf(buffer, required_size, "%s %s", str, owner_str);

	const fck_hash_int hash = fck_hash(buffer, offset);
	fckc_size_t index = ((fckc_size_t)hash) % registry->value->capacity;
	SDL_free(buffer);

	while (1)
	{
		// No need for safe iteration. ONE element IS empty for sure due to pre-condition
		const fck_member_info *current = &registry->value->info[index];
		if (fck_identifier_is_null(current->identifier))
		{
			break;
		}
		SDL_assert(!(fck_identifier_is_same(current->identifier, identifier) && fck_type_is_same(current->owner, desc.owner)));
		index = (index + 1) % registry->value->capacity;
	}

	// After the while loop above we know index "points" to a valid, empty slot
	fck_member_info *info = registry->value->info + index;
	info->hash = hash;
	info->identifier = identifier;
	info->next = fck_member_null();
	info->owner = desc.owner;
	info->type = desc.type;
	info->stride = desc.stride;
	registry->value->count = registry->value->count + 1;
	fck_member member = {registry, hash};
	fck_type_info_add_member(desc.owner, member);
	return member;
}
