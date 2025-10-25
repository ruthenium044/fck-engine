#define FCK_TYPE_SYSTEM_EXPORT
#include "fck_type_system.h"

#include "fck_serialiser_vt.h"
#include "fck_dynarr.h"

#include <fck_os.h>
#include <kll_heap.h>

static inline fck_marshal_params fck_serialiser_params_next(fck_marshal_params *prev, const char *name, struct fck_type *type)
{
	fck_marshal_params p = *prev;
	p.type = type;
	p.name = name;
	return p;
}

int fck_dynarr_serialise(fck_marshaller *marshaller, fck_marshal_params *p, void **self, fckc_size_t *out_count)
{
	// c has to be -1 cause dynarr...
	// TODO: Make dynarr flag for count
	fck_type_system *ts = marshaller->type_system;

	fck_type type = *p->type;
	struct fck_type_info *type_info = ts->type->resolve(type);
	fck_serialiser_params params;
	params.name = "count";

	fck_dynarr_info *info = fck_dynarr_get_info(*self);
	fckc_u64 count = *self == NULL ? 0 : (fckc_u64)info->size;

	marshaller->serialiser->vt->u64(marshaller->serialiser, &params, &count, 1);

	if (*self == NULL)
	{
		if (count == 0)
		{
			return 0;
		}
		fckc_size_t size = ts->type->size_of(type);
		*self = fck_dynarr_alloc(kll_heap, size, 0);
		info = fck_dynarr_get_info(*self);
	}

	for (fckc_size_t index = info->size; index < count; index++)
	{
		info = fck_dynarr_get_info(*self);
		fck_dynarr_expand(self, info->element_size);
		info = fck_dynarr_get_info(*self);
		fckc_u8 *bytes = (fckc_u8 *)(*self);
		os->mem->set(&bytes[index * info->element_size], 0, info->element_size);
	}
	info = fck_dynarr_get_info(*self);
	info->size = count;

	p->name = "data";
	*out_count = count;
	return 1;
}

void fck_type_serialise_pretty(fck_marshaller *marshaller, fck_marshal_params *params, void *data, fckc_size_t count);

void fck_type_serialise_members_pretty(fck_marshaller *marshaller, fck_marshal_params *params, void *data, fck_member members)
{
	fck_type_system *ts = marshaller->type_system;

	while (!ts->member->is_null(members))
	{

		struct fck_member_info *info = ts->member->resolve(members);
		fck_identifier identifier = ts->member->identify(info);
		// parameters.name = ts->identifier->resolve(identifier);

		fckc_u8 *self = ((fckc_u8 *)(data)) + ts->member->stride_of(info);
		fck_type type = ts->member->type_of(info);
		fckc_size_t count = ts->member->count_of(info);
		// parameters.type = &type;
		fck_marshal_params parameters = fck_serialiser_params_next(params, ts->identifier->resolve(identifier), &type);

		fck_type_serialise_pretty(marshaller, &parameters, (void *)(self), count);

		members = ts->member->next_of(info);
	}
}

void fck_type_serialise_pretty(fck_marshaller *marshaller, fck_marshal_params *params, void *data, fckc_size_t count)
{
	fck_type type = *params->type;
	fck_type_system *ts = marshaller->type_system;
	struct fck_type_info *info = ts->type->resolve(type);
	fck_identifier owner_identifier = ts->type->identify(info);

	fck_marshal_func *serialise = ts->marshal->get(type);
	if (serialise != NULL)
	{
		serialise(marshaller->serialiser, params, data, count);
		return;
	}

	fck_member members = ts->type->members_of(info);
	if (ts->member->is_null(members))
	{
		return;
	}

	if (marshaller->serialiser->vt->pretty->tree_push(marshaller, params, data, count))
	{
		switch (count)
		{
		case 0:
			// We could make this an abstracted concept.
			// template_serialiser serialise = ts->template->get(member)
			// The template_serialiser or however we call it should rebind self/data and hand out a size
			// like the dynarr one is doing it right now. Only requirement: linear memory(maybe?)
			// otherwise we need a iteration abstraction which can be paaaainful :-D
			// if(serialise != NULL && serialise(serialiser, params, (void **)(data), &count)) { ... }
			if (fck_dynarr_serialise(marshaller, params, (void **)(data), &count))
			{
				// This cast to void** and then deref is ugly and shit
				// It is more advisable to let let the xxx_serialise function provide us with a new
				// payload pointer
				fck_type_serialise_pretty(marshaller, params, *(void **)data, count);
			}
			break;
		case 1:
			fck_type_serialise_members_pretty(marshaller, params, data, members);
			break;
		default: {
			fck_marshal_params p = *params;
			fckc_size_t size = ts->type->size_of(type);
			for (fckc_size_t index = 0; index < count; index++)
			{
				fck_marshal_params parameters = *params;
				char buffer[32];
				int result = os->io->snprintf(buffer, sizeof(buffer), "[%llu]", (fckc_u64)index);
				parameters.name = buffer;
				fckc_u8 *offset_ptr = ((fckc_u8 *)(data)) + (size * index);

				if (marshaller->serialiser->vt->pretty->tree_push(marshaller, &parameters, data, 1))
				{
					fck_type_serialise_members_pretty(marshaller, &parameters, offset_ptr, members);
					marshaller->serialiser->vt->pretty->tree_pop(marshaller, &parameters, data, 1);
				}
			}
		}
		break;
		}
		marshaller->serialiser->vt->pretty->tree_pop(marshaller, params, data, 0);
	}
}

void fck_type_serialise_ugly(fck_marshaller *marshaller, fck_marshal_params *params, void *data, fckc_size_t count)
{
	fck_type type = *params->type;
	fck_type_system *ts = marshaller->type_system;

	fck_marshal_func *serialise = ts->marshal->get(type);
	if (serialise != NULL)
	{
		serialise(marshaller->serialiser, params, data, count);
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
		if (fck_dynarr_serialise(marshaller, params, (void **)(data), &count))
		{
			fck_type_serialise_ugly(marshaller, params, *(void **)data, count);
		}
		return;
	}

	fckc_size_t size = ts->type->size_of(type);
	for (fckc_size_t index = 0; index < count; index++)
	{
		fck_member current = members;
		while (!ts->member->is_null(current))
		{
			fck_marshal_params parameters = *params;

			struct fck_member_info *member = ts->member->resolve(current);

			fck_identifier member_identifier = ts->member->identify(member);
			parameters.name = ts->identifier->resolve(member_identifier);
			fckc_u8 *offset_ptr = ((fckc_u8 *)(data)) + ts->member->stride_of(member) + (size * index);
			fck_type member_type = ts->member->type_of(member);
			fckc_size_t primitive_count = ts->member->count_of(member);
			parameters.type = &member_type;

			fck_type_serialise_ugly(marshaller, &parameters, (void *)(offset_ptr), primitive_count);

			current = ts->member->next_of(member);
		}
	}
}

void fck_type_serialise(fck_marshaller *marshaller, fck_marshal_params *params, void *data, fckc_size_t count)
{
	if (marshaller->serialiser->vt->pretty != NULL)
	{
		fck_type_serialise_pretty(marshaller, params, data, count);
	}
	else
	{
		fck_type_serialise_ugly(marshaller, params, data, count);
	}
}