#include "fck_instance.h"

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_render.h>

#include <fck_hash.h>

#include "fck_ui.h"

#include "fck_nuklear_demos.h"
#include "fck_ui_window_manager.h"

#include "fck_serialiser.h"
#include "fck_serialiser_byte_vt.h"
#include "fck_serialiser_json_vt.h"
#include "fck_serialiser_nk_edit_vt.h"
#include "fck_serialiser_string_vt.h"
#include "fck_serialiser_vt.h"

#include "kll_heap.h"

#ifndef offsetof
#define offsetof(st, m) ((fckc_uptr) & (((st *)0)->m))
#endif

typedef fckc_u32 fck_entity_index;
typedef struct fck_entity
{
	fck_entity_index id;
} fck_entity;

typedef fckc_u32 fck_entity_index;
typedef struct fck_entity_set
{
	fckc_size_t count;
	fckc_size_t capacity;

	fck_entity_index id[1];
} fck_entity_set;

typedef struct fck_identifier_registry_entry
{
	fck_hash_int hash;
	char *str;
} fck_identifier_registry_entry;

typedef struct fck_identifier_registry
{
	fckc_size_t count;
	fckc_size_t capacity;

	fck_identifier_registry_entry identifiers[1]; // ...
} fck_identifier_registry;

typedef struct fck_identifier_registry_ref
{
	fck_identifier_registry *value;
} fck_identifier_registry_ref;

typedef struct fck_identifier
{
	const fck_identifier_registry_ref *table;
	fck_hash_int hash;
} fck_identifier;

static fckc_u64 fck_registry_add_next_capacity(fckc_u64 n)
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

fck_identifier_registry *fck_identifier_registry_alloc(fckc_size_t capacity)
{
	const fckc_size_t size = offsetof(fck_identifier_registry, identifiers[capacity]);
	fck_identifier_registry *table = (fck_identifier_registry *)SDL_malloc(size);

	for (fckc_size_t index = 0; index < capacity; index++)
	{
		fck_identifier_registry_entry *entry = &table->identifiers[index];
		entry->hash = 0;
		entry->str = NULL;
	}

	table->capacity = capacity;
	table->count = 0;
	return table;
}

fck_identifier_registry *fck_identifier_registry_free(fck_identifier_registry *ptr)
{
	SDL_free(ptr);
	return NULL;
}

fck_identifier fck_identifier_null()
{
	return (fck_identifier){NULL, 0};
}

int fck_identifier_is_null(fck_identifier str)
{
	return str.table == NULL;
}

int fck_identifier_is_same(fck_identifier a, fck_identifier b)
{
	return a.table == b.table && a.hash == b.hash;
}

fck_identifier fck_identifiers_add(fck_identifier_registry_ref *identifiers_ref, const char *str)
{
	SDL_assert(identifiers_ref != NULL);

	if (identifiers_ref->value->count >= (identifiers_ref->value->capacity >> 1))
	{
		// Realloc if required
		fckc_size_t next_capacity = fck_registry_add_next_capacity(identifiers_ref->value->capacity + 1); // + 1... I think
		fck_identifier_registry *result = fck_identifier_registry_alloc(next_capacity);

		for (fckc_size_t index = 0; index < identifiers_ref->value->capacity; index++)
		{
			fck_identifier_registry_entry *entry = &identifiers_ref->value->identifiers[index];
			if (entry->str == NULL)
			{
				continue;
			}

			fckc_size_t new_index = entry->hash % result->capacity;
			while (1)
			{
				fck_identifier_registry_entry *new_entry = &identifiers_ref->value->identifiers[new_index];
				if (new_entry->str == NULL)
				{
					SDL_memcpy(new_entry, entry, sizeof(*entry));
					break;
				}
				new_index = new_index + 1 % result->capacity;
			}
		}

		result->capacity = next_capacity;
		result->count = identifiers_ref->value->count;
		fck_identifier_registry_free(identifiers_ref->value);
		identifiers_ref->value = result;
	}

	{
		// Add or get...
		fck_identifier_registry *identifiers = identifiers_ref->value;

		fck_hash_int hash = fck_hash(str, strlen(str));
		fckc_size_t index = hash % identifiers->capacity;

		// That this while loop breaks is enforced through the realloc and size check
		while (1)
		{
			fck_identifier_registry_entry *entry = &identifiers->identifiers[index];
			const char *string = entry->str;
			if (entry->hash == hash)
			{
				SDL_assert(SDL_strcmp(str, entry->str) == 0);
				return (fck_identifier){identifiers_ref, hash};
			}
			if (string == NULL)
			{
				// Newly added
				entry->hash = hash;
				entry->str = SDL_strdup(str);
				identifiers_ref->value->count = identifiers_ref->value->count + 1;
				return (fck_identifier){identifiers_ref, hash};
			}
			index = index + 1;
		}
	}
}

