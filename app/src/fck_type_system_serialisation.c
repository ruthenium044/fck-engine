#include "fck_serialiser_vt.h"
#include "fck_stretchy.h"
#include "fck_type_system.h"
#include <SDL3/SDL_stdinc.h>
#include <kll_heap.h>

int fck_stretchy_serialise(struct fck_serialiser *s, struct fck_serialiser_params *p, void **self, fckc_size_t *out_count)
{
	// c has to be -1 cause stretchy...
	// TODO: Make stretchy flag for count
	fck_type_system *ts = p->type_system;

	fck_type type = *p->type;
	struct fck_type_info *type_info = ts->type->resolve(type);
	fck_serialiser_params params = *p;
	params.name = "count";

	fck_stretchy_info *info = fck_stretchy_get_info(*self);
	fckc_u64 count = *self == NULL ? 0 : (fckc_u64)info->size;

	s->vt->u64(s, &params, &count, 1);

	if (*self == NULL)
	{
		if (count == 0)
		{
			return 0;
		}
		fckc_size_t size = p->type_system->type->size_of(type);
		*self = fck_stretchy_alloc(kll_heap, size, 0);
		info = fck_stretchy_get_info(*self);
	}

	for (fckc_size_t index = info->size; index < count; index++)
	{
		info = fck_stretchy_get_info(*self);
		fck_stretchy_expand(self, info->element_size);
		info = fck_stretchy_get_info(*self);
		fckc_u8 *bytes = (fckc_u8 *)(*self);
		SDL_memset(&bytes[index * info->element_size], 0, info->element_size);
	}
	info = fck_stretchy_get_info(*self);
	info->size = count;

	params.name = "data";
	*p = params;
	*out_count = count;
	return 1;
}

void fck_type_serialise_pretty(fck_serialiser *serialiser, fck_serialiser_params *params, void *data, fckc_size_t count);

void fck_type_serialise_members_pretty(fck_serialiser *serialiser, fck_serialiser_params *params, void *data, fck_member members)
{
	fck_type_system *ts = params->type_system;

	while (!ts->member->is_null(members))
	{

		struct fck_member_info *info = ts->member->resolve(members);
		fck_identifier identifier = ts->member->identify(info);
		// parameters.name = ts->identifier->resolve(identifier);

		fckc_u8 *self = ((fckc_u8 *)(data)) + ts->member->stride_of(info);
		fck_type type = ts->member->type_of(info);
		fckc_size_t count = ts->member->count_of(info);
		// parameters.type = &type;
		fck_serialiser_params parameters = fck_serialiser_params_next(params, ts->identifier->resolve(identifier), &type);

		fck_type_serialise_pretty((fck_serialiser *)serialiser, &parameters, (void *)(self), count);

		members = ts->member->next_of(info);
	}
}

void fck_type_serialise_pretty(fck_serialiser *serialiser, fck_serialiser_params *params, void *data, fckc_size_t count)
{
	fck_type type = *params->type;
	fck_type_system *ts = params->type_system;
	struct fck_type_info *info = ts->type->resolve(type);
	fck_identifier owner_identifier = ts->type->identify(info);

	fck_marshal_func *serialise = ts->marshal->get(type);
	if (serialise != NULL)
	{
		serialise((fck_serialiser *)serialiser, params, data, count);
		return;
	}

	fck_member members = ts->type->members_of(info);
	if (ts->member->is_null(members))
	{
		return;
	}

	if (serialiser->vt->pretty->tree_push(serialiser, params, data, count))
	{
		switch (count)
		{
		case 0:
			// We could make this an abstracted concept.
			// template_serialiser serialise = ts->template->get(member)
			// The template_serialiser or however we call it should rebind self/data and hand out a size
			// like the stretchy one is doing it right now. Only requirement: linear memory(maybe?)
			// otherwise we need a iteration abstraction which can be paaaainful :-D
			// if(serialise != NULL && serialise(serialiser, params, (void **)(data), &count)) { ... }
			if (fck_stretchy_serialise(serialiser, params, (void **)(data), &count))
			{
				// This cast to void** and then deref is ugly and shit
				// It is more advisable to let let the xxx_serialise function provide us with a new
				// payload pointer
				fck_type_serialise_pretty(serialiser, params, *(void **)data, count);
			}
			break;
		case 1:
			fck_type_serialise_members_pretty(serialiser, params, data, members);
			break;
		default: {
			fck_serialiser_params p = *params;
			fckc_size_t size = ts->type->size_of(type);
			for (fckc_size_t index = 0; index < count; index++)
			{
				fck_serialiser_params parameters = *params;
				char buffer[32];
				int result = SDL_snprintf(buffer, sizeof(buffer), "[%llu]", (fckc_u64)index);
				parameters.name = buffer;
				fckc_u8 *offset_ptr = ((fckc_u8 *)(data)) + (size * index);

				if (serialiser->vt->pretty->tree_push(serialiser, &parameters, data, 1))
				{
					fck_type_serialise_members_pretty(serialiser, &parameters, offset_ptr, members);
					serialiser->vt->pretty->tree_pop(serialiser, &parameters, data, 1);
				}
			}
		}
		break;
		}
		serialiser->vt->pretty->tree_pop(serialiser, params, data, 0);
	}
}

void fck_type_serialise_ugly(fck_serialiser *serialiser, fck_serialiser_params *params, void *data, fckc_size_t count)
{
	fck_type type = *params->type;
	fck_type_system *ts = params->type_system;

	fck_marshal_func *serialise = ts->marshal->get(type);
	if (serialise != NULL)
	{
		serialise((fck_serialiser *)serialiser, params, data, count);
		return;
	}

	struct fck_type_info *info = ts->type->resolve(type);
	fck_member members = ts->type->members_of(info);
	if (ts->member->is_null(members))
	{
		return;
	}

	if (count == 0)
	{
		if (fck_stretchy_serialise(serialiser, params, (void **)(data), &count))
		{
			fck_type_serialise_ugly(serialiser, params, *(void **)data, count);
		}
		return;
	}

	fckc_size_t size = ts->type->size_of(type);
	for (fckc_size_t index = 0; index < count; index++)
	{
		fck_member current = members;
		while (!ts->member->is_null(current))
		{
			fck_serialiser_params parameters = *params;

			struct fck_member_info *member = ts->member->resolve(current);

			fck_identifier member_identifier = ts->member->identify(member);
			parameters.name = ts->identifier->resolve(member_identifier);
			fckc_u8 *offset_ptr = ((fckc_u8 *)(data)) + ts->member->stride_of(member) + (size * index);
			fck_type member_type = ts->member->type_of(member);
			fckc_size_t primitive_count = ts->member->count_of(member);
			parameters.type = &member_type;

			fck_type_serialise_ugly(serialiser, &parameters, (void *)(offset_ptr), primitive_count);

			current = ts->member->next_of(member);
		}
	}
}

void fck_type_serialise(fck_serialiser *serialiser, fck_serialiser_params *params, void *data, fckc_size_t count)
{
	if (serialiser->vt->pretty != NULL)
	{
		fck_type_serialise_pretty(serialiser, params, data, count);
	}
	else
	{
		fck_type_serialise_ugly(serialiser, params, data, count);
	}
}