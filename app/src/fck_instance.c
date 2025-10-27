#include "fck_instance.h"

#include <SDL3/SDL_events.h>

#include <fck_hash.h>

#include "fck_ui.h"

#include "fck_nuklear_demos.h"
#include "fck_ui_window_manager.h"

#include "fck_serialiser_nk_edit_vt.h"
#include "fck_serialiser_vt.h"

#include <kll.h>
#include <kll_heap.h>
#include <kll_malloc.h>

#include "fck_type_system.h"
#include <fck_apis.h>

#include <fck_os.h>
#include <fck_set.h>

#include <fck_canvas.h>

// This struct is good enough for now
typedef struct fck_instance
{
	fck_ui *ui; // User
	// struct SDL_Window *window;     // This one could stay public - Makes sense for multi-instance stuff
	// struct SDL_Renderer *renderer; // User
	fck_window wind;
	fck_canvas canvas;
	fck_ui_window_manager *window_manager;
	struct fck_assembly *assembly;
} fck_instance;

fck_api_registry *api;
fck_type_system *ts;
fck_canvas_api *canvas;

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

	some_type *dynarr;

	some_type arr[5];

	fckc_i32 some_int;
} example_type;

void setup_some_stuff(fck_instance *app)
{
	// fck_type_memory mem = fck_type_memory_create(kll_heap);
	// fck_type_memory_alloc(&mem, sizeof(example_type));

	// fck_type example_type_handle = fck_types_add(ts->get_types(), (fck_type_desc){fck_name(example_type)});
	fck_type example_type_handle = ts->type->add(app->assembly, (fck_type_desc){fck_name(example_type)});
	fck_type some_type_handle = ts->type->add(app->assembly, (fck_type_desc){fck_name(some_type)});

	fck_type f32 = ts->type->find(app->assembly, fck_id(fckc_f32));
	fck_type f64 = ts->type->find(app->assembly, fck_id(fckc_f64));
	fck_type i8 = ts->type->find(app->assembly, fck_id(fckc_i8));
	fck_type i16 = ts->type->find(app->assembly, fck_id(fckc_i16));
	fck_type i32 = ts->type->find(app->assembly, fck_id(fckc_i32));
	fck_type i64 = ts->type->find(app->assembly, fck_id(fckc_i64));
	fck_type u8 = ts->type->find(app->assembly, fck_id(fckc_u8));
	fck_type u16 = ts->type->find(app->assembly, fck_id(fckc_u16));
	fck_type u32 = ts->type->find(app->assembly, fck_id(fckc_u32));
	fck_type u64 = ts->type->find(app->assembly, fck_id(fckc_u64));

	ts->member->add(some_type_handle, fck_value_decl(some_type, f32, x));
	ts->member->add(some_type_handle, fck_value_decl(some_type, f32, y));

	ts->member->add(example_type_handle, fck_dynarr_decl(example_type, some_type_handle, dynarr));
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
}

