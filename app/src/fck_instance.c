#include "fck_instance.h"

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_render.h>

#include <fck_hash.h>

#include "fck_ui.h"

#include "fck_nuklear_demos.h"
#include "fck_ui_window_manager.h"

#include "fck_serialiser.h"
#include "fck_serialiser_json_vt.h"
#include "fck_serialiser_nk_edit_vt.h"
#include "fck_serialiser_vt.h"

#include "kll_heap.h"

#include "fck_type_system.h"

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

typedef void (*fck_serialise_function)(fck_serialiser *, fck_serialiser_params *, void *, fckc_size_t);

#define FCK_SERIALISE_FUNC(x) (fck_serialise_function)(x)

typedef struct fck_serialiser_info
{
	fck_type type;
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

fck_serialiser_registry *fck_serialiser_registry_alloc(fckc_size_t capacity)
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

void fck_serialiser_registry_free(fck_serialiser_registry *registry)
{
	SDL_free(registry);
}

void fck_serialiser_registry_add(fck_serialiser_registry_ref *registry, fck_type type, fck_serialise_function serialise_func)
{
	SDL_assert(registry);
	SDL_assert(!fck_type_is_null(type));

	fckc_size_t index = ((fckc_size_t)type.hash) % registry->value->capacity;
	while (1)
	{
		const fck_serialiser_info *current = &registry->value->info[index];
		if (fck_type_is_null(current->type))
		{
			break;
		}
		SDL_assert(!fck_type_is_same(type, current->type) && "Already added, do not add twice?!");
		index = (index + 1) % registry->value->capacity;
	}

	fck_serialiser_info *info = registry->value->info + index;
	info->type = type;
	info->serialise = serialise_func;
}

fck_serialise_function fck_serialiser_registry_get(fck_serialiser_registry_ref *registry, fck_type type)
{
	SDL_assert(registry);
	SDL_assert(!fck_type_is_null(type));

	fckc_size_t index = ((fckc_size_t)type.hash) % registry->value->capacity;
	while (1)
	{
		const fck_serialiser_info *current = &registry->value->info[index];
		if (fck_type_is_null(current->type))
		{
			return NULL;
		}
		if (fck_type_is_same(type, current->type))
		{
			return current->serialise;
		}
		index = (index + 1) % registry->value->capacity;
	}
}

typedef struct fck_component_info
{
	struct fck_type_info *type;
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

struct fck_identifiers *identifiers;
struct fck_members *members;
struct fck_types *types;
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

void fck_type_add_u32(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type member_type = fck_types_find_from_string(type.types, fck_id(fckc_u32));
	fck_members_add(members, (fck_member_desc){.type = member_type, .name = name, .owner = type, .stride = stride});
}

void fck_type_add_double(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type member_type = fck_types_find_from_string(type.types, fck_id(double));
	fck_members_add(members, (fck_member_desc){.type = member_type, .name = name, .owner = type, .stride = stride});
}

void fck_type_add_float(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type member_type = fck_types_find_from_string(type.types, fck_id(float));
	fck_members_add(members, (fck_member_desc){.type = member_type, .name = name, .owner = type, .stride = stride});
}

void fck_type_add_float2(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type member_type = fck_types_find_from_string(type.types, fck_id(float2));
	fck_members_add(members, (fck_member_desc){.type = member_type, .name = name, .owner = type, .stride = stride});
}

void fck_type_add_float3(struct fck_members *members, fck_type type, const char *name, fckc_size_t stride)
{
	fck_type member_type = fck_types_find_from_string(type.types, fck_id(float3));
	fck_members_add(members, (fck_member_desc){.type = member_type, .name = name, .owner = type, .stride = stride});
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
	fck_serialiser_registry_add(serialisers, fck_types_add(types, (fck_type_desc){fck_id(type)}), FCK_SERIALISE_FUNC(serialise))

void setup_base_primitives(struct fck_types *types, fck_serialiser_registry_ref *serialisers)
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
	identifiers = fck_identifiers_alloc(128);

	members = fck_members_alloc(identifiers, 64);
	types = fck_types_alloc(identifiers, 64);
	serialisers.value = fck_serialiser_registry_alloc(64);

	setup_base_primitives(types, &serialisers);

	fck_type float_type_handle = fck_types_add(types, (fck_type_desc){fck_id(float)});
	fck_type float2_type_handle = fck_types_add(types, (fck_type_desc){fck_id(float2)});
	fck_type_add_float(members, float2_type_handle, "x", sizeof(float) * 0);
	fck_type_add_float(members, float2_type_handle, "y", sizeof(float) * 1);

	fck_type float3_type_handle = fck_types_add(types, (fck_type_desc){fck_id(float3)});
	fck_type_add_float(members, float3_type_handle, "x", sizeof(float) * 0);
	fck_type_add_float(members, float3_type_handle, "y", sizeof(float) * 1);
	fck_type_add_float(members, float3_type_handle, "z", sizeof(float) * 2);

	fck_type example_type_handle = fck_types_add(types, (fck_type_desc){fck_id(example_type)});
	fck_type_add_float(members, example_type_handle, fck_id(cooldown), offsetof(example_type, cooldown));
	fck_type_add_float2(members, example_type_handle, fck_id(position), offsetof(example_type, position));
	fck_type_add_float3(members, example_type_handle, fck_id(rgb), offsetof(example_type, rgb));
	fck_type_add_double(members, example_type_handle, fck_id(double_value), offsetof(example_type, double_value));
	fck_type_add_u32(members, example_type_handle, fck_id(some_int), offsetof(example_type, some_int));
}

void fck_type_read(fck_type type_handel, void *value)
{
}

void fck_type_edit(fck_serialiser *serialiser, fck_ui_ctx *ctx, fck_type type_handle, const char *name, void *data)
{
	struct fck_type_info *type = fck_type_resolve(type_handle);
	fck_identifier owner_identifier = fck_type_info_identify(type);

	const char *owner_name_name = fck_identifier_resolve(owner_identifier);

	fck_serialise_function serialise = fck_serialiser_registry_get(&serialisers, type_handle);
	if (serialise != NULL)
	{
		fck_serialiser_params params;
		params.name = name;
		params.user = NULL;

		fckc_size_t label_start = serialiser->at;
		serialise(serialiser, &params, data, 1);
	}

	fck_member current = fck_type_info_first_member(type);
	if (fck_member_is_null(current))
	{
		return;
	}

	// Recurse through children
	char buffer[256];
	int count = SDL_snprintf(buffer, sizeof(buffer), "%s %s", owner_name_name, name);
	if (nk_tree_push_hashed(ctx, NK_TREE_NODE, buffer, NK_MINIMIZED, buffer, count, __LINE__))
	{
		while (!fck_member_is_null(current))
		{
			struct fck_member_info *member = fck_member_resolve(current);
			fck_identifier member_identifier = fck_member_info_identify(member);
			const char *member_name = fck_identifier_resolve(member_identifier);
			fckc_u8 *offset_ptr = ((fckc_u8 *)(data)) + fck_member_info_stride(member);
			fck_type member_type = fck_member_info_type(member);
			fck_type_edit(serialiser, ctx, member_type, member_name, (void *)(offset_ptr));

			current = fck_member_info_next(member);
		}
		nk_tree_pop(ctx);
	}
}

void fck_type_serialise(fck_serialiser *serialiser, fck_type type_handle, const char *name, void *data)
{
	struct fck_type_info *type = fck_type_resolve(type_handle);
	fck_identifier owner_identifier = fck_type_info_identify(type);
	const char *owner_name_name = fck_identifier_resolve(owner_identifier);

	fck_serialise_function serialise = fck_serialiser_registry_get(&serialisers, type_handle);
	if (serialise != NULL)
	{
		fck_serialiser_params params;
		params.name = name;
		params.user = NULL;

		fckc_size_t label_start = serialiser->at;
		serialise(serialiser, &params, data, 1);
	}

	fck_member current = fck_type_info_first_member(type);
	while (!fck_member_is_null(current))
	{
		struct fck_member_info *member = fck_member_resolve(current);
		fck_identifier member_identifier = fck_member_info_identify(member);
		const char *member_name = fck_identifier_resolve(member_identifier);
		fckc_u8 *offset_ptr = ((fckc_u8 *)(data)) + fck_member_info_stride(member);
		fck_type member_type = fck_member_info_type(member);
		fck_type_serialise(serialiser, member_type, member_name, (void *)(offset_ptr));

		current = fck_member_info_next(member);
	}
}

int fck_ui_window_entities(struct fck_ui *ui, fck_ui_window *window, void *userdata)
{
	fck_ui_ctx *ctx = fck_ui_context(ui);

	nk_layout_row_dynamic(ctx, 25, 1);

	fck_type custom_type = fck_types_find_from_string(types, fck_id(example_type));
	struct fck_type_info *type = fck_type_resolve(custom_type);

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
