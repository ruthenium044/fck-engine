#include "fck_type_system.h"

#include "SDL3/SDL_assert.h"
#include <SDL3/SDL_stdinc.h>

typedef struct fck_serialiser_info
{
	fck_type type;
	fck_serialise_func *serialise;
} fck_serialiser_info;

typedef struct fck_serialiser_registry
{
	fckc_size_t count;
	fckc_size_t capacity;
	fck_serialiser_info info[1];
} fck_serialiser_registry;

typedef struct fck_serialise_interfaces
{
	struct fck_serialiser_registry *value;
} fck_serialise_interfaces;

static fck_serialiser_registry *fck_serialiser_registry_alloc(fckc_size_t capacity)
{
	fckc_size_t size = offsetof(fck_serialiser_registry, info[capacity]);
	fck_serialiser_registry *registry = (fck_serialiser_registry *)SDL_malloc(size);

	for (fckc_size_t index = 0; index < capacity; index++)
	{
		fck_serialiser_info *entry = &registry->info[index];
		entry->type = fck_type_null();
		entry->serialise = NULL;
	}

	registry->count = 0;
	registry->capacity = capacity;
	return registry;
}

struct fck_serialise_interfaces *fck_serialise_interfaces_alloc(fckc_size_t capacity)
{
	fck_serialise_interfaces *interfaces = (fck_serialise_interfaces *)SDL_malloc(sizeof(*interfaces));
	interfaces->value = fck_serialiser_registry_alloc(capacity);
	return interfaces;
}

void fck_serialiser_registry_free(fck_serialiser_registry *registry)
{
	SDL_assert(registry);
	SDL_free(registry);
}

void fck_serialise_interfaces_free(struct fck_serialise_interfaces *interfaces)
{
	SDL_assert(interfaces->value);
	fck_serialiser_registry_free(interfaces->value);
	SDL_free(interfaces);
}

void fck_serialise_interfaces_add(struct fck_serialise_interfaces *interfaces, fck_serialise_desc desc)
{
	// TODO: resize
	SDL_assert(interfaces);
	SDL_assert(!fck_type_is_null(desc.type));

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
}

fck_serialise_func *fck_serialise_interfaces_get(struct fck_serialise_interfaces *interfaces, fck_type type)
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