int fck_ui_window_entities(struct fck_ui *ui, fck_ui_window *window, void *userdata)
{
	fck_instance *app = (fck_instance *)userdata;
	fck_ui_ctx *ctx = fck_ui_context(ui);

	nk_layout_row_dynamic(ctx, 25, 1);

	fck_type custom_type = ts->type->find(app->assembly, fck_name(example_type));
	struct fck_type_info *type = ts->type->resolve(custom_type);

	// fck_json_serialiser json;
	// fck_serialiser_json_writer_alloc(&json, kll_heap);

	static fckc_u8 opaque[1024];

	fck_marshal_params params;
	params.name = "dummy";
	params.type = &custom_type;

	// fck_type_serialise((fck_serialiser *)&serialiser, &params, opaque, 1);
	fck_type assembly_type = ts->type->find(app->assembly, fck_name(fck_assembly));
	fck_marshal_params json_params;
	json_params.name = "Assembly";
	json_params.type = &assembly_type;
	// json_params.parent = NULL;

	fck_nk_serialiser serialiser = {.ctx = ctx, .vt = fck_nk_read_vt};

	fck_marshaller marshaller;
	marshaller.serialiser = (fck_serialiser *)&serialiser;
	marshaller.type_system = ts;
	ts->marshal->invoke((fck_serialiser *)&serialiser, &assembly_type, "Assembly", app->assembly, 1);
	// fck_type_serialise(&marshaller, &json_params, app->assembly, 1);

	//    char *text = fck_serialiser_json_string_alloc(&json);
	//    SDL_Log("%s", text);

	nk_layout_row_begin(ctx, NK_DYNAMIC, 25, (int)1);

	nk_layout_row_push(ctx, 1.0f);
	nk_label(ctx, "", NK_TEXT_ALIGN_LEFT);

	struct fck_type_info *info = ts->type->resolve(assembly_type);
	fck_member mem = ts->type->members_of(info);
	while (!ts->member->is_null(mem))
	{
		struct fck_member_info *mem_info = ts->member->resolve(mem);

		nk_layout_row_push(ctx, 1.0f);
		const char *text = ts->identifier->resolve(ts->member->identify(mem_info));
		nk_button_label(ctx, text);
		mem = ts->member->next_of(mem_info);
	}
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

	return 1;
	static char input_text_buffer[512];
	static int input_text_len = 0;
	nk_layout_row_push(ctx, 1.0f);
	nk_edit_string(ctx, NK_EDIT_SIMPLE, input_text_buffer, &input_text_len, sizeof(input_text_buffer), nk_filter_default);

	nk_layout_row_push(ctx, 1.0f);
	nk_label(ctx, "", NK_TEXT_ALIGN_LEFT);

	fck_type current = ts->type->null();
	while (ts->type->iterate(app->assembly, &current))
	{
		struct fck_type_info *info = ts->type->resolve(current);
		fck_identifier identifier = ts->type->identify(info);
		const char *name = ts->identifier->resolve(identifier);
		nk_layout_row_push(ctx, 1.0f);
		if (nk_button_label(ctx, name))
		{
			fckc_size_t offset = sizeof(example_type);

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
	if (!os->win->size_get(instance->wind, &width, &height))
	{
		return FCK_INSTANCE_FAILURE;
	}

	fck_ui_window_manager *window_manager = instance->window_manager;
	fck_ui_window_manager_tick(instance->ui, window_manager, 0, 0, width, height);

	switch (fck_ui_window_manager_query_text_input_signal(instance->ui, instance->window_manager))
	{
	case FCK_UI_WINDOW_MANAGER_TEXT_INPUT_SIGNAL_START:
		os->win->text_input_start(instance->wind);
		break;
	case FCK_UI_WINDOW_MANAGER_TEXT_INPUT_SIGNAL_STOP:
		os->win->text_input_stop(instance->wind);
		break;
	default:
		// Shut up compiler
		break;
	}
	return FCK_INSTANCE_CONTINUE;
}

fck_instance *fck_instance_alloc(int argc, char *argv[])
{
	fck_apis_manifest manifest[] = {
		{.api = (void **)&ts, .name = "fck-ts.dylib", NULL},
		{.api = (void **)&canvas, .name = "fck-canvas.dylib", NULL},
	};

	fck_apis_init init = (fck_apis_init){
		.manifest = manifest,
		.count = fck_arraysize(manifest),
	};
	fck_shared_object api_so = os->so->load("fck-api.dylib");
	fck_main_func *main_so = (fck_main_func *)os->so->symbol(api_so, FCK_ENTRY_POINT);
	api = (fck_api_registry *)main_so(api, &init);

	fck_instance *app = (fck_instance *)SDL_malloc(sizeof(fck_instance));

	app->wind = os->win->create("Widnow", 1280, 720);
	app->canvas = canvas->create(app->wind.handle);

	app->ui = fck_ui_alloc(&app->canvas);
	app->window_manager = fck_ui_window_manager_alloc(16);

	// fck_type_system *ts = fck_load_type_system(apis);
	app->assembly = ts->assembly->alloc(kll_heap);

	fck_ui_window_manager_create(app->window_manager, "Entities", app, fck_ui_window_entities);
	fck_ui_window_manager_create(app->window_manager, "Nk Overview", NULL, fck_ui_window_overview);
	fck_ui_set_style(fck_ui_context(app->ui), THEME_DRACULA);

	setup_some_stuff(app);

	return app;
}

void fck_instance_free(fck_instance *instance)
{
	fck_ui_window_manager_free(instance->window_manager);

	fck_ui_free(instance->ui);
	canvas->destroy(instance->canvas);
	os->win->destroy(instance->wind);
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

	canvas->set_color(instance->canvas, 0, 0, 0, 0);
	canvas->clear(instance->canvas);

	fck_ui_render(instance->ui, &instance->canvas);

	canvas->present(instance->canvas);

	return FCK_INSTANCE_CONTINUE;
}
