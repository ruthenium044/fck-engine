#include "fck_instance.h"

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_render.h>

#include <fck_hash.h>

#include "fck_ui.h"

#include "fck_nuklear_demos.h"
#include "fck_ui_window_manager.h"

#include "fck_serialiser_nk_edit_vt.h"
#include "fck_serialiser_vt.h"

#include "kll_heap.h"
#include <kll.h>
#include <kll_malloc.h>

#include "fck_apis.h"
#include "fck_type_system.h"

#include "fck_stretchy.h"

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

typedef struct some_type
{
	fckc_f32 x, y;
} some_type;

typedef struct example_type
{
	fckc_f32x2 other;
	fckc_f32 cooldown;
	fckc_f32x2 position;
	fckc_f32x3 rgb;
	fckc_f64 double_value;

	fckc_i64 i64;
	fckc_u64 u64;

	fckc_u32 u32;
	fckc_i32 i32;
	fckc_i16 i16;
	fckc_u16 u16;
	fckc_i8 i8;
	fckc_u8 u8;

	some_type *stretchy;

	some_type arr[5];

	fckc_i32 some_int;
} example_type;

typedef struct fck_type_memory
{
	kll_allocator *alloctor;
	fckc_size_t size;
	fckc_u8 *buffer;
} fck_type_memory;

fck_type_memory fck_type_memory_create(kll_allocator *alloctor)
{
	return (fck_type_memory){alloctor, 0, NULL};
}

void fck_type_memory_zero(fck_type_memory *memory)
{
	SDL_memset(memory->buffer, 0, memory->size);
}

void fck_type_memory_alloc(fck_type_memory *memory, fckc_size_t size)
{
	memory->buffer = (fckc_u8 *)kll_malloc(memory->alloctor, size);
	memory->size = size;
	fck_type_memory_zero(memory);
}

void fck_type_memory_free(fck_type_memory *memory)
{
	kll_free(memory->alloctor, memory->buffer);
	memory->size = 0;
	memory->buffer = NULL;
}

fckc_u8 *fck_type_memory_squeeze(fck_type_memory *memory, fckc_size_t stride, fckc_size_t size)
{
	fck_type_memory result = fck_type_memory_create(memory->alloctor);
	fck_type_memory_alloc(&result, memory->size + size);

	SDL_memcpy(result.buffer, memory->buffer, memory->size);
	fckc_u8 *src = memory->buffer + stride;
	fckc_u8 *dst = memory->buffer + size;
	SDL_memmove(dst, src, size);

	fck_type_memory_free(memory);
	*memory = result;

	// Return start of added memory for convenience
	return memory->buffer + stride;
}

void setup_some_stuff(fck_instance *app)
{
	// fck_type_memory mem = fck_type_memory_create(kll_heap);
	// fck_type_memory_alloc(&mem, sizeof(example_type));

	fck_type_system *ts = fck_get_type_system(app->apis);

	// fck_type example_type_handle = fck_types_add(ts->get_types(), (fck_type_desc){fck_name(example_type)});
	fck_type example_type_handle = ts->type->add((fck_type_desc){fck_name(example_type)});
	fck_type some_type_handle = ts->type->add((fck_type_desc){fck_name(some_type)});

	fck_type f32 = ts->type->find(fck_id(fckc_f32));
	fck_type f64 = ts->type->find(fck_id(fckc_f64));
	fck_type i8 = ts->type->find(fck_id(fckc_i8));
	fck_type i16 = ts->type->find(fck_id(fckc_i16));
	fck_type i32 = ts->type->find(fck_id(fckc_i32));
	fck_type i64 = ts->type->find(fck_id(fckc_i64));
	fck_type u8 = ts->type->find(fck_id(fckc_u8));
	fck_type u16 = ts->type->find(fck_id(fckc_u16));
	fck_type u32 = ts->type->find(fck_id(fckc_u32));
	fck_type u64 = ts->type->find(fck_id(fckc_u64));

	ts->member->add(some_type_handle, fck_value_decl(some_type, f32, x));
	ts->member->add(some_type_handle, fck_value_decl(some_type, f32, y));

	ts->member->add(example_type_handle, fck_stretchy_decl(example_type, some_type_handle, stretchy));
	ts->member->add(example_type_handle, fck_array_decl(example_type, f32, other, 2));
	ts->member->add(example_type_handle, fck_value_decl(example_type, f32, cooldown));
	ts->member->add(example_type_handle, fck_array_decl(example_type, f32, position, 2));
	ts->member->add(example_type_handle, fck_array_decl(example_type, f32, rgb, 3));
	ts->member->add(example_type_handle, fck_value_decl(example_type, f64, double_value));
	ts->member->add(example_type_handle, fck_value_decl(example_type, i32, some_int));
	ts->member->add(example_type_handle, fck_value_decl(example_type, i8, i8));
	ts->member->add(example_type_handle, fck_value_decl(example_type, i16, i16));
	ts->member->add(example_type_handle, fck_value_decl(example_type, i32, i32));
	ts->member->add(example_type_handle, fck_value_decl(example_type, i64, i64));
	ts->member->add(example_type_handle, fck_value_decl(example_type, u8, u8));
	ts->member->add(example_type_handle, fck_value_decl(example_type, u16, u16));
	ts->member->add(example_type_handle, fck_value_decl(example_type, u32, u32));
	ts->member->add(example_type_handle, fck_value_decl(example_type, u64, u64));

	ts->member->add(example_type_handle, fck_array_decl(example_type, some_type_handle, arr, 5));

	fckc_size_t size = ts->type->size_of(example_type_handle);
	SDL_Log("%llu", size);
}

