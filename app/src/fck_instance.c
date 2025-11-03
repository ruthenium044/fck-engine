#include <fck_hash.h>

#include "fck_ui.h"

#include "fck_nuklear_demos.h"
#include "fck_ui_window_manager.h"

#include "fck_serialiser_ext.h"
#include "fck_serialiser_nk_edit_vt.h"
#include "fck_serialiser_vt.h"

#include <kll.h>
#include <kll_heap.h>
#include <kll_malloc.h>
#include <kll_temp.h>

#include <fckc_inttypes.h>

#include "fck_type_system.h"
#include <fck_apis.h>

#include <fck_os.h>
#include <fck_set.h>

#include <fck_canvas.h>
#include <fck_img.h>
#include <fck_render.h>

#include <fck_app_loadable.h>
#include <fck_events.h>

typedef struct fck_input_event
{
	fckc_u64 time_stamp;
} fck_input_event;

// This struct is good enough for now
typedef struct fck_asset_database
{
	char *root;
} fck_asset_database;

char *fck_asset_database_make_path(fck_asset_database *db, kll_temp_allocator *alloactor, const char *path)
{
	return kll_temp_format(alloactor, "%s/%s", db->root, path);
}

typedef struct fck_asset_database_api
{
	fck_asset_database (*create)(char *root);
	void (*destroy)(fck_asset_database db);
	char *(*make_path)(fck_asset_database *db, kll_temp_allocator *alloactor, const char *path);
} fck_asset_database_api;

fck_asset_database fck_asset_database_create(char *root)
{
	fck_asset_database db; // = (fck_asset_database *)kll_malloc(allocator, sizeof(*db));
	db.root = root;
	return db;
}

void fck_asset_database_destroy(fck_asset_database db)
{
}

static fck_asset_database_api asset_database_api = {
	.create = fck_asset_database_create,
	.destroy = fck_asset_database_destroy,
	.make_path = fck_asset_database_make_path,
};

// typedef struct fck_asset_database_api
//{
//
// };

typedef struct fck_instance
{
	// Polymnorphism hehe
	fck_app app;

	fck_ui *ui; // User
	// struct SDL_Window *window;     // This one could stay public - Makes sense for multi-instance stuff
	// struct SDL_Renderer *renderer; // User
	fck_window wind;
	fck_renderer renderer;

	fck_ui_window_manager *window_manager;
	struct fck_assembly *assembly;

	fck_asset_database assets;
} fck_instance;

fck_api_registry *api;
fck_type_system *ts;
fck_canvas_api *canvas;
fck_render_api *render_api;
fck_serialiser_ext_api *ser_api;
fck_img_api *img;
fck_asset_database_api *asset_db = &asset_database_api;

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