fck_identifier fck_identifier_find_from_hash(fck_identifier_registry_ref *identifiers, fck_hash_int hash)
{
	SDL_assert(identifiers != NULL);

	fckc_size_t index = hash % identifiers->value->capacity;

	// That this while loop breaks is enforced through the realloc and size check
	while (1)
	{
		fck_identifier_registry_entry *entry = &identifiers->value->identifiers[index];
		const char *string = entry->str;
		if (entry->hash == hash)
		{
			return (fck_identifier){identifiers, hash};
		}
		if (string == NULL)
		{
			return (fck_identifier){NULL, 0};
		}
		index = index + 1;
	}
}

fck_identifier fck_identifier_find_from_string(fck_identifier_registry_ref *identifiers, const char *str)
{
	SDL_assert(identifiers != NULL);

	fck_hash_int hash = fck_hash(str, strlen(str));
	return fck_identifier_find_from_hash(identifiers, hash);
}

const char *fck_identifier_resolve(fck_identifier identifier)
{
	fckc_size_t index = identifier.hash % identifier.table->value->capacity;
	while (1)
	{
		fck_identifier_registry_entry *entry = &identifier.table->value->identifiers[index];
		const char *str = entry->str;

		if (entry->hash == identifier.hash)
		{
			return str;
		}
		if (str == NULL)
		{
			return NULL;
		}
		index = index + 1;
	}
}

typedef struct fck_type_handle
{
	struct fck_type_registry_ref *registry;
	fckc_size_t hash;
} fck_type_handle;

typedef struct fck_member_handle
{
	struct fck_member_registry_ref *registry;
	fckc_size_t hash;
} fck_member_handle;

typedef struct fck_type_info
{
	fck_hash_int hash;
	fck_identifier identifier;
	fck_member_handle first_member;
	fck_member_handle last_member;
} fck_type_info;

typedef struct fck_member_info
{
	fck_hash_int hash;
	fck_type_handle owner;
	fck_identifier identifier;
	fck_type_handle type;
	fckc_size_t stride;
	fck_member_handle next;
} fck_member_info;

typedef struct fck_member_registry
{
	fckc_size_t count;
	fckc_size_t capacity;

	fck_identifier_registry_ref *identifiers;

	fck_member_info info[1];
} fck_member_registry;

typedef struct fck_type_registry
{
	fckc_size_t count;
	fckc_size_t capacity;

	fck_identifier_registry_ref *identifiers;

	fck_type_info info[1];
} fck_type_registry;

typedef struct fck_member_registry_ref
{
	fck_member_registry *value;
} fck_member_registry_ref;

typedef struct fck_type_registry_ref
{
	fck_type_registry *value;
} fck_type_registry_ref;

typedef void (*fck_serialise_function)(fck_serialiser *, fck_serialiser_params *, void *, fckc_size_t);

#define FCK_SERIALISE_FUNC(x) (fck_serialise_function)(x)

typedef struct fck_serialiser_info
{
	fck_type_handle type;
	fck_serialise_function serialise;
} fck_serialiser_info;

typedef struct fck_serialiser_registry
{
	fckc_size_t count;
	fckc_size_t capacity;
	fck_serialiser_info info[1];
} fck_serialiser_registry;

typedef struct fck_serialiser_registry_ref
{
	fck_serialiser_registry *value;
} fck_serialiser_registry_ref;

fck_type_handle fck_type_handle_null()
{
	return (fck_type_handle){NULL, 0};
}

fck_member_handle fck_member_handle_null()
{
	return (fck_member_handle){NULL, 0};
}

int fck_type_handle_is_null(fck_type_handle handle)
{
	return handle.registry == NULL;
}

int fck_member_handle_is_null(fck_member_handle handle)
{
	return handle.registry == NULL;
}

int fck_type_handle_is_same(fck_type_handle a, fck_type_handle b)
{
	return a.registry == b.registry && a.hash == b.hash;
}

int fck_type_is(fck_type_handle a, const char *str)
{
	return a.registry != NULL && (a.hash == fck_hash(str, SDL_strlen(str)));
}

int fck_member_handle_is_same(fck_member_handle a, fck_member_handle b)
{
	return a.registry == b.registry && a.hash == b.hash;
}

