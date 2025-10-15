#include "fck_type_system.inl"

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_stdinc.h>

#include <kll.h>
#include <kll_heap.h>
#include <kll_malloc.h>

#include <fck_hash.h>

#include "fck_set.h"

typedef struct fck_marshal_info
{
	fck_type type;
	fck_marshal_func *func;
} fck_marshal_info;

void fck_marshal_alloc(struct fck_marshal *marshal, struct fck_assembly *assembly, fckc_size_t capacity)
{
	marshal->infoo = fck_set_new(fck_marshal_info, kll_heap, capacity);
	marshal->assembly = assembly;
}

void fck_marshal_free(struct fck_marshal *interfaces)
{
	SDL_assert(interfaces);
	fck_set_destroy(interfaces->infoo);
}

void fck_marshal_add(struct fck_marshal *marshal, fck_marshal_desc desc)
{
	SDL_assert(marshal);
	SDL_assert(!fck_type_is_null(desc.type));

	fckc_size_t has = fck_set_find(marshal->infoo, desc.type.hash);
	if (has)
	{
		// We do NOTHING meaningful here, maybe assert? 
		return;
	}

	fck_marshal_info *info = &fck_set_at(marshal->infoo, desc.type.hash);
	info->type = desc.type;
	info->func = desc.func;
}

// TODO: Make it invoke!!!!!!!!!!
fck_marshal_func *fck_marshal_get(struct fck_marshal *marshal, fck_type type)
{
	SDL_assert(marshal);
	SDL_assert(!fck_type_is_null(type));

	fckc_size_t has = fck_set_find(marshal->infoo, type.hash);
	if (has)
	{
		const fck_marshal_info *info = &marshal->infoo[has - 1];
		return info->func;
	}
	return NULL;
}