int fck_instance_overlay(fck_instance *instance)
{
	int width;
	int height;
	if (!os->win->size_get(instance->wind, &width, &height))
	{
		return 1;
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
	return 0;
}

static fck_texture texture;

char *fck_instance_parse_runtime_asset_path(int argc, char *argv[])
{
	for (int i = 0; i < argc; i++)
	{
		char *current = argv[i];
		// int asset_len = os->str->len("-assets", 16);
		char *entry = os->str->find->string(current, "-assets");
		if (entry)
		{
			char *cntrl = os->str->find->chr(entry, '=');
			if (cntrl)
			{
				char *begin = os->str->find->chr(cntrl + 1, '"');
				if (begin)
				{
					begin = begin + 1;
					char *end = os->str->find->chr(begin, '"');
					if (end)
					{
						fckc_size_t len = (fckc_size_t)end - (fckc_size_t)begin;
						return os->str->dup(begin, len);
					}
				}
			}
		}
	}
	return NULL;
}

char *fck_instance_resolve_asset_path(int argc, char *argv[])
{
	char *arg_asset_path = fck_instance_parse_runtime_asset_path(argc, argv);
	if (arg_asset_path)
	{
		return arg_asset_path;
	}
	// Weird and small dups for consistency...
	// Free it, or don't lol
#if defined(FCK_APP_ASSET_PATH)
	return os->str->dup(FCK_APP_ASSET_PATH, sizeof(FCK_APP_ASSET_PATH));
#else
	return os->str->unsafe->dup("/");
#endif
}

fck_instance *fck_instance_alloc(int argc, char *argv[])
{
	fck_apis_manifest manifest[] = {
		{.api = (void **)&ts, .name = "fck-ts", NULL},
		{.api = (void **)&canvas, .name = "fck-canvas", NULL},
		{.api = (void **)&render_api, .name = "fck-render-sdl", NULL},
		{.api = (void **)&ser_api, .name = "fck-ser-ext"},
		{.api = (void **)&img, .name = "fck-img-sdl"},
	};

	fck_apis_init init = (fck_apis_init){
		.manifest = manifest,
		.count = fck_arraysize(manifest),
	};
	fck_shared_object api_so = os->so->load("fck-api");
	fck_main_func *main_so = (fck_main_func *)os->so->symbol(api_so, FCK_ENTRY_POINT);
	api = (fck_api_registry *)main_so(api, &init);

	fck_instance *app = (fck_instance *)kll_malloc(kll_heap, sizeof(fck_instance));
	char *asset_root = fck_instance_resolve_asset_path(argc, argv);
	app->assets = asset_db->create(asset_root);

	app->wind = os->win->create("Widnow", 1280, 720);
	app->renderer = render_api->new(&app->wind);

	app->ui = fck_ui_alloc(&app->renderer);
	app->window_manager = fck_ui_window_manager_alloc(16);

	kll_temp_allocator *temp = kll_temp_new(kll_heap, 256);
	char *image_path = asset_db->make_path(&app->assets, temp, "snow.png");
	fck_img image = img->load(kll_heap, image_path);
	kll_temp_delete(temp);

	texture = app->renderer.vt->texture->from_img(app->renderer.obj, &image, FCK_TEXTURE_ACCESS_STATIC, FCK_TEXTURE_BLEND_MODE_BLEND);
	img->free(image);

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

	fck_ui_free(instance->ui, &instance->renderer);
	render_api->delete(instance->renderer);
	os->win->destroy(instance->wind);
	kll_free(kll_heap, instance);
}

int fck_instance_event(fck_instance *instance, fck_event const *event)
{
	fck_ui_enqueue_event(instance->ui, event);
	return 0;
}

int fck_instance_tick(fck_instance *instance)
{
	fck_instance_overlay(instance);

	instance->renderer.vt->clear(instance->renderer.obj);

	fck_rect_dst dst = {400.0f, 400.0f, 667.0f / 2, 883.0f / 2};
	canvas->sprite(&instance->renderer, &texture, NULL, &dst);

	fck_ui_render(instance->ui, &instance->renderer);

	// canvas->rect(&instance->renderer, &dst);

	instance->renderer.vt->present(instance->renderer.obj);
	return 0;
}

struct fck_app *app_init(int argc, char *argv[])
{
	fck_instance *instance = fck_instance_alloc(argc, argv);
	instance->app.name = "fck app";
	return &instance->app;
}

void app_quit(struct fck_app *app)
{
	fck_instance *instance = (fck_instance *)app;
	fck_instance_free(instance);
}

int app_tick(struct fck_app *app)
{
	fck_instance *instance = (fck_instance *)app;
	return fck_instance_tick(instance);
}

int app_on_event(struct fck_app* app, struct fck_event const* event)
{
	fck_instance* instance = (fck_instance*)app;
	fck_instance_event(instance, event);
	return 0;
}

static fck_app_api app_api = {
	.init = app_init,
	.tick = app_tick,
	.quit = app_quit,
	.on_event = app_on_event,
};

FCK_EXPORT_API fck_app_api *fck_load(void)
{
	// api->add("FCK_APP", &app_api);
	return &app_api;
}