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

static fck_entity_set *entities;

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
struct fck_serialise_interfaces *serialisers;

typedef struct example_type
{
	fckc_f32x2 other;
	double double_value;
	float cooldown;
	fckc_f32x2 position;
	fckc_f32x3 rgb;
	fckc_u32 some_int;
} example_type;

void setup_some_stuff()
{
	identifiers = fck_identifiers_alloc(1);

	members = fck_members_alloc(identifiers, 1);
	types = fck_types_alloc(identifiers, 1);
	serialisers = fck_serialise_interfaces_alloc(1);

	fck_type_system_setup_primitives(types, members, serialisers);

	fck_type example_type_handle = fck_types_add(types, (fck_type_desc){fck_name(example_type)});
	fck_type_add_f32x2(members, example_type_handle, fck_name(other), offsetof(example_type, other));
	fck_type_add_f32(members, example_type_handle, fck_name(cooldown), offsetof(example_type, cooldown));
	fck_type_add_f32x2(members, example_type_handle, fck_name(position), offsetof(example_type, position));
	fck_type_add_f32x3(members, example_type_handle, fck_name(rgb), offsetof(example_type, rgb));
	fck_type_add_f64(members, example_type_handle, fck_name(double_value), offsetof(example_type, double_value));
	fck_type_add_u32(members, example_type_handle, fck_name(some_int), offsetof(example_type, some_int));
}

void fck_type_read(fck_type type_handel, void *value)
{
}

void fck_type_edit(fck_serialiser *serialiser, fck_ui_ctx *ctx, fck_type type, const char *name, void *data)
{
	struct fck_type_info *info = fck_type_resolve(type);
	fck_identifier owner_identifier = fck_type_info_identify(info);

	const char *owner_name_name = fck_identifier_resolve(owner_identifier);

	fck_serialise_func *serialise = fck_serialise_interfaces_get(serialisers, type);
	if (serialise != NULL)
	{
		fck_serialiser_params params;
		params.name = name;
		params.user = NULL;

		fckc_size_t label_start = serialiser->at;
		serialise(data, 1, serialiser, &params);
	}

	fck_member current = fck_type_info_first_member(info);
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

void fck_type_serialise(fck_serialiser *serialiser, fck_type type, const char *name, void *data)
{
	struct fck_type_info *type_info = fck_type_resolve(type);
	fck_identifier owner_identifier = fck_type_info_identify(type_info);
	const char *owner_name_name = fck_identifier_resolve(owner_identifier);

	fck_serialise_func *serialise = fck_serialise_interfaces_get(serialisers, type);
	if (serialise != NULL)
	{
		fck_serialiser_params params;
		params.name = name;
		params.user = NULL;

		fckc_size_t label_start = serialiser->at;
		serialise(data, 1, serialiser, &params);
	}

	fck_member current = fck_type_info_first_member(type_info);
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

	fck_type custom_type = fck_types_find_from_string(types, fck_name(example_type));
	struct fck_type_info *type = fck_type_resolve(custom_type);

	// fck_serialiser serialiser = fck_serialiser_alloc(kll_heap, fck_byte_writer_vt, 256);
	// fck_serialiser serialiser = fck_serialiser_alloc(kll_heap, fck_byte_reader_vt, 256);
	// fck_serialiser serialiser = fck_serialiser_alloc(kll_heap, fck_string_writer_vt, 256);
	// fck_serialiser serialiser = fck_serialiser_alloc(kll_heap, fck_string_reader_vt, 256);

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
		example.other.x = 99.0f;
		example.other.y = 123.0f;
		example.cooldown = 69.0f;
		example.position.x = 1.0f;
		example.position.y = 2.0f;
		example.rgb = (fckc_f32x3){4.0f, 2.0, 0.0f};
		example.some_int = 99;
		example.double_value = 999.0;
	}
	static fckc_u8 opaque[64];
	fck_type_edit(&serialiser, ctx, custom_type, "dummy", &example);
	// fck_type_serialise(&json, custom_type, "dummy", &example);

	// fck_serialise_func *serialise = fck_serialise_interfaces_get(serialisers, fck_types_find_from_string(types, fck_name(float)));
	// fck_serialiser_params params;
	// params.name = "SOME TEST";
	// params.user = NULL;

	// fckc_size_t label_start = serialiser.at;
	// serialise(&example, 2, &serialiser, &params);

	if (nk_button_label(ctx, "Save to disk"))
	{
		fck_serialiser json;
		fck_serialiser_json_writer_alloc(&json, kll_heap);
		fck_type_serialise(&json, custom_type, "Template", &example);

		char *json_data = fck_serialiser_json_string_alloc(&json);
		fck_serialiser_json_string_free(&json, json_data);

		fck_serialiser_free(&serialiser);
	}

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
	app->window = SDL_CreateWindow(title, 1280, 720, SDL_WINDOW_RESIZABLE);
	app->renderer = SDL_CreateRenderer(app->window, renderer_name);
	app->ui = fck_ui_alloc(app->renderer);
	app->window_manager = fck_ui_window_manager_alloc(16);

	fck_ui_window_manager_create(app->window_manager, "Entities", NULL, fck_ui_window_entities);
	fck_ui_window_manager_create(app->window_manager, "Nk Overview", NULL, fck_ui_window_overview);
	fck_ui_set_style(fck_ui_context(app->ui), THEME_DRACULA);

	setup_some_stuff();

	entities = SDL_malloc(offsetof(fck_entity_set, id[128]));
	entities->capacity = 128;
	entities->count = 2;

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
