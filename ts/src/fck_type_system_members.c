#include "fck_type_system.inl"

#include <kll.h>
#include <kll_heap.h>
#include <kll_malloc.h>

#include <fck_hash.h>

#include "fck_set.h"

#include <assert.h>
#include <fck_os.h>

fck_member fck_member_null(void)
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

	fckc_size_t has = fck_set_find(member.members->info, member.hash);
	if (has)
	{
		fck_member_info *entry = &member.members->info[has - 1];
		return entry;
	}
	return NULL;
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

void fck_members_alloc(fck_members *members, struct fck_assembly *assembly, struct fck_identifiers *identifiers, fckc_size_t capacity)
{
	assert(members);
	assert(assembly);
	assert(identifiers);

	members->info = fck_set_new(fck_member_info, kll_heap, capacity);
	members->assembly = assembly;
	members->identifiers = identifiers;
}

void fck_members_free(struct fck_members *ptr)
{
	assert(ptr);
	fck_set_destroy(ptr->info);
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
	assert(members != NULL);

	fck_identifier_desc identifier_desc;
	identifier_desc.name = desc.name;

	assert(members->identifiers != NULL);
	fck_identifier identifier = fck_identifiers_add(members->identifiers, identifier_desc);

	const char *str = fck_identifier_resolve(identifier);
	const fck_hash_int hash = fck_hash(str, os->str->unsafe->len(str));
	fckc_size_t has = fck_set_find(members->info, hash);
	if (has)
	{
		return (fck_member){members, hash};
	}

	fck_member_info *info = &fck_set_at(members->info, hash);
	info->identifier = identifier;
	info->next = fck_member_null();
	info->owner = owner;
	info->type = desc.type;
	info->stride = desc.stride;
	info->extra_count = desc.extra_count;
	fck_member member = {members, hash};
	fck_type_info_add_member(owner, member);
	return member;
}

void fck_serialise_members(struct fck_serialiser *serialiser, struct fck_marshal_params *params, fck_members *v, fckc_size_t c)
{
	if (v == NULL)
	{
		return;
	}

	for (fckc_size_t i = 0; i < c; i++)
	{
		fck_members *members = v + i;

		fckc_size_t at = fck_set_begin(members->info);
		while (fck_set_next(members->info, at))
		{
			fck_member_info *entry = &members->info[at];
			if (!fck_identifier_is_null(entry->identifier))
			{
				fck_serialise_member_info(serialiser, params, entry, 1);
			}
		}
	}
}