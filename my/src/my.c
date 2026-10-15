#include "my.h"

#include <fck_apis.h>
#include <fckc_apidef.h>

#include <kll.h>
#include <kll_malloc.h>

#include <fck_hash.h>
#include <fck_set.h>

typedef struct my_assembly
{
	struct kll_arena  *arena;
	const char        *name;
	/* set */ my_type *types;
} my_assembly;

static my_primitives my_api_primitives;

static my_assembly *my_api_create_assembly(kll_allocator *allocator, const char *name)
{
	my_assembly      *assembly;
	const fckc_size_t types_capacity = 128; // TODO

	const fckc_size_t types_total  = types_capacity * sizeof(*assembly->types);
	const fckc_size_t fields_total = types_capacity * sizeof(*assembly->types->fields);
	const fckc_size_t total        = sizeof(assembly) + types_total + fields_total;

	kll_arena *arena = kll->arena->create(allocator, total);
	assembly         = (my_assembly *)kll_malloc(arena, sizeof(*assembly));
	memset(assembly, 0, sizeof(assembly));

	assembly->arena = arena;
	assembly->types = fck_set_new(my_type, arena, types_capacity);
	assembly->name  = arena->format(arena, "%s", name);

	return assembly;
}

static void my_api_destroy_assembly(my_assembly *assembly)
{
	kll->arena->destroy(assembly->arena);
}

static my_type *my_api_create_type(const my_assembly *assembly, fckc_size_t size, const char *name)
{
	const fckc_u64 hash = fck_hash(name, strlen(name));
	my_type       *type = &fck_set_at(assembly->types, hash);
	memset(type, 0, sizeof(*type));

	type->assembly = assembly;
	type->name     = assembly->arena->format(assembly->arena, "%s", name);
	type->size     = size;
	type->hash     = hash;
	return type;
}

static my_field *my_api_add_field(my_type *type, fckc_size_t offset, const my_type *field_type, const char *name)
{
	kll_arena *arena = type->assembly->arena;
	my_field  *field = (my_field *)kll_malloc(arena, sizeof(*type->fields));

	field->name   = arena->format(arena, "%s", name);
	field->type   = field_type;
	field->offset = offset;
	field->next   = NULL;

	if (type->fields == NULL)
	{
		fck_assert(type->last == NULL);
		type->fields = field;
		type->last   = field;
	}
	else
	{
		my_field *last = (my_field *)type->last;

		last->next = field;
		type->last = field;
	}
	return field;
}

static const my_type *my_api_typeof(const my_assembly *assembly, const char *name);

static my_api fck_my_api = {
	.primitives  = &my_api_primitives,
	.create_type = my_api_create_type,
	.typeof      = my_api_typeof,
	.add_field   = my_api_add_field,
};

const my_type *my_api_typeof(const my_assembly *assembly, const char *name)
{
	const fckc_u64    hash   = fck_hash(name, strlen(name));
	const fckc_size_t result = fck_set_find(assembly->types, hash);

	// Maybe add some extra checks! :)

	if (result)
	{
		return assembly->types + result - 1;
	}

	// Fallback to system assembly - Override may exist in selected assembly
	return fck_my_api.typeof(fck_my_api.system, name);
}

#define my_create_native(my, assembly, type) my->create_type((assembly), sizeof(type), fck_nameof(type))

FCK_EXPORT_API my_api *fck_my_load(fck_api_registry *registry, void *old)
{
	my_primitives *primitives = &my_api_primitives;

	my_api *my = &fck_my_api;

	my_assembly *system = my_api_create_assembly(kll->system, "system");

	my->system = system;

	primitives->i8  = my_create_native(my, system, fckc_i8);
	primitives->i16 = my_create_native(my, system, fckc_i16);
	primitives->i32 = my_create_native(my, system, fckc_i32);
	primitives->i64 = my_create_native(my, system, fckc_i64);
	primitives->u8  = my_create_native(my, system, fckc_u8);
	primitives->u16 = my_create_native(my, system, fckc_u16);
	primitives->u32 = my_create_native(my, system, fckc_u32);
	primitives->u64 = my_create_native(my, system, fckc_u64);
	primitives->f32 = my_create_native(my, system, fckc_f32);
	primitives->f64 = my_create_native(my, system, fckc_f64);

	registry->add(fck_my_api_name, &fck_my_api);
	return &fck_my_api;
}
