#include "fck_type_system.inl"

#include "SDL3/SDL_assert.h"
#include <SDL3/SDL_stdinc.h>

typedef struct fck_serialiser_info
{
	fck_type type;
	fck_marshal_func *serialise;
} fck_serialiser_info;

typedef struct fck_marshal_registry
{
	struct fck_assembly *assembly;

	fckc_size_t count;
	fckc_size_t capacity;

	fck_serialiser_info info[1];
} fck_marshal_registry;

static fckc_u64 fck_marshal_registry_add_next_capacity(fckc_u64 n)
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

fck_marshal_registry *fck_marshal_registry_alloc(struct fck_assembly *assembly, fckc_size_t capacity)
{
	fckc_size_t size = offsetof(fck_marshal_registry, info[capacity]);
	fck_marshal_registry *registry = (fck_marshal_registry *)SDL_malloc(size);

	for (fckc_size_t index = 0; index < capacity; index++)
	{
		fck_serialiser_info *entry = &registry->info[index];
		entry->type = fck_type_null();
		entry->serialise = NULL;
	}
	registry->assembly = assembly;
	registry->count = 0;
	registry->capacity = capacity;
	return registry;
}

void fck_marshal_registry_free(fck_marshal_registry *registry)
{
	SDL_assert(registry);
	SDL_free(registry);
}

void fck_marshal_free(struct fck_marshal*interfaces)
{
	SDL_assert(interfaces->value);
	fck_marshal_registry_free(interfaces->value);
	SDL_free(interfaces);
}

void fck_marshal_add(struct fck_marshal*interfaces, fck_marshal_desc desc)
{
	SDL_assert(interfaces);
	SDL_assert(!fck_type_is_null(desc.type));

	// Maybe resize
	if (interfaces->value->count >= (interfaces->value->capacity >> 1))
	{
		fckc_size_t next = fck_marshal_registry_add_next_capacity(interfaces->value->capacity + 1);
		fck_marshal_registry *result = fck_marshal_registry_alloc(interfaces->value->assembly, next);

		for (fckc_size_t index = 0; index < interfaces->value->capacity; index++)
		{
			fck_serialiser_info *entry = &interfaces->value->info[index];
			// Maybe check for func == NULL?
			if (fck_type_is_null(entry->type))
			{
				continue;
			}

			fckc_size_t new_index = entry->type.hash % result->capacity;
			while (1)
			{
				fck_serialiser_info *new_entry = &result->info[new_index];
				if (fck_type_is_null(new_entry->type))
				{
					SDL_memcpy(new_entry, entry, sizeof(*entry));
					break;
				}
				new_index = (new_index + 1) % result->capacity;
			}
		}

		result->count = interfaces->value->count;
		result->capacity = next;
		fck_marshal_registry_free(interfaces->value);
		interfaces->value = result;
	}

	fckc_size_t index = ((fckc_size_t)desc.type.hash) % interfaces->value->capacity;
	while (1)
	{
		const fck_serialiser_info *current = &interfaces->value->info[index];
		if (fck_type_is_null(current->type))
		{
			break;
		}
		SDL_assert(!fck_type_is_same(desc.type, current->type) && "Already added, do not add twice?!");
		index = (index + 1) % interfaces->value->capacity;
	}

	fck_serialiser_info *info = interfaces->value->info + index;
	info->type = desc.type;
	info->serialise = desc.func;
	interfaces->value->count = interfaces->value->count + 1;
}

fck_marshal_func * fck_marshal_get(struct fck_marshal*interfaces, fck_type type)
{
	SDL_assert(interfaces);
	SDL_assert(!fck_type_is_null(type));

	fckc_size_t index = ((fckc_size_t)type.hash) % interfaces->value->capacity;
	while (1)
	{
		const fck_serialiser_info *current = &interfaces->value->info[index];
		if (fck_type_is_null(current->type))
		{
			return NULL;
		}
		if (fck_type_is_same(type, current->type))
		{
			return current->serialise;
		}
		index = (index + 1) % interfaces->value->capacity;
	}
}