fck_type_info *fck_type_handle_resolve(fck_type_handle handle)
{
	fckc_size_t index = handle.hash % handle.registry->value->capacity;
	while (1)
	{
		fck_type_info *entry = &handle.registry->value->info[index];

		if (entry->hash == handle.hash)
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

fck_member_info *fck_member_handle_resolve(fck_member_handle handle)
{
	fckc_size_t index = handle.hash % handle.registry->value->capacity;
	while (1)
	{
		fck_member_info *entry = &handle.registry->value->info[index];

		if (entry->hash == handle.hash)
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

fck_type_registry *fck_type_registry_alloc(fckc_size_t capacity, fck_identifier_registry_ref *identifiers)
{
	fckc_size_t size = offsetof(fck_type_registry, info[capacity]);
	fck_type_registry *registry = (fck_type_registry *)SDL_malloc(size);
	for (fckc_size_t index = 0; index < capacity; index++)
	{
		fck_type_info *entry = &registry->info[index];
		entry->hash = 0;
		entry->identifier = fck_identifier_null();
	}

	registry->count = 0;
	registry->capacity = capacity;
	registry->identifiers = identifiers;
	return registry;
}

void fck_type_registry_free(fck_type_registry *registry)
{
	SDL_free(registry);
}

fck_type_handle fck_type_registry_add(fck_type_registry_ref *registry, const char *name)
{
	SDL_assert(registry != NULL);

	if (registry->value->count >= (registry->value->capacity >> 1))
	{
		// Realloc if required
		fckc_size_t next_capacity = fck_registry_add_next_capacity(registry->value->capacity + 1); // + 1... I think
		fck_type_registry *result = fck_type_registry_alloc(next_capacity, registry->value->identifiers);

		for (fckc_size_t index = 0; index < registry->value->capacity; index++)
		{
			fck_type_info *entry = &registry->value->info[index];
			if (fck_identifier_is_null(entry->identifier))
			{
				continue;
			}

			fckc_size_t new_index = entry->hash % result->capacity;
			while (1)
			{
				fck_type_info *new_entry = &registry->value->info[new_index];
				if (fck_identifier_is_null(new_entry->identifier))
				{
					SDL_memcpy(new_entry, entry, sizeof(*entry));
					break;
				}
				new_index = new_index + 1 % result->capacity;
			}
		}

		result->capacity = next_capacity;
		result->count = registry->value->count;
		fck_type_registry_free(registry->value);
		registry->value = result;
	}

	fck_type_registry *head = registry->value;

	fck_identifier identifier = fck_identifiers_add(head->identifiers, name);
	const char *str = fck_identifier_resolve(identifier);
	const fck_hash_int hash = fck_hash(str, SDL_strlen(str));
	fckc_size_t index = ((fckc_size_t)hash) % registry->value->capacity;

	while (1)
	{
		// No need for safe iteration. ONE element IS empty for sure due to pre-condition
		const fck_type_info *current = &registry->value->info[index];
		if (fck_identifier_is_null(current->identifier))
		{
			break;
		}
		if (fck_identifier_is_same(current->identifier, identifier))
		{
			// ALREADY ADDED MAYBE ERROR?! WHAT THE ACUTAL FUCK
			return (fck_type_handle){registry, hash};
		}
		index = (index + 1) % registry->value->capacity;
	}

	// After the while loop above we know index "points" to a valid, empty slot
	fck_type_info *info = registry->value->info + index;
	info->hash = hash;
	info->identifier = identifier;
	info->first_member = fck_member_handle_null();
	info->last_member = fck_member_handle_null();
	registry->value->count = registry->value->count + 1;
	return (fck_type_handle){registry, hash};
}

fck_type_handle fck_type_registry_get(fck_type_registry_ref *registry, const char *name)
{
	const fck_hash_int hash = fck_hash(name, SDL_strlen(name));
	fckc_size_t index = ((fckc_size_t)hash) % registry->value->capacity;
	fck_type_handle handle = {registry, hash};
	while (1)
	{
		// No need for safe iteration. ONE element IS empty for sure due to pre-condition
		const fck_type_info *current = &registry->value->info[index];
		if (fck_identifier_is_null(current->identifier))
		{
			return fck_type_handle_null();
		}
		if (current->hash == hash)
		{
			SDL_assert(SDL_strcmp(name, fck_identifier_resolve(current->identifier)) == 0);
			return (fck_type_handle){registry, hash};
			// ALREADY ADDED MAYBE ERROR?! WHAT THE ACUTAL FUCK
		}
		index = (index + 1) % registry->value->capacity;
	}

	return handle;
}

void fck_type_info_add_member(fck_type_handle type_handle, fck_member_handle member_handle)
{
	fck_type_info *info = fck_type_handle_resolve(type_handle);
	fck_member_info *member = fck_member_handle_resolve(member_handle);

	member->owner = type_handle;
	member->next = fck_member_handle_null();
	if (fck_member_handle_is_null(info->first_member))
	{
		info->first_member = member_handle;
		info->last_member = member_handle;
		return;
	}
	// else
	fck_member_info *last_member = fck_member_handle_resolve(info->last_member);
	last_member->next = member_handle;
	info->last_member = member_handle;
};

fck_member_registry *fck_member_registry_alloc(fckc_size_t capacity, fck_identifier_registry_ref *identifiers_ref)
{
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
	registry->identifiers = identifiers_ref;
	return registry;
}

void fck_member_registry_free(fck_member_registry *registry)
{
	SDL_free(registry);
}

typedef struct fck_member_desc
{
	fck_type_handle type;
	fck_type_handle owner;
	const char *name;
	fckc_size_t stride;
} fck_member_desc;

fck_member_handle fck_member_registry_add(fck_member_registry_ref *registry, fck_member_desc desc)
{
	fck_member_registry *head = registry->value;
	fck_type_info *owner_info = fck_type_handle_resolve(desc.owner);

	fck_identifier identifier = fck_identifiers_add(head->identifiers, desc.name);
	const char *str = fck_identifier_resolve(identifier);
	const char *owner_str = fck_identifier_resolve(owner_info->identifier);

	fckc_size_t required_size = SDL_strlen(str) + SDL_strlen(owner_str) + 64;
	fckc_u8 *buffer = (fckc_u8 *)SDL_malloc(required_size);
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
		SDL_assert(!(fck_identifier_is_same(current->identifier, identifier) && fck_type_handle_is_same(current->owner, desc.owner)));
		index = (index + 1) % registry->value->capacity;
	}

	// After the while loop above we know index "points" to a valid, empty slot
	fck_member_info *info = registry->value->info + index;
	info->hash = hash;
	info->identifier = identifier;
	info->next = fck_member_handle_null();
	info->owner = desc.owner;
	info->type = desc.type;
	info->stride = desc.stride;
	registry->value->count = registry->value->count + 1;
	fck_member_handle member = {registry, hash};
	fck_type_info_add_member(desc.owner, member);
	return member;
}

fck_serialiser_registry *fck_serialiser_registry_alloc(fckc_size_t capacity)
{
	fckc_size_t size = offsetof(fck_serialiser_registry, info[capacity]);
	fck_serialiser_registry *registry = (fck_serialiser_registry *)SDL_malloc(size);

	for (fckc_size_t index = 0; index < capacity; index++)
	{
		fck_serialiser_info *entry = &registry->info[index];
		entry->type = fck_type_handle_null();
		entry->serialise = NULL;
	}

	registry->count = 0;
	registry->capacity = capacity;
	return registry;
}

void fck_serialiser_registry_free(fck_serialiser_registry *registry)
{
	SDL_free(registry);
}

void fck_serialiser_registry_add(fck_serialiser_registry_ref *registry, fck_type_handle type, fck_serialise_function serialise_func)
{
	SDL_assert(registry);
	SDL_assert(!fck_type_handle_is_null(type));

	fckc_size_t index = ((fckc_size_t)type.hash) % registry->value->capacity;
	while (1)
	{
		const fck_serialiser_info *current = &registry->value->info[index];
		if (fck_type_handle_is_null(current->type))
		{
			break;
		}
		SDL_assert(!fck_type_handle_is_same(type, current->type) && "Already added, do not add twice?!");
		index = (index + 1) % registry->value->capacity;
	}

	fck_serialiser_info *info = registry->value->info + index;
	info->type = type;
	info->serialise = serialise_func;
}

fck_serialise_function fck_serialiser_registry_get(fck_serialiser_registry_ref *registry, fck_type_handle type)
{
	SDL_assert(registry);
	SDL_assert(!fck_type_handle_is_null(type));

	fckc_size_t index = ((fckc_size_t)type.hash) % registry->value->capacity;
	while (1)
	{
		const fck_serialiser_info *current = &registry->value->info[index];
		if (fck_type_handle_is_null(current->type))
		{
			return NULL;
		}
		if (fck_type_handle_is_same(type, current->type))
		{
			return current->serialise;
		}
		index = (index + 1) % registry->value->capacity;
	}
}

typedef struct fck_component_info
{
	fck_type_info *type;
} fck_component_info;

typedef fckc_u16 fck_component_id;

typedef struct fck_component_set
{
	fck_component_info info;

	fckc_u8 *memory;

	fck_component_id *lookup;
	fck_component_id *owners;
	fckc_u8 *data;

	fckc_size_t count;
	fckc_size_t capacity;
} fck_component_set;

typedef struct fckc_components
{
	fckc_size_t count;
	fckc_size_t capacity;

	fck_component_set sets[1];
} fckc_components;

typedef struct fck_ecs
{
	fck_entity_set *entities;
	fckc_components *components;
} fck_ecs;

fckc_components *fck_components_alloc(fckc_size_t capacity)
{
	const fckc_size_t size = offsetof(fckc_components, sets[capacity]);
	fckc_components *components = (fckc_components *)SDL_malloc(size);
	components->capacity = capacity;
	components->count = 0;
	return components;
}

fck_identifier_registry_ref identifiers;
fck_member_registry_ref members;
fck_type_registry_ref types;
fck_serialiser_registry_ref serialisers;

#define fck_id(s) #s

typedef struct float2
{
	float x;
	float y;
} float2;

typedef struct float3
{
	float x;
	float y;
	float z;
} float3;

typedef struct example_type
{
	double double_value;
	float cooldown;
	float2 position;
	float3 rgb;
	fckc_u32 some_int;
} example_type;

void fck_type_add_u32(fck_member_registry_ref *registry, fck_type_handle type, const char *name, fckc_size_t stride)
{
	fck_type_handle member_type = fck_type_registry_get(&types, fck_id(fckc_u32));
	fck_member_registry_add(registry, (fck_member_desc){.type = member_type, .name = name, .owner = type, .stride = stride});
}

void fck_type_add_double(fck_member_registry_ref *registry, fck_type_handle type, const char *name, fckc_size_t stride)
{
	fck_type_handle member_type = fck_type_registry_get(&types, fck_id(double));
	fck_member_registry_add(registry, (fck_member_desc){.type = member_type, .name = name, .owner = type, .stride = stride});
}

void fck_type_add_float(fck_member_registry_ref *registry, fck_type_handle type, const char *name, fckc_size_t stride)
{
	fck_type_handle member_type = fck_type_registry_get(&types, fck_id(float));
	fck_member_registry_add(registry, (fck_member_desc){.type = member_type, .name = name, .owner = type, .stride = stride});
}

void fck_type_add_float2(fck_member_registry_ref *registry, fck_type_handle type, const char *name, fckc_size_t stride)
{
	fck_type_handle member_type = fck_type_registry_get(&types, fck_id(float2));
	fck_member_registry_add(registry, (fck_member_desc){.type = member_type, .name = name, .owner = type, .stride = stride});
}

void fck_type_add_float3(fck_member_registry_ref *registry, fck_type_handle type, const char *name, fckc_size_t stride)
{
	fck_type_handle member_type = fck_type_registry_get(&types, fck_id(float3));
	fck_member_registry_add(registry, (fck_member_desc){.type = member_type, .name = name, .owner = type, .stride = stride});
}

void fck_serialise_float(fck_serialiser *serialiser, fck_serialiser_params *params, float *value, fckc_size_t count)
{
	serialiser->vt->f32(serialiser, params, value, count);
}
void fck_serialise_double(fck_serialiser *serialiser, fck_serialiser_params *params, double *value, fckc_size_t count)
{
	serialiser->vt->f64(serialiser, params, value, count);
}
void fck_serialise_i8(fck_serialiser *serialiser, fck_serialiser_params *params, fckc_i8 *value, fckc_size_t count)
{
	serialiser->vt->i8(serialiser, params, value, count);
}
void fck_serialise_i16(fck_serialiser *serialiser, fck_serialiser_params *params, fckc_i16 *value, fckc_size_t count)
{
	serialiser->vt->i16(serialiser, params, value, count);
}
void fck_serialise_i32(fck_serialiser *serialiser, fck_serialiser_params *params, fckc_i32 *value, fckc_size_t count)
{
	serialiser->vt->i32(serialiser, params, value, count);
}
void fck_serialise_i64(fck_serialiser *serialiser, fck_serialiser_params *params, fckc_i64 *value, fckc_size_t count)
{
	serialiser->vt->i64(serialiser, params, value, count);
}
void fck_serialise_u8(fck_serialiser *serialiser, fck_serialiser_params *params, fckc_u8 *value, fckc_size_t count)
{
	serialiser->vt->u8(serialiser, params, value, count);
}
void fck_serialise_u16(fck_serialiser *serialiser, fck_serialiser_params *params, fckc_u16 *value, fckc_size_t count)
{
	serialiser->vt->u16(serialiser, params, value, count);
}
void fck_serialise_u32(fck_serialiser *serialiser, fck_serialiser_params *params, fckc_u32 *value, fckc_size_t count)
{
	serialiser->vt->u32(serialiser, params, value, count);
}
void fck_serialise_u64(fck_serialiser *serialiser, fck_serialiser_params *params, fckc_u64 *value, fckc_size_t count)
{
	serialiser->vt->u64(serialiser, params, value, count);
}

#define fck_setup_base_primitive(types, serialisers, type, serialise)                                                                      \
	fck_serialiser_registry_add(serialisers, fck_type_registry_add(types, fck_id(type)), FCK_SERIALISE_FUNC(serialise))

void setup_base_primitives(fck_type_registry_ref *types, fck_serialiser_registry_ref *serialisers)
{
	fck_setup_base_primitive(types, serialisers, float, fck_serialise_float);
	fck_setup_base_primitive(types, serialisers, double, fck_serialise_double);
	fck_setup_base_primitive(types, serialisers, fckc_i8, fck_serialise_i8);
	fck_setup_base_primitive(types, serialisers, fckc_i16, fck_serialise_i16);
	fck_setup_base_primitive(types, serialisers, fckc_i32, fck_serialise_i32);
	fck_setup_base_primitive(types, serialisers, fckc_i64, fck_serialise_i64);
	fck_setup_base_primitive(types, serialisers, fckc_u8, fck_serialise_u8);
	fck_setup_base_primitive(types, serialisers, fckc_u16, fck_serialise_u16);
	fck_setup_base_primitive(types, serialisers, fckc_u32, fck_serialise_u32);
	fck_setup_base_primitive(types, serialisers, fckc_u64, fck_serialise_u64);
}

void setup_some_stuff()
{
	identifiers.value = fck_identifier_registry_alloc(128);

	members.value = fck_member_registry_alloc(64, &identifiers);
	types.value = fck_type_registry_alloc(64, &identifiers);
	serialisers.value = fck_serialiser_registry_alloc(64);

	setup_base_primitives(&types, &serialisers);

	fck_type_handle float_type_handle = fck_type_registry_get(&types, fck_id(float));
	fck_type_handle float2_type_handle = fck_type_registry_add(&types, fck_id(float2));
	fck_type_add_float(&members, float2_type_handle, "x", sizeof(float) * 0);
	fck_type_add_float(&members, float2_type_handle, "y", sizeof(float) * 1);

	fck_type_handle float3_type_handle = fck_type_registry_add(&types, fck_id(float3));
	fck_type_add_float(&members, float3_type_handle, "x", sizeof(float) * 0);
	fck_type_add_float(&members, float3_type_handle, "y", sizeof(float) * 1);
	fck_type_add_float(&members, float3_type_handle, "z", sizeof(float) * 2);

	fck_type_handle example_type_handle = fck_type_registry_add(&types, fck_id(example_type));
	fck_type_add_float(&members, example_type_handle, fck_id(cooldown), offsetof(example_type, cooldown));
	fck_type_add_float2(&members, example_type_handle, fck_id(position), offsetof(example_type, position));
	fck_type_add_float3(&members, example_type_handle, fck_id(rgb), offsetof(example_type, rgb));
	fck_type_add_double(&members, example_type_handle, fck_id(double_value), offsetof(example_type, double_value));
	fck_type_add_u32(&members, example_type_handle, fck_id(some_int), offsetof(example_type, some_int));
}

void fck_type_read(fck_type_handle type_handel, void *value)
{
}


void fck_type_edit(fck_serialiser *serialiser, fck_ui_ctx *ctx, fck_type_handle type_handle, const char *name, void *data)
{
	fck_type_info *type = fck_type_handle_resolve(type_handle);
	const char *owner_name_name = fck_identifier_resolve(type->identifier);

	fck_serialise_function serialise = fck_serialiser_registry_get(&serialisers, type_handle);
	if (serialise != NULL)
	{
		fck_serialiser_params params;
		params.name = name;
		params.user = NULL;

		fckc_size_t label_start = serialiser->at;
		serialise(serialiser, &params, data, 1);
	}

	fck_member_handle current = type->first_member;
	if (fck_member_handle_is_null(type->first_member))
	{
		return;
	}

	// Recurse through children
	char buffer[256];
	int count = SDL_snprintf(buffer, sizeof(buffer), "%s %s", owner_name_name, name);
	if (nk_tree_push_hashed(ctx, NK_TREE_NODE, buffer, NK_MINIMIZED, buffer, count, __LINE__))
	{
		while (!fck_member_handle_is_null(current))
		{
			fck_member_info *member = fck_member_handle_resolve(current);
			const char *member_name = fck_identifier_resolve(member->identifier);
			fckc_u8 *offset_ptr = ((fckc_u8 *)(data)) + member->stride;
			fck_type_edit(serialiser, ctx, member->type, member_name, (void *)(offset_ptr));
			current = member->next;
		}
		nk_tree_pop(ctx);
	}
}

void fck_type_serialise(fck_serialiser *serialiser, fck_type_handle type_handle, const char *name, void *data)
{
	fck_type_info *type = fck_type_handle_resolve(type_handle);
	const char *owner_name_name = fck_identifier_resolve(type->identifier);

	fck_serialise_function serialise = fck_serialiser_registry_get(&serialisers, type_handle);
	if (serialise != NULL)
	{
		fck_serialiser_params params;
		params.name = name;
		params.user = NULL;

		fckc_size_t label_start = serialiser->at;
		serialise(serialiser, &params, data, 1);
	}

	fck_member_handle current = type->first_member;
	if (fck_member_handle_is_null(type->first_member))
	{
		return;
	}

	// Recurse through children
	char buffer[256];
	int count = SDL_snprintf(buffer, sizeof(buffer), "%s %s", owner_name_name, name);

	while (!fck_member_handle_is_null(current))
	{
		fck_member_info *member = fck_member_handle_resolve(current);
		const char *member_name = fck_identifier_resolve(member->identifier);
		fckc_u8 *offset_ptr = ((fckc_u8 *)(data)) + member->stride;
		fck_type_serialise(serialiser, member->type, member_name, (void *)(offset_ptr));
		current = member->next;
	}
}

int fck_ui_window_entities(struct fck_ui *ui, fck_ui_window *window, void *userdata)
{
	fck_ui_ctx *ctx = fck_ui_context(ui);

	nk_layout_row_dynamic(ctx, 25, 1);

	fck_type_handle custom_type = fck_type_registry_get(&types, fck_id(example_type));

	fck_type_info *type = fck_type_handle_resolve(custom_type);

	// fck_serialiser serialiser = fck_serialiser_alloc(kll_heap, fck_byte_writer_vt, 256);
	// fck_serialiser serialiser = fck_serialiser_alloc(kll_heap, fck_byte_reader_vt, 256);
	// fck_serialiser serialiser = fck_serialiser_alloc(kll_heap, fck_string_writer_vt, 256);
	// fck_serialiser serialiser = fck_serialiser_alloc(kll_heap, fck_string_reader_vt, 256);
	fck_serialiser json;
	fck_serliaser_json_writer_alloc(&json, kll_heap);

	// This one below does not make sense - We need a json serialiser and a json READER
	// but the reader cannot be a serialiser since it does not... serialised LOL
	// fck_serialiser serialiser = fck_serialiser_alloc(kll_heap, fck_json_reader_vt, 256);
	fck_serialiser serialiser = fck_serialiser_alloc(kll_heap, fck_nk_edit_vt, 256);
	serialiser.user = ctx;

	static int is_init = 0;
	static example_type example;
	if (is_init == 0)
	{
		is_init = 1;
		example.cooldown = 69.0f;
		example.position.x = 1.0f;
		example.position.y = 2.0f;
		example.rgb = (float3){4.0f, 2.0, 0.0f};
		example.some_int = 99;
		example.double_value = 999.0;
	}
	static fckc_u8 opaque[64];
	fck_type_edit(&serialiser, ctx, custom_type, "dummy", &opaque);
	fck_type_serialise(&json, custom_type, "dummy", &example);

	const char *json_data = fck_serliaser_json_string_alloc(&json);

	fck_serialiser_free(&serialiser);
	// fck_member_handle current = type->first_member;
	// if (nk_tree_push(ctx, NK_TREE_TAB, owner_name_name, NK_MINIMIZED))
	//{
	//	while (!fck_member_handle_is_null(current))
	//	{
	//		fck_member_info *member = fck_member_handle_resolve(current);
	//		fck_type_info *member_type = fck_type_handle_resolve(member->type);
	//		const char *type_name = fck_identifier_resolve(member_type->identifier);
	//		const char *member_name = fck_identifier_resolve(member->identifier);

	//		nk_labelf(ctx, NK_TEXT_LEFT, "%s %s", type_name, member_name);
	//		current = member->next;
	//	}

	//	nk_tree_pop(ctx);
	//}

	const char *identifier = "float";

	// static fckc_u32 counter = 0;
	// if (nk_button_label(ctx, "Add float"))
	//{
	//	fck_type_handle type_handle = fck_type_registry_get(&types, identifier);
	//	fck_type_info *type_info = fck_type_handle_resolve(type_handle);
	//	const char *type_name = fck_identifier_resolve(type_info->identifier);

	//	char buffer[256];
	//	int count = SDL_snprintf(buffer, sizeof(buffer), "%s_%u", type_name, counter);

	//	fck_member_registry_add(&members, custom_type, type_handle, buffer);
	//	counter++;
	//}

	// if (nk_button_label(ctx, "Add int"))
	//{
	//	fck_type_handle type_handle = fck_type_registry_get(&types, "int");
	//	fck_type_info *type_info = fck_type_handle_resolve(type_handle);
	//	const char *type_name = fck_identifier_resolve(type_info->identifier);
	//	char buffer[256];
	//	SDL_snprintf(buffer, sizeof(buffer), "%s_%u", type_name, counter);

	//	fck_member_registry_add(&members, custom_type, type_handle, buffer);
	//	counter++;
	//}

	// if (nk_button_label(ctx, "Add double"))
	//{
	//	fck_type_handle type_handle = fck_type_registry_get(&types, "double");
	//	fck_type_info *type_info = fck_type_handle_resolve(type_handle);
	//	const char *type_name = fck_identifier_resolve(type_info->identifier);
	//	char buffer[256];
	//	SDL_snprintf(buffer, sizeof(buffer), "%s_%u", type_name, counter);

	//	fck_member_registry_add(&members, custom_type, type_handle, buffer);
	//	counter++;
	//}

	// fck_identifier_registry_free(identifiers);
	// fck_type_registry_free(type_registry);
	// fck_member_registry_free(member_registry);
	return 1;
}

fck_instance_result fck_instance_overlay(fck_instance *instance)
{
	int width;
	int height;
	if (!SDL_GetWindowSize(instance->window, &width, &height))
	{
		return FCK_INSTANCE_FAILURE;
	}

	fck_ui_window_manager *window_manager = instance->window_manager;
	fck_ui_window_manager_tick(instance->ui, window_manager, 0, 0, width, height);

	switch (fck_ui_window_manager_query_text_input_signal(instance->ui, instance->window_manager))
	{
	case FCK_UI_WINDOW_MANAGER_TEXT_INPUT_SIGNAL_START:
		SDL_StartTextInput(instance->window);
		break;
	case FCK_UI_WINDOW_MANAGER_TEXT_INPUT_SIGNAL_STOP:
		SDL_StopTextInput(instance->window);
		break;
	default:
		// Shut up compiler
		break;
	}
	return FCK_INSTANCE_CONTINUE;
}

fck_instance *fck_instance_alloc(const char *title, int with, int height, SDL_WindowFlags window_flags, const char *renderer_name)
{
	fck_instance *app = (fck_instance *)SDL_malloc(sizeof(fck_instance));
	app->window = SDL_CreateWindow(title, 1920, 1080, SDL_WINDOW_RESIZABLE);
	app->renderer = SDL_CreateRenderer(app->window, renderer_name);
	app->ui = fck_ui_alloc(app->renderer);
	app->window_manager = fck_ui_window_manager_alloc(16);

	fck_ui_window_manager_create(app->window_manager, "Entities", NULL, fck_ui_window_entities);
	fck_ui_window_manager_create(app->window_manager, "Nk Overview", NULL, fck_ui_window_overview);
	fck_ui_set_style(fck_ui_context(app->ui), THEME_DRACULA);

	setup_some_stuff();

	return app;
}

void fck_instance_free(fck_instance *instance)
{
	fck_ui_window_manager_free(instance->window_manager);

	fck_ui_free(instance->ui);
	SDL_DestroyRenderer(instance->renderer);
	SDL_DestroyWindow(instance->window);
	SDL_free(instance);
}

fck_instance_result fck_instance_event(fck_instance *instance, SDL_Event const *event)
{
	fck_ui_enqueue_event(instance->ui, event);
	return FCK_INSTANCE_CONTINUE;
}

fck_instance_result fck_instance_tick(fck_instance *instance)
{
	fck_instance_overlay(instance);

	SDL_SetRenderDrawColor(instance->renderer, 0, 0, 0, 0);
	SDL_RenderClear(instance->renderer);

	fck_ui_render(instance->ui, instance->renderer);

	SDL_RenderPresent(instance->renderer);

	return FCK_INSTANCE_CONTINUE;
}