int fck_ui_window_entities(struct fck_ui *ui, fck_ui_window *window, void *userdata)
{
	fck_instance *app = (fck_instance *)userdata;
	fck_ui_ctx *ctx = fck_ui_context(ui);

	nk_layout_row_dynamic(ctx, 25, 1);

	fck_type_system *ts = fck_get_type_system(app->apis);

	fck_type custom_type = ts->type->find(fck_name(example_type));
	struct fck_type_info *type = ts->type->resolve(custom_type);

	fck_nk_serialiser serialiser = {.ctx = ctx, .vt = fck_nk_edit_vt};

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
		example.stretchy = fck_stretchy_new(some_type, kll_heap, 8);
	}

	static fckc_u8 opaque[1024];
	static fckc_size_t offset = sizeof(example);

	fck_serialiser_params params;
	params.name = "dummy";
	params.type_system = ts;
	params.type = &custom_type;

	fck_type_serialise((fck_serialiser *)&serialiser, &params, opaque, 1);

	nk_layout_row_begin(ctx, NK_DYNAMIC, 25, (int)1);

	nk_layout_row_push(ctx, 1.0f);
	nk_label(ctx, "", NK_TEXT_ALIGN_LEFT);

	nk_layout_row_push(ctx, 1.0f);
	if (nk_button_label(ctx, "Save to disk"))
	{
		// fck_serialiser json;
		// fck_serialiser_json_writer_alloc(&json, kll_heap);
		// fck_type_serialise(&json, custom_type, "Template", &example);

		// char *json_data = fck_serialiser_json_string_alloc(&json);
		// fck_serialiser_json_string_free(&json, json_data);

		// fck_serialiser_free(&serialiser);
	}

	static char input_text_buffer[512];
	static int input_text_len = 0;
	nk_layout_row_push(ctx, 1.0f);
	nk_edit_string(ctx, NK_EDIT_SIMPLE, input_text_buffer, &input_text_len, sizeof(input_text_buffer), nk_filter_default);

	nk_layout_row_push(ctx, 1.0f);
	nk_label(ctx, "", NK_TEXT_ALIGN_LEFT);

	fck_type current = ts->type->null();
	while (ts->type->iterate(&current))
	{
		struct fck_type_info *info = ts->type->resolve(current);
		fck_identifier identifier = ts->type->identify(info);
		const char *name = ts->identifier->resolve(identifier);
		nk_layout_row_push(ctx, 1.0f);
		if (nk_button_label(ctx, name))
		{
			fck_member_desc desc;
			desc.extra_count = 0;
			desc.stride = offset;
			desc.type = current;

			desc.name = input_text_buffer;

			ts->member->add(custom_type, desc);
			offset = offset + 8;
		}
	}
	nk_layout_row_end(ctx);

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

	app->apis = fck_apis_load();
	fck_load_type_system(app->apis);

	fck_ui_window_manager_create(app->window_manager, "Entities", app, fck_ui_window_entities);
	fck_ui_window_manager_create(app->window_manager, "Nk Overview", NULL, fck_ui_window_overview);
	fck_ui_set_style(fck_ui_context(app->ui), THEME_DRACULA);

	setup_some_stuff(app);

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
