#include "fck_type_system.inl"

#include <kll.h>
#include <kll_heap.h>
#include <kll_malloc.h>

#include <fck_hash.h>

#include "fck_set.h"

#include <assert.h>
#include <fck_os.h>

fck_type fck_type_null(void)
{
	return (fck_type){NULL, 0};
}

int fck_type_is_null(fck_type type)
{
	return type.types == NULL;
}

int fck_type_is(fck_type a, const char *str)
{
	return a.types != NULL && (a.hash == fck_hash(str, std->str->unsafe->len(str)));
}

int fck_type_is_same(fck_type a, fck_type b)
{
	return a.types == b.types && a.hash == b.hash;
}

fck_type_info *fck_type_resolve(fck_type handle)
{
	fckc_size_t has = fck_set_find(handle.types->info, handle.hash);
	if (has)
	{
		return &handle.types->info[has - 1];
	}
	return NULL;
}

fck_identifier fck_type_info_identify(struct fck_type_info *info)
{
	assert(info);
	return info->identifier;
}

fck_member fck_type_info_first_member(struct fck_type_info *info)
{
	assert(info);
	return info->first_member;
}

void fck_types_alloc(struct fck_types *types, struct fck_assembly *assembly, struct fck_identifiers *identifiers, fckc_size_t capacity)
{
	assert(types);
	assert(assembly);
	assert(identifiers);
	types->info = fck_set_new(fck_type_info, kll_heap, capacity);

	types->assembly = assembly;
	types->identifiers = identifiers;
}

void fck_types_free(struct fck_types *types)
{
	assert(types);
	fck_set_destroy(types->info);
}

struct fck_assembly *fck_types_assembly(struct fck_types *types)
{
	return types->assembly;
}

fck_type fck_types_add(struct fck_types *types, fck_type_desc desc)
{
	assert(types != NULL);

	fck_identifier_desc identifier_desc;
	identifier_desc.name = desc.name;

	assert(types->identifiers != NULL);
	fck_identifier identifier = fck_identifiers_add(types->identifiers, identifier_desc);

	const char *str = fck_identifier_resolve(identifier);
	const fck_hash_int hash = fck_hash(str, std->str->unsafe->len(str));
	fckc_size_t has = fck_set_find(types->info, hash);
	if (has)
	{
		return (fck_type){types, hash};
	}

	fck_type_info *info = &fck_set_at(types->info, hash);
	info->identifier = identifier;
	info->first_member = fck_member_null();
	info->last_member = fck_member_null();
	return (fck_type){types, hash};
}

fck_type fck_types_find_from_hash(struct fck_types *types, fckc_u64 hash)
{
	assert(types != NULL);

	fckc_size_t has = fck_set_find(types->info, hash);
	if (has)
	{
		return (fck_type){types, hash};
	}
	return fck_type_null();
}

fck_type fck_types_find_from_string(struct fck_types *types, const char *name)
{
	const fck_hash_int hash = fck_hash(name, std->str->unsafe->len(name));
	return fck_types_find_from_hash(types, hash);
}

int fck_types_iterate(struct fck_types *types, fck_type *type)
{
	fck_type_info *info = types->info;

	fckc_size_t at;
	if (fck_type_is_null(*type))
	{
		at = fck_set_begin(info);
	}
	else
	{
		assert(type->types == types);
		fckc_u64 hash = type->hash;
		fckc_size_t has = fck_set_find(info, hash);
		assert(has);
		at = has /*+ 1*/; // Due to how probe works, it is already advanced
	}

	int has_next = fck_set_next(info, at);
	if (has_next)
	{
		const fck_type_info *current = &info[at];
		type->types = types;
		struct fck_set_key *key = fck_set_keys_at(info, at);
		type->hash = fck_set_key_resolve(key);
		return 1;
	}
	return 0;
}

void fck_serialise_types(struct fck_serialiser *serialiser, struct fck_marshal_params *params, fck_types *v, fckc_size_t c)
{
	if (v == NULL)
	{
		return;
	}

	for (fckc_size_t i = 0; i < c; i++)
	{
		fck_types *types = v + i;

		fck_type it = fck_type_null();
		while (fck_types_iterate(types, &it))
		{
			fck_type_info *entry = fck_type_resolve(it);
			fck_serialise_type_info(serialiser, params, entry, 1);
		}
	}
}