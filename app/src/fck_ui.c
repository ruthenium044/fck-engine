
#define NK_IMPLEMENTATION
#include "fck_ui.h"

#include <fck_os.h>
#include <fckc_assert.h>
#include <fckc_inttypes.h>

// #include <SDL3/SDL_clipboard.h>
// #include <SDL3/SDL_events.h>
// #include <SDL3/SDL_log.h>
//  #include <SDL3/SDL_timer.h>

/*
 * ==============================================================
 *
 *                          IMPLEMENTATION
 *
 * ===============================================================
 */
// TODO: Prefer SDL libs
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// #include <fck_render.h>

#include <kll.h>
#include <kll_heap.h>
#include <kll_malloc.h>

#include <fck_events.h>
#include <fck_shader.h>
#include <sht_render.h>

// typedef union nk_sdl_input_event {
//	fck_event
//	SDL_EventType type;
//	SDL_KeyboardEvent key;
//	SDL_MouseButtonEvent button;
//	SDL_MouseMotionEvent motion;
//	SDL_MouseWheelEvent wheel;
//	SDL_TextInputEvent text;
// } nk_sdl_input_event;

// UI vertex
typedef struct fck_vertex_ui
{
	fckc_f32 position[2];
	fckc_f32 colour[4];
	fckc_f32 uv[2];

} fck_vertex_ui;

void fck_vertex_ui_log(fck_vertex_ui const *v)
{
	os->io->log("x: %f.4 - y: %f.4 - r: %f.4 - g: %f.4 - b: %f.4 - a: %f.4 - u: %f.4 - v: %f.4", v->position[0], v->position[1],
	            v->colour[0], v->colour[1], v->colour[2], v->colour[3], v->uv[0], v->uv[1]);
}

const sht_vertex_binding vertex_bindings[] = {
	{.format = SHT_FORMAT_R32G32_SFLOAT, .offset = offsetof(fck_vertex_ui, position), .location = 0},
	{.format = SHT_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(fck_vertex_ui, colour), .location = 1},
	{.format = SHT_FORMAT_R32G32_SFLOAT, .offset = offsetof(fck_vertex_ui, uv), .location = 2}};

typedef struct nk_sdl_input_event_queue
{
	fckc_size_t count;
	fck_event events[64];
} nk_sdl_input_event_queue;

typedef struct nk_sdl_device
{
	struct nk_buffer cmds;
	struct nk_draw_null_texture tex_null;
} nk_sdl_device;

// typedef struct fck_renderer_vertex
//{
//	float position[2];
//	float col[4];
//	float uv[2];
// } fck_renderer_vertex;

typedef struct nk_sdl
{
	struct nk_sdl_device ogl;
	struct nk_font_atlas atlas;
	struct nk_context ctx;
	fckc_u64 time_of_last_frame;

	nk_sdl_input_event_queue input_queue;
} nk_sdl;

typedef struct fck_ui
{
	nk_sdl sdl;

	sht_buffer vertices;
	sht_buffer indices;

	sht_bss bss;
	sht_graphics_pipeline pipeline;
	struct
	{
		sht_image image;
		sht_image_view view;
	} font;

	sht_sampler sampler;

	sht_image depth_image;
	sht_image_view depth_view;

} fck_ui;

static int nk_sdl_input_event_is_valid(fck_event const *event)
{
	switch (event->type)
	{
	case FCK_EVENT_TYPE_DEVICE:
	case FCK_EVENT_TYPE_TEXT:
		return 1;
	default:
		return 0;
	}
}

static void nk_sdl_input_event_queue_reset(nk_sdl_input_event_queue *queue)
{
	queue->count = 0;
}

static void nk_sdl_input_event_queue_convert_and_maybe_push(nk_sdl_input_event_queue *queue, fck_event const *event)
{
	if (nk_sdl_input_event_is_valid(event))
	{
		fck_assert(queue->count < 64);
		memcpy(queue->events + queue->count, event, sizeof(*queue->events));
		queue->count = queue->count + 1;
	}
}

static void nk_sdl_device_upload_atlas(struct fck_ui *ui, struct sht_driver *driver, const void *pixels, int width, int height)
{
	struct nk_sdl_device *dev = &ui->sdl.ogl;
	sht_memory *mem = driver->vt->memory(*driver);
	ui->font.image = mem->image->create(mem->bump,
	                                    &(sht_image_configuration){.format = SHT_FORMAT_R8G8B8A8_UNORM,
	                                                               .height = height,
	                                                               .width = width,
	                                                               .transfer = SHT_TRANSFER_TARGET,
	                                                               .usage = SHT_IMAGE_USAGE_SAMPLED},
	                                    SHT_MEMORY_GPU);
	if (!mem->image->is_ok(&ui->font.image))
	{
		return;
	}
	fckc_size_t size = (fckc_size_t)width * height * 4;
	driver->vt->upload_image(*driver, &ui->font.image, pixels, size);
	ui->font.view = mem->image->view(mem->bump, ui->font.image, ui->font.image.format);
}

static void nk_sdl_clipboard_paste(nk_handle usr, struct nk_text_edit *edit)
{
	(void)usr;

	fck_clipboard clipboard = os->clipboard->receive();
	if (os->clipboard->is_valid(clipboard))
	{
		nk_textedit_paste(edit, clipboard.text, nk_strlen(clipboard.text));
	}
	os->clipboard->close(clipboard);
}

static void nk_sdl_clipboard_copy(nk_handle usr, const char *text, int len)
{
	char *str = 0;
	(void)usr;
	if (!len)
	{
		return;
	}
	str = (char *)malloc((fckc_size_t)len + 1);
	if (!str)
	{
		return;
	}
	memcpy(str, text, (fckc_size_t)len);
	str[len] = '\0';
	os->clipboard->set(str);
	free(str);
}

static void nk_sdl_font_stash_begin(struct fck_ui *ui, struct nk_font_atlas **atlas)
{
	nk_font_atlas_init_default(&ui->sdl.atlas);
	nk_font_atlas_begin(&ui->sdl.atlas);
	*atlas = &ui->sdl.atlas;
}

static void nk_sdl_font_stash_end(struct fck_ui *ui, struct sht_driver *driver)
{
	const void *image;
	int w, h;
	image = nk_font_atlas_bake(&ui->sdl.atlas, &w, &h, NK_FONT_ATLAS_RGBA32);
	nk_sdl_device_upload_atlas(ui, driver, image, w, h);
	nk_font_atlas_end(&ui->sdl.atlas, nk_handle_ptr(&ui->font), &ui->sdl.ogl.tex_null);
	if (ui->sdl.atlas.default_font)
		nk_style_set_font(&ui->sdl.ctx, &ui->sdl.atlas.default_font->handle);
}

struct fck_ui *fck_ui_init_internal()
{
	fck_ui *ui = (fck_ui *)kll_malloc(kll_heap, sizeof(*ui));

	ui->sdl.time_of_last_frame = os->chrono->ms();
	nk_init_default(&ui->sdl.ctx, 0);
	ui->sdl.ctx.clip.copy = nk_sdl_clipboard_copy;
	ui->sdl.ctx.clip.paste = nk_sdl_clipboard_paste;
	ui->sdl.ctx.clip.userdata = nk_handle_ptr(0);
	nk_buffer_init_default(&ui->sdl.ogl.cmds);
	return ui;
}

void fck_ui_free(fck_ui *ui, struct sht_driver *driver)
{
	struct nk_sdl_device *dev = &ui->sdl.ogl;
	nk_font_atlas_clear(&ui->sdl.atlas);
	nk_free(&ui->sdl.ctx);

	sht_memory *mem = driver->vt->memory(*driver);
	mem->image->discard(mem->bump, &ui->font.view);
	mem->image->destroy(mem->bump, &ui->font.image);
	mem->free(mem->bump, &ui->indices);
	mem->free(mem->bump, &ui->indices);
	/* glDeleteTextures(1, &dev->font_tex); */
	nk_buffer_free(&dev->cmds);
	memset(ui, 0, sizeof(*ui));

	kll_free(kll_heap, ui);
}

static void fck_ui_handle_event_device(struct fck_ui *ui, fck_event const *evt)
{
	// This might fuck with C89, but let's stick with C99 for declarations initialisation
	// until I am actually forced to move to C89 for that - which will realistically never happen
	// fck_event_as(fck_event_keyboard, keyboard, evt);
	// fck_event_as(fck_event_mouse, mouse, evt);
	// fck_event_as(fck_event_input_device, device, evt);
	int down;

	struct nk_context *ctx = &ui->sdl.ctx;

	switch (evt->device.device_type)
	{
	case FCK_INPUT_DEVICE_TYPE_MOUSE:
		switch (evt->mouse.type)
		{
		case FCK_MOUSE_EVENT_TYPE_BUTTON_LEFT:
			if (evt->mouse.clicks > 1)
			{
				nk_input_button(ctx, NK_BUTTON_DOUBLE, evt->mouse.x, evt->mouse.y, evt->mouse.is_down);
			}
			nk_input_button(ctx, NK_BUTTON_LEFT, evt->mouse.x, evt->mouse.y, evt->mouse.is_down);
			break;
		case FCK_MOUSE_EVENT_TYPE_BUTTON_RIGHT:
			nk_input_button(ctx, NK_BUTTON_RIGHT, evt->mouse.x, evt->mouse.y, evt->mouse.is_down);
			break;
		case FCK_MOUSE_EVENT_TYPE_BUTTON_MIDDLE:
			nk_input_button(ctx, NK_BUTTON_MIDDLE, evt->mouse.x, evt->mouse.y, evt->mouse.is_down);
			break;
		case FCK_MOUSE_EVENT_TYPE_WHEEL:
			nk_input_scroll(ctx, nk_vec2((float)evt->mouse.x, (float)evt->mouse.y));
			break;
		case FCK_MOUSE_EVENT_TYPE_POSITION:
			if (ctx->input.mouse.grabbed)
			{
				int x = (int)ctx->input.mouse.prev.x;
				int y = (int)ctx->input.mouse.prev.y;
				nk_input_motion(ctx, x + evt->mouse.dx, y + evt->mouse.dy);
			}
			else
			{
				nk_input_motion(ctx, evt->mouse.x, evt->mouse.y);
			}
			break;
		case FCK_MOUSE_EVENT_TYPE_BUTTON_NONE:
		case FCK_MOUSE_EVENT_TYPE_BUTTON_4:
		case FCK_MOUSE_EVENT_TYPE_BUTTON_5:
			break;
		}
		break;
	case FCK_INPUT_DEVICE_TYPE_KEYBOARD:
		down = evt->key.type == FCK_KEYBOARD_EVENT_TYPE_DOWN;
		switch (evt->key.pkey)
		{
		default:
			break;
		case FCK_PKEY_RSHIFT: /* RSHIFT & LSHIFT share same routine */
		case FCK_PKEY_LSHIFT:
			nk_input_key(ctx, NK_KEY_SHIFT, down);
			break;
		case FCK_PKEY_DELETE:
			nk_input_key(ctx, NK_KEY_DEL, down);
			break;
		case FCK_PKEY_RETURN:
			nk_input_key(ctx, NK_KEY_ENTER, down);
			break;
		case FCK_PKEY_TAB:
			nk_input_key(ctx, NK_KEY_TAB, down);
			break;
		case FCK_PKEY_BACKSPACE:
			nk_input_key(ctx, NK_KEY_BACKSPACE, down);
			break;
		case FCK_PKEY_HOME:
			nk_input_key(ctx, NK_KEY_TEXT_START, down);
			nk_input_key(ctx, NK_KEY_SCROLL_START, down);
			break;
		case FCK_PKEY_END:
			nk_input_key(ctx, NK_KEY_TEXT_END, down);
			nk_input_key(ctx, NK_KEY_SCROLL_END, down);
			break;
		case FCK_PKEY_PAGEDOWN:
			nk_input_key(ctx, NK_KEY_SCROLL_DOWN, down);
			break;
		case FCK_PKEY_PAGEUP:
			nk_input_key(ctx, NK_KEY_SCROLL_UP, down);
			break;
		case FCK_PKEY_UP:
			nk_input_key(ctx, NK_KEY_UP, down);
			break;
		case FCK_PKEY_DOWN:
			nk_input_key(ctx, NK_KEY_DOWN, down);
			break;
		case FCK_PKEY_LEFT:
			if ((evt->key.mod | FCK_KEY_MOD_LCTRL) == FCK_KEY_MOD_LCTRL)
				nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT, down);
			else
				nk_input_key(ctx, NK_KEY_LEFT, down);
			break;
		case FCK_PKEY_RIGHT:
			if ((evt->key.mod | FCK_KEY_MOD_LCTRL) == FCK_KEY_MOD_LCTRL)
				nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, down);
			else
				nk_input_key(ctx, NK_KEY_RIGHT, down);
			break;
		case FCK_PKEY_Z:
			nk_input_key(ctx, NK_KEY_TEXT_UNDO, down && (evt->key.mod | FCK_KEY_MOD_LCTRL) == FCK_KEY_MOD_LCTRL);
			break;
		case FCK_PKEY_R:
			nk_input_key(ctx, NK_KEY_TEXT_REDO, down && (evt->key.mod | FCK_KEY_MOD_LCTRL) == FCK_KEY_MOD_LCTRL);
			break;
		case FCK_PKEY_C:
			nk_input_key(ctx, NK_KEY_COPY, down && (evt->key.mod | FCK_KEY_MOD_LCTRL) == FCK_KEY_MOD_LCTRL);
			break;
		case FCK_PKEY_V:
			nk_input_key(ctx, NK_KEY_PASTE, down && (evt->key.mod | FCK_KEY_MOD_LCTRL) == FCK_KEY_MOD_LCTRL);
			break;
		case FCK_PKEY_X:
			nk_input_key(ctx, NK_KEY_CUT, down && (evt->key.mod | FCK_KEY_MOD_LCTRL) == FCK_KEY_MOD_LCTRL);
			break;
		case FCK_PKEY_B:
			nk_input_key(ctx, NK_KEY_TEXT_LINE_START, down && (evt->key.mod | FCK_KEY_MOD_LCTRL) == FCK_KEY_MOD_LCTRL);
			break;
		case FCK_PKEY_E:
			nk_input_key(ctx, NK_KEY_TEXT_LINE_END, down && (evt->key.mod | FCK_KEY_MOD_LCTRL) == FCK_KEY_MOD_LCTRL);
		}
		break;
	case FCK_INPUT_DEVICE_TYPE_NONE:
		// TODO: NONE IS ERROR
		break;
	}
}

static void fck_ui_handle_event(struct fck_ui *ui, fck_event const *evt)
{
	struct nk_context *ctx = &ui->sdl.ctx;
	// fck_event_as(fck_event_input_device, device, evt);
	// fck_event_as(fck_event_text_input, text, evt);

	switch (evt->type)
	{
	case FCK_EVENT_TYPE_DEVICE: {
		fck_ui_handle_event_device(ui, evt);
	}
	break;
	case FCK_EVENT_TYPE_TEXT: {
		nk_glyph glyph;
		size_t size;
		size = os->str->unsafe->len(evt->text.text);
		memcpy(glyph, evt->text.text, size);
		nk_input_glyph(ctx, glyph);
	}
	break;
	case FCK_EVENT_TYPE_NONE:
		// TODO: NONE IS ERROR
		break;
	}
}

struct fck_ui *fck_ui_alloc(struct sht_driver *driver)
{
	float font_scale = 8;

	fck_ui *ui = fck_ui_init_internal();

	{
		struct nk_font_atlas *atlas;
		struct nk_font_config config = nk_font_config(0);
		struct nk_font *font;

		/* set up the font atlas and add desired font; note that font sizes are
		 * multiplied by font_scale to produce better results at higher DPIs */
		nk_sdl_font_stash_begin(ui, &atlas);
		font = nk_font_atlas_add_default(atlas, 13 * font_scale, &config);
		/*font = nk_font_atlas_add_from_file(atlas, "../../../extra_font/DroidSans.ttf", 14 * font_scale, &config);*/
		/*font = nk_font_atlas_add_from_file(atlas, "../../../extra_font/Roboto-Regular.ttf", 16 * font_scale, &config);*/
		/*font = nk_font_atlas_add_from_file(atlas, "../../../extra_font/kenvector_future_thin.ttf", 13 * font_scale, &config);*/
		/*font = nk_font_atlas_add_from_file(atlas, "../../../extra_font/ProggyClean.ttf", 12 * font_scale, &config);*/
		/*font = nk_font_atlas_add_from_file(atlas, "../../../extra_font/ProggyTiny.ttf", 10 * font_scale, &config);*/
		/*font = nk_font_atlas_add_from_file(atlas, "../../../extra_font/Cousine-Regular.ttf", 13 * font_scale, &config);*/
		nk_sdl_font_stash_end(ui, driver);

		/* this hack makes the font appear to be scaled down to the desired
		 * size and is only necessary when font_scale > 1 */
		font->handle.height /= font_scale;
		/*nk_style_load_all_cursors(ctx, atlas->cursors);*/
		nk_style_set_font(&ui->sdl.ctx, &font->handle);
	}

	sht_swapchain swapchain = driver->vt->swapchain(*driver);
	sht_memory *mem = driver->vt->memory(*driver);

	sht_binding bindings[] = {
		{.id = 0, .type = SHT_BINDING_UNIFORM, .stages = SHT_STAGE_VERTEX_SHADER},
		{.id = 1, .type = SHT_BINDING_READ_ONLY_IMAGE, .stages = SHT_STAGE_FRAGMENT_SHADER},
	};

	ui->sampler = driver->vt->create_sampler(*driver);
	driver->vt->bss->create(*driver, &(sht_binding_desc){.bindings = bindings, .count = fck_arraysize(bindings)});

	sht_extent extent = swapchain.vt->extent(swapchain);
	{
		sht_image_configuration config = (sht_image_configuration){
			.format = SHT_FORMAT_D16_UNORM,
			.width = extent.width,
			.height = extent.height,
			.transfer = SHT_TRANSFER_RETAINED,
			.usage = SHT_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT,
		};

		ui->depth_image = mem->image->create(mem->bump, &config, SHT_MEMORY_GPU);
		ui->depth_view = mem->image->view(mem->bump, ui->depth_image, SHT_FORMAT_UNDEFINED);
		ui->vertices = mem->malloc(mem->bump, &sht_buffer_retained(SHT_BUFFER_USAGE_VERTEX, sizeof(fck_vertex_ui) * 4096), SHT_MEMORY_CPU);
		ui->indices = mem->malloc(mem->bump, &sht_buffer_retained(SHT_BUFFER_USAGE_INDEX, sizeof(fckc_u32) * 4096), SHT_MEMORY_CPU);
		ui->bss = driver->vt->bss->create(*driver, &(sht_binding_desc){.bindings = bindings, .count = fck_arraysize(bindings)});

		{
			fck_shader_compiler compiler = fck_shader_compiler_create();
			fck_file vert_file = os->fs->open("/Users/ruthenium/fck/app/assets/fck_ui.hlsl.vert", "r");
			fck_shader_desc vert_desc = (fck_shader_desc){FCK_SHADER_VERTEX, "triangle-vert", "main"};

			fck_file frag_file = os->fs->open("/Users/ruthenium/fck/app/assets/fck_ui.hlsl.frag", "r");
			fck_shader_desc frag_desc = (fck_shader_desc){FCK_SHADER_FRAGMENT, "triangle-frag", "main"};

			fck_hlsl_object vert = compiler.create_hlsl_from_file(&compiler, &vert_desc, &vert_file);
			fck_hlsl_object frag = compiler.create_hlsl_from_file(&compiler, &frag_desc, &frag_file);

			sht_graphic_desc desc = (sht_graphic_desc){.fragment = &frag.generic,
			                                           .vertex = &vert.generic,
			                                           .vertex_desc = &(sht_vertex_desc){.stride = sizeof(fck_vertex_ui),
			                                                                             .bindings = vertex_bindings,
			                                                                             .count = fck_arraysize(vertex_bindings)},
			                                           .raster = (sht_raster_desc){
														   .cull_mode = SHT_CULL_MODE_NONE,
														   .topology = SHT_TRIANGLE_LIST,
														   .color = SHT_FORMAT_B8G8R8A8_UNORM,
														   .depth = SHT_FORMAT_D16_UNORM,
													   }};
			ui->pipeline = driver->vt->graphics_pipeline->create(*driver, ui->bss, &desc);
			compiler.destroy(&compiler, &vert.generic);
			compiler.destroy(&compiler, &frag.generic);
			compiler.shutdown(&compiler);
		}
	}

	nk_sdl_input_event_queue_reset(&ui->sdl.input_queue);
	return ui;
}

void fck_ui_enqueue_event(struct fck_ui *ui, fck_event const *event)
{
	nk_sdl_input_event_queue_convert_and_maybe_push(&ui->sdl.input_queue, event);
}

struct nk_context *fck_ui_context(struct fck_ui *ui)
{
	return &ui->sdl.ctx;
}

void fck_ui_render(struct fck_ui *ui, struct sht_driver *driver, struct sht_command_buffer *command_buffer, struct sht_image_view *target)
{
	sht_command_buffer_vt *command = driver->vt->command_buffer;

	// Apply enqueued events
	{
		struct nk_context *context = &ui->sdl.ctx;
		nk_input_begin(context);
		for (int index = 0; index < ui->sdl.input_queue.count; index++)
		{
			fck_ui_handle_event(ui, &ui->sdl.input_queue.events[index]);
		}
		nk_input_end(context);
		nk_sdl_input_event_queue_reset(&ui->sdl.input_queue);
	}

	struct nk_sdl_device *dev = &ui->sdl.ogl;
	{
		// SDL_Rect saved_clip;

		//	int clipping_enabled;
		int vs = sizeof(struct fck_vertex_ui);
		fckc_size_t vp = offsetof(struct fck_vertex_ui, position);
		/* convert from command queue into draw list and draw to screen */
		const struct nk_draw_command *cmd;
		const nk_draw_index *offset = NULL;
		struct nk_buffer vbuf, ebuf;

		/* fill converting configuration */
		struct nk_convert_config config;
		static const struct nk_draw_vertex_layout_element vertex_layout[] = {
			{NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(struct fck_vertex_ui, position)},
			{NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(struct fck_vertex_ui, uv)},
			{NK_VERTEX_COLOR, NK_FORMAT_R32G32B32A32_FLOAT, NK_OFFSETOF(struct fck_vertex_ui, colour)},
			{NK_VERTEX_LAYOUT_END}};

		fckc_u64 now = os->chrono->ms();
		ui->sdl.ctx.delta_time_seconds = (float)(now - ui->sdl.time_of_last_frame) / 1000;
		ui->sdl.time_of_last_frame = now;

		NK_MEMSET(&config, 0, sizeof(config));
		config.vertex_layout = vertex_layout;
		config.vertex_size = sizeof(struct fck_vertex_ui);
		config.vertex_alignment = NK_ALIGNOF(struct fck_vertex_ui);
		config.tex_null = dev->tex_null;
		config.circle_segment_count = 22;
		config.curve_segment_count = 22;
		config.arc_segment_count = 22;
		config.global_alpha = 1.0f;
		config.shape_AA = NK_ANTI_ALIASING_OFF;
		config.line_AA = NK_ANTI_ALIASING_OFF;

		/* convert shapes into vertexes */
		nk_buffer_init_default(&vbuf);
		nk_buffer_init_default(&ebuf);
		nk_convert(&ui->sdl.ctx, &dev->cmds, &vbuf, &ebuf, &config);

		const fck_vertex_ui *vertices = (const fck_vertex_ui *)nk_buffer_memory_const(&vbuf);
		const void *indices = nk_buffer_memory_const(&ebuf);
		memcpy(ui->vertices.cpu, vertices, vbuf.needed);
		memcpy(ui->indices.cpu, indices, ebuf.needed);

		sht_swapchain swapchain = driver->vt->swapchain(*driver);

		sht_extent extent = swapchain.vt->extent(swapchain);

		float dpi = swapchain.vt->scale(swapchain);
		fckc_f32 projection[16] = {
			2.0f,  0.0f,  0.0f,  0.0f,

			0.0f,  2.0f,  0.0f,  0.0f,

			0.0f,  0.0f,  -1.0f, 0.0f,

			-1.0f, -1.0f, 0.0f,  1.0f,
		};
		projection[0] /= (extent.width / dpi);
		projection[5] /= (extent.height / dpi);

		sht_render_desc desc = (sht_render_desc){
			.colour = {.view = *target, .load_op = SHT_CLEAR, .store_op = SHT_STORE, .clear_value = {0.0f, 0.0f, 0.0f, 0.0f}},
			.depth = {.view = ui->depth_view, .load_op = SHT_CLEAR, .store_op = SHT_DONT_CARE, .clear_value = 0.0f},
		};

		sht_render_pass render_pass = command->render_pass->begin(*command_buffer, &desc);
		if (command->render_pass->is_ok(render_pass))
		{
			driver->vt->bss->upload(ui->bss, 0, sht_upload_params{.data = projection, .size = sizeof(projection)});
			driver->vt->bss->upload(ui->bss, 1, sht_upload_params{.view = ui->font.view, .sampler = ui->sampler});

			command->bss(*command_buffer, ui->bss);

			command->graphics_pipeline(*command_buffer, ui->pipeline);
			command->vertex_buffer(*command_buffer, &ui->vertices, 0);
			command->index_buffer(*command_buffer, &ui->indices, 0);

			fckc_size_t index_offset = 0;
			nk_draw_foreach(cmd, &ui->sdl.ctx, &dev->cmds)
			{
				if (!cmd->elem_count && !cmd->texture.ptr)
					continue;

				driver->vt->bss->upload(ui->bss, 1, sht_upload_params{.view = ui->font.view, .sampler = ui->sampler});

				fckc_u32 x = cmd->clip_rect.x;
				fckc_u32 y = cmd->clip_rect.y;
				sht_scissor scissor = (sht_scissor){.offset = {x, y}, .extent = {cmd->clip_rect.w * dpi, cmd->clip_rect.h * dpi}};
				command->scissor(*command_buffer, &scissor);
				command->draw_indexed(*command_buffer, &(sht_draw_indexed_desc){
														   .first_index = index_offset,
														   .index_count = cmd->elem_count,
														   .instance_count = 1,
														   .first_instance = 0,
														   .vertex_offset = 0,
													   });
				index_offset = index_offset + cmd->elem_count;
			}
			command->render_pass->end(*command_buffer);
		}

		nk_clear(&ui->sdl.ctx);
		nk_buffer_clear(&dev->cmds);
		nk_buffer_free(&vbuf);
		nk_buffer_free(&ebuf);
	}
}

static struct nk_color fck_ui_cached_colour_table[NK_COLOR_COUNT];

struct nk_color *fck_ui_set_style(struct nk_context *ctx, enum fck_theme theme)
{
	if (theme == THEME_WHITE)
	{
		fck_ui_cached_colour_table[NK_COLOR_TEXT] = nk_rgba(70, 70, 70, 255);
		fck_ui_cached_colour_table[NK_COLOR_WINDOW] = nk_rgba(175, 175, 175, 255);
		fck_ui_cached_colour_table[NK_COLOR_HEADER] = nk_rgba(175, 175, 175, 255);
		fck_ui_cached_colour_table[NK_COLOR_BORDER] = nk_rgba(0, 0, 0, 255);
		fck_ui_cached_colour_table[NK_COLOR_BUTTON] = nk_rgba(185, 185, 185, 255);
		fck_ui_cached_colour_table[NK_COLOR_BUTTON_HOVER] = nk_rgba(170, 170, 170, 255);
		fck_ui_cached_colour_table[NK_COLOR_BUTTON_ACTIVE] = nk_rgba(160, 160, 160, 255);
		fck_ui_cached_colour_table[NK_COLOR_TOGGLE] = nk_rgba(150, 150, 150, 255);
		fck_ui_cached_colour_table[NK_COLOR_TOGGLE_HOVER] = nk_rgba(120, 120, 120, 255);
		fck_ui_cached_colour_table[NK_COLOR_TOGGLE_CURSOR] = nk_rgba(175, 175, 175, 255);
		fck_ui_cached_colour_table[NK_COLOR_SELECT] = nk_rgba(190, 190, 190, 255);
		fck_ui_cached_colour_table[NK_COLOR_SELECT_ACTIVE] = nk_rgba(175, 175, 175, 255);
		fck_ui_cached_colour_table[NK_COLOR_SLIDER] = nk_rgba(190, 190, 190, 255);
		fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR] = nk_rgba(80, 80, 80, 255);
		fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR_HOVER] = nk_rgba(70, 70, 70, 255);
		fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = nk_rgba(60, 60, 60, 255);
		fck_ui_cached_colour_table[NK_COLOR_PROPERTY] = nk_rgba(175, 175, 175, 255);
		fck_ui_cached_colour_table[NK_COLOR_EDIT] = nk_rgba(150, 150, 150, 255);
		fck_ui_cached_colour_table[NK_COLOR_EDIT_CURSOR] = nk_rgba(0, 0, 0, 255);
		fck_ui_cached_colour_table[NK_COLOR_COMBO] = nk_rgba(175, 175, 175, 255);
		fck_ui_cached_colour_table[NK_COLOR_CHART] = nk_rgba(160, 160, 160, 255);
		fck_ui_cached_colour_table[NK_COLOR_CHART_COLOR] = nk_rgba(45, 45, 45, 255);
		fck_ui_cached_colour_table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = nk_rgba(255, 0, 0, 255);
		fck_ui_cached_colour_table[NK_COLOR_SCROLLBAR] = nk_rgba(180, 180, 180, 255);
		fck_ui_cached_colour_table[NK_COLOR_SCROLLBAR_CURSOR] = nk_rgba(140, 140, 140, 255);
		fck_ui_cached_colour_table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = nk_rgba(150, 150, 150, 255);
		fck_ui_cached_colour_table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = nk_rgba(160, 160, 160, 255);
		fck_ui_cached_colour_table[NK_COLOR_TAB_HEADER] = nk_rgba(180, 180, 180, 255);
		fck_ui_cached_colour_table[NK_COLOR_KNOB] = fck_ui_cached_colour_table[NK_COLOR_SLIDER];
		fck_ui_cached_colour_table[NK_COLOR_KNOB_CURSOR] = fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR];
		fck_ui_cached_colour_table[NK_COLOR_KNOB_CURSOR_HOVER] = fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR_HOVER];
		fck_ui_cached_colour_table[NK_COLOR_KNOB_CURSOR_ACTIVE] = fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR_ACTIVE];
		nk_style_from_table(ctx, fck_ui_cached_colour_table);
	}
	else if (theme == THEME_RED)
	{
		fck_ui_cached_colour_table[NK_COLOR_TEXT] = nk_rgba(190, 190, 190, 255);
		fck_ui_cached_colour_table[NK_COLOR_WINDOW] = nk_rgba(30, 33, 40, 215);
		fck_ui_cached_colour_table[NK_COLOR_HEADER] = nk_rgba(181, 45, 69, 220);
		fck_ui_cached_colour_table[NK_COLOR_BORDER] = nk_rgba(51, 55, 67, 255);
		fck_ui_cached_colour_table[NK_COLOR_BUTTON] = nk_rgba(181, 45, 69, 255);
		fck_ui_cached_colour_table[NK_COLOR_BUTTON_HOVER] = nk_rgba(190, 50, 70, 255);
		fck_ui_cached_colour_table[NK_COLOR_BUTTON_ACTIVE] = nk_rgba(195, 55, 75, 255);
		fck_ui_cached_colour_table[NK_COLOR_TOGGLE] = nk_rgba(51, 55, 67, 255);
		fck_ui_cached_colour_table[NK_COLOR_TOGGLE_HOVER] = nk_rgba(45, 60, 60, 255);
		fck_ui_cached_colour_table[NK_COLOR_TOGGLE_CURSOR] = nk_rgba(181, 45, 69, 255);
		fck_ui_cached_colour_table[NK_COLOR_SELECT] = nk_rgba(51, 55, 67, 255);
		fck_ui_cached_colour_table[NK_COLOR_SELECT_ACTIVE] = nk_rgba(181, 45, 69, 255);
		fck_ui_cached_colour_table[NK_COLOR_SLIDER] = nk_rgba(51, 55, 67, 255);
		fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR] = nk_rgba(181, 45, 69, 255);
		fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR_HOVER] = nk_rgba(186, 50, 74, 255);
		fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = nk_rgba(191, 55, 79, 255);
		fck_ui_cached_colour_table[NK_COLOR_PROPERTY] = nk_rgba(51, 55, 67, 255);
		fck_ui_cached_colour_table[NK_COLOR_EDIT] = nk_rgba(51, 55, 67, 225);
		fck_ui_cached_colour_table[NK_COLOR_EDIT_CURSOR] = nk_rgba(190, 190, 190, 255);
		fck_ui_cached_colour_table[NK_COLOR_COMBO] = nk_rgba(51, 55, 67, 255);
		fck_ui_cached_colour_table[NK_COLOR_CHART] = nk_rgba(51, 55, 67, 255);
		fck_ui_cached_colour_table[NK_COLOR_CHART_COLOR] = nk_rgba(170, 40, 60, 255);
		fck_ui_cached_colour_table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = nk_rgba(255, 0, 0, 255);
		fck_ui_cached_colour_table[NK_COLOR_SCROLLBAR] = nk_rgba(30, 33, 40, 255);
		fck_ui_cached_colour_table[NK_COLOR_SCROLLBAR_CURSOR] = nk_rgba(64, 84, 95, 255);
		fck_ui_cached_colour_table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = nk_rgba(70, 90, 100, 255);
		fck_ui_cached_colour_table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = nk_rgba(75, 95, 105, 255);
		fck_ui_cached_colour_table[NK_COLOR_TAB_HEADER] = nk_rgba(181, 45, 69, 220);
		fck_ui_cached_colour_table[NK_COLOR_KNOB] = fck_ui_cached_colour_table[NK_COLOR_SLIDER];
		fck_ui_cached_colour_table[NK_COLOR_KNOB_CURSOR] = fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR];
		fck_ui_cached_colour_table[NK_COLOR_KNOB_CURSOR_HOVER] = fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR_HOVER];
		fck_ui_cached_colour_table[NK_COLOR_KNOB_CURSOR_ACTIVE] = fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR_ACTIVE];
		nk_style_from_table(ctx, fck_ui_cached_colour_table);
	}
	else if (theme == THEME_BLUE)
	{
		fck_ui_cached_colour_table[NK_COLOR_TEXT] = nk_rgba(20, 20, 20, 255);
		fck_ui_cached_colour_table[NK_COLOR_WINDOW] = nk_rgba(202, 212, 214, 215);
		fck_ui_cached_colour_table[NK_COLOR_HEADER] = nk_rgba(137, 182, 224, 220);
		fck_ui_cached_colour_table[NK_COLOR_BORDER] = nk_rgba(140, 159, 173, 255);
		fck_ui_cached_colour_table[NK_COLOR_BUTTON] = nk_rgba(137, 182, 224, 255);
		fck_ui_cached_colour_table[NK_COLOR_BUTTON_HOVER] = nk_rgba(142, 187, 229, 255);
		fck_ui_cached_colour_table[NK_COLOR_BUTTON_ACTIVE] = nk_rgba(147, 192, 234, 255);
		fck_ui_cached_colour_table[NK_COLOR_TOGGLE] = nk_rgba(177, 210, 210, 255);
		fck_ui_cached_colour_table[NK_COLOR_TOGGLE_HOVER] = nk_rgba(182, 215, 215, 255);
		fck_ui_cached_colour_table[NK_COLOR_TOGGLE_CURSOR] = nk_rgba(137, 182, 224, 255);
		fck_ui_cached_colour_table[NK_COLOR_SELECT] = nk_rgba(177, 210, 210, 255);
		fck_ui_cached_colour_table[NK_COLOR_SELECT_ACTIVE] = nk_rgba(137, 182, 224, 255);
		fck_ui_cached_colour_table[NK_COLOR_SLIDER] = nk_rgba(177, 210, 210, 255);
		fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR] = nk_rgba(137, 182, 224, 245);
		fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR_HOVER] = nk_rgba(142, 188, 229, 255);
		fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = nk_rgba(147, 193, 234, 255);
		fck_ui_cached_colour_table[NK_COLOR_PROPERTY] = nk_rgba(210, 210, 210, 255);
		fck_ui_cached_colour_table[NK_COLOR_EDIT] = nk_rgba(210, 210, 210, 225);
		fck_ui_cached_colour_table[NK_COLOR_EDIT_CURSOR] = nk_rgba(20, 20, 20, 255);
		fck_ui_cached_colour_table[NK_COLOR_COMBO] = nk_rgba(210, 210, 210, 255);
		fck_ui_cached_colour_table[NK_COLOR_CHART] = nk_rgba(210, 210, 210, 255);
		fck_ui_cached_colour_table[NK_COLOR_CHART_COLOR] = nk_rgba(137, 182, 224, 255);
		fck_ui_cached_colour_table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = nk_rgba(255, 0, 0, 255);
		fck_ui_cached_colour_table[NK_COLOR_SCROLLBAR] = nk_rgba(190, 200, 200, 255);
		fck_ui_cached_colour_table[NK_COLOR_SCROLLBAR_CURSOR] = nk_rgba(64, 84, 95, 255);
		fck_ui_cached_colour_table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = nk_rgba(70, 90, 100, 255);
		fck_ui_cached_colour_table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = nk_rgba(75, 95, 105, 255);
		fck_ui_cached_colour_table[NK_COLOR_TAB_HEADER] = nk_rgba(156, 193, 220, 255);
		fck_ui_cached_colour_table[NK_COLOR_KNOB] = fck_ui_cached_colour_table[NK_COLOR_SLIDER];
		fck_ui_cached_colour_table[NK_COLOR_KNOB_CURSOR] = fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR];
		fck_ui_cached_colour_table[NK_COLOR_KNOB_CURSOR_HOVER] = fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR_HOVER];
		fck_ui_cached_colour_table[NK_COLOR_KNOB_CURSOR_ACTIVE] = fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR_ACTIVE];
		nk_style_from_table(ctx, fck_ui_cached_colour_table);
	}
	else if (theme == THEME_DARK)
	{
		fck_ui_cached_colour_table[NK_COLOR_TEXT] = nk_rgba(210, 210, 210, 255);
		fck_ui_cached_colour_table[NK_COLOR_WINDOW] = nk_rgba(57, 67, 71, 215);
		fck_ui_cached_colour_table[NK_COLOR_HEADER] = nk_rgba(51, 51, 56, 220);
		fck_ui_cached_colour_table[NK_COLOR_BORDER] = nk_rgba(46, 46, 46, 255);
		fck_ui_cached_colour_table[NK_COLOR_BUTTON] = nk_rgba(48, 83, 111, 255);
		fck_ui_cached_colour_table[NK_COLOR_BUTTON_HOVER] = nk_rgba(58, 93, 121, 255);
		fck_ui_cached_colour_table[NK_COLOR_BUTTON_ACTIVE] = nk_rgba(63, 98, 126, 255);
		fck_ui_cached_colour_table[NK_COLOR_TOGGLE] = nk_rgba(50, 58, 61, 255);
		fck_ui_cached_colour_table[NK_COLOR_TOGGLE_HOVER] = nk_rgba(45, 53, 56, 255);
		fck_ui_cached_colour_table[NK_COLOR_TOGGLE_CURSOR] = nk_rgba(48, 83, 111, 255);
		fck_ui_cached_colour_table[NK_COLOR_SELECT] = nk_rgba(57, 67, 61, 255);
		fck_ui_cached_colour_table[NK_COLOR_SELECT_ACTIVE] = nk_rgba(48, 83, 111, 255);
		fck_ui_cached_colour_table[NK_COLOR_SLIDER] = nk_rgba(50, 58, 61, 255);
		fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR] = nk_rgba(48, 83, 111, 245);
		fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR_HOVER] = nk_rgba(53, 88, 116, 255);
		fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = nk_rgba(58, 93, 121, 255);
		fck_ui_cached_colour_table[NK_COLOR_PROPERTY] = nk_rgba(50, 58, 61, 255);
		fck_ui_cached_colour_table[NK_COLOR_EDIT] = nk_rgba(50, 58, 61, 225);
		fck_ui_cached_colour_table[NK_COLOR_EDIT_CURSOR] = nk_rgba(210, 210, 210, 255);
		fck_ui_cached_colour_table[NK_COLOR_COMBO] = nk_rgba(50, 58, 61, 255);
		fck_ui_cached_colour_table[NK_COLOR_CHART] = nk_rgba(50, 58, 61, 255);
		fck_ui_cached_colour_table[NK_COLOR_CHART_COLOR] = nk_rgba(48, 83, 111, 255);
		fck_ui_cached_colour_table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = nk_rgba(255, 0, 0, 255);
		fck_ui_cached_colour_table[NK_COLOR_SCROLLBAR] = nk_rgba(50, 58, 61, 255);
		fck_ui_cached_colour_table[NK_COLOR_SCROLLBAR_CURSOR] = nk_rgba(48, 83, 111, 255);
		fck_ui_cached_colour_table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = nk_rgba(53, 88, 116, 255);
		fck_ui_cached_colour_table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = nk_rgba(58, 93, 121, 255);
		fck_ui_cached_colour_table[NK_COLOR_TAB_HEADER] = nk_rgba(48, 83, 111, 255);
		fck_ui_cached_colour_table[NK_COLOR_KNOB] = fck_ui_cached_colour_table[NK_COLOR_SLIDER];
		fck_ui_cached_colour_table[NK_COLOR_KNOB_CURSOR] = fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR];
		fck_ui_cached_colour_table[NK_COLOR_KNOB_CURSOR_HOVER] = fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR_HOVER];
		fck_ui_cached_colour_table[NK_COLOR_KNOB_CURSOR_ACTIVE] = fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR_ACTIVE];
		nk_style_from_table(ctx, fck_ui_cached_colour_table);
	}
	else if (theme == THEME_DRACULA)
	{
		struct nk_color background = nk_rgba(40, 42, 54, 255);
		struct nk_color currentline = nk_rgba(68, 71, 90, 255);
		struct nk_color foreground = nk_rgba(248, 248, 242, 255);
		struct nk_color comment = nk_rgba(98, 114, 164, 255);
		/* struct nk_color cyan = nk_rgba(139, 233, 253, 255); */
		/* struct nk_color green = nk_rgba(80, 250, 123, 255); */
		/* struct nk_color orange = nk_rgba(255, 184, 108, 255); */
		struct nk_color pink = nk_rgba(255, 121, 198, 255);
		struct nk_color purple = nk_rgba(189, 147, 249, 255);
		/* struct nk_color red = nk_rgba(255, 85, 85, 255); */
		/* struct nk_color yellow = nk_rgba(241, 250, 140, 255); */
		fck_ui_cached_colour_table[NK_COLOR_TEXT] = foreground;
		fck_ui_cached_colour_table[NK_COLOR_WINDOW] = background;
		fck_ui_cached_colour_table[NK_COLOR_HEADER] = currentline;
		fck_ui_cached_colour_table[NK_COLOR_BORDER] = currentline;
		fck_ui_cached_colour_table[NK_COLOR_BUTTON] = currentline;
		fck_ui_cached_colour_table[NK_COLOR_BUTTON_HOVER] = comment;
		fck_ui_cached_colour_table[NK_COLOR_BUTTON_ACTIVE] = purple;
		fck_ui_cached_colour_table[NK_COLOR_TOGGLE] = currentline;
		fck_ui_cached_colour_table[NK_COLOR_TOGGLE_HOVER] = comment;
		fck_ui_cached_colour_table[NK_COLOR_TOGGLE_CURSOR] = pink;
		fck_ui_cached_colour_table[NK_COLOR_SELECT] = currentline;
		fck_ui_cached_colour_table[NK_COLOR_SELECT_ACTIVE] = comment;
		fck_ui_cached_colour_table[NK_COLOR_SLIDER] = background;
		fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR] = currentline;
		fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR_HOVER] = comment;
		fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = comment;
		fck_ui_cached_colour_table[NK_COLOR_PROPERTY] = currentline;
		fck_ui_cached_colour_table[NK_COLOR_EDIT] = currentline;
		fck_ui_cached_colour_table[NK_COLOR_EDIT_CURSOR] = foreground;
		fck_ui_cached_colour_table[NK_COLOR_COMBO] = currentline;
		fck_ui_cached_colour_table[NK_COLOR_CHART] = currentline;
		fck_ui_cached_colour_table[NK_COLOR_CHART_COLOR] = comment;
		fck_ui_cached_colour_table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = purple;
		fck_ui_cached_colour_table[NK_COLOR_SCROLLBAR] = background;
		fck_ui_cached_colour_table[NK_COLOR_SCROLLBAR_CURSOR] = currentline;
		fck_ui_cached_colour_table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = comment;
		fck_ui_cached_colour_table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = purple;
		fck_ui_cached_colour_table[NK_COLOR_TAB_HEADER] = currentline;
		fck_ui_cached_colour_table[NK_COLOR_KNOB] = fck_ui_cached_colour_table[NK_COLOR_SLIDER];
		fck_ui_cached_colour_table[NK_COLOR_KNOB_CURSOR] = fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR];
		fck_ui_cached_colour_table[NK_COLOR_KNOB_CURSOR_HOVER] = fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR_HOVER];
		fck_ui_cached_colour_table[NK_COLOR_KNOB_CURSOR_ACTIVE] = fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR_ACTIVE];
		nk_style_from_table(ctx, fck_ui_cached_colour_table);
	}
	else if (theme == THEME_CATPPUCCIN_LATTE)
	{
		/*struct nk_color rosewater = nk_rgba(220, 138, 120, 255);*/
		/*struct nk_color flamingo = nk_rgba(221, 120, 120, 255);*/
		struct nk_color pink = nk_rgba(234, 118, 203, 255);
		struct nk_color mauve = nk_rgba(136, 57, 239, 255);
		/*struct nk_color red = nk_rgba(210, 15, 57, 255);*/
		/*struct nk_color maroon = nk_rgba(230, 69, 83, 255);*/
		/*struct nk_color peach = nk_rgba(254, 100, 11, 255);*/
		struct nk_color yellow = nk_rgba(223, 142, 29, 255);
		/*struct nk_color green = nk_rgba(64, 160, 43, 255);*/
		struct nk_color teal = nk_rgba(23, 146, 153, 255);
		/*struct nk_color sky = nk_rgba(4, 165, 229, 255);*/
		/*struct nk_color sapphire = nk_rgba(32, 159, 181, 255);*/
		/*struct nk_color blue = nk_rgba(30, 102, 245, 255);*/
		/*struct nk_color lavender = nk_rgba(114, 135, 253, 255);*/
		struct nk_color text = nk_rgba(76, 79, 105, 255);
		/*struct nk_color subtext1 = nk_rgba(92, 95, 119, 255);*/
		/*struct nk_color subtext0 = nk_rgba(108, 111, 133, 255);*/
		struct nk_color overlay2 = nk_rgba(124, 127, 147, 55);
		/*struct nk_color overlay1 = nk_rgba(140, 143, 161, 255);*/
		struct nk_color overlay0 = nk_rgba(156, 160, 176, 255);
		struct nk_color surface2 = nk_rgba(172, 176, 190, 255);
		struct nk_color surface1 = nk_rgba(188, 192, 204, 255);
		struct nk_color surface0 = nk_rgba(204, 208, 218, 255);
		struct nk_color base = nk_rgba(239, 241, 245, 255);
		struct nk_color mantle = nk_rgba(230, 233, 239, 255);
		/*struct nk_color crust = nk_rgba(220, 224, 232, 255);*/
		fck_ui_cached_colour_table[NK_COLOR_TEXT] = text;
		fck_ui_cached_colour_table[NK_COLOR_WINDOW] = base;
		fck_ui_cached_colour_table[NK_COLOR_HEADER] = mantle;
		fck_ui_cached_colour_table[NK_COLOR_BORDER] = mantle;
		fck_ui_cached_colour_table[NK_COLOR_BUTTON] = surface0;
		fck_ui_cached_colour_table[NK_COLOR_BUTTON_HOVER] = overlay2;
		fck_ui_cached_colour_table[NK_COLOR_BUTTON_ACTIVE] = overlay0;
		fck_ui_cached_colour_table[NK_COLOR_TOGGLE] = surface2;
		fck_ui_cached_colour_table[NK_COLOR_TOGGLE_HOVER] = overlay2;
		fck_ui_cached_colour_table[NK_COLOR_TOGGLE_CURSOR] = yellow;
		fck_ui_cached_colour_table[NK_COLOR_SELECT] = surface0;
		fck_ui_cached_colour_table[NK_COLOR_SELECT_ACTIVE] = overlay0;
		fck_ui_cached_colour_table[NK_COLOR_SLIDER] = surface1;
		fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR] = teal;
		fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR_HOVER] = teal;
		fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = teal;
		fck_ui_cached_colour_table[NK_COLOR_PROPERTY] = surface0;
		fck_ui_cached_colour_table[NK_COLOR_EDIT] = surface0;
		fck_ui_cached_colour_table[NK_COLOR_EDIT_CURSOR] = mauve;
		fck_ui_cached_colour_table[NK_COLOR_COMBO] = surface0;
		fck_ui_cached_colour_table[NK_COLOR_CHART] = surface0;
		fck_ui_cached_colour_table[NK_COLOR_CHART_COLOR] = teal;
		fck_ui_cached_colour_table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = mauve;
		fck_ui_cached_colour_table[NK_COLOR_SCROLLBAR] = surface0;
		fck_ui_cached_colour_table[NK_COLOR_SCROLLBAR_CURSOR] = overlay0;
		fck_ui_cached_colour_table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = mauve;
		fck_ui_cached_colour_table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = mauve;
		fck_ui_cached_colour_table[NK_COLOR_TAB_HEADER] = surface0;
		fck_ui_cached_colour_table[NK_COLOR_KNOB] = fck_ui_cached_colour_table[NK_COLOR_SLIDER];
		fck_ui_cached_colour_table[NK_COLOR_KNOB_CURSOR] = pink;
		fck_ui_cached_colour_table[NK_COLOR_KNOB_CURSOR_HOVER] = pink;
		fck_ui_cached_colour_table[NK_COLOR_KNOB_CURSOR_ACTIVE] = pink;
		nk_style_from_table(ctx, fck_ui_cached_colour_table);
	}
	else if (theme == THEME_CATPPUCCIN_FRAPPE)
	{
		/*struct nk_color rosewater = nk_rgba(242, 213, 207, 255);*/
		/*struct nk_color flamingo = nk_rgba(238, 190, 190, 255);*/
		struct nk_color pink = nk_rgba(244, 184, 228, 255);
		/*struct nk_color mauve = nk_rgba(202, 158, 230, 255);*/
		/*struct nk_color red = nk_rgba(231, 130, 132, 255);*/
		/*struct nk_color maroon = nk_rgba(234, 153, 156, 255);*/
		/*struct nk_color peach = nk_rgba(239, 159, 118, 255);*/
		/*struct nk_color yellow = nk_rgba(229, 200, 144, 255);*/
		struct nk_color green = nk_rgba(166, 209, 137, 255);
		/*struct nk_color teal = nk_rgba(129, 200, 190, 255);*/
		/*struct nk_color sky = nk_rgba(153, 209, 219, 255);*/
		/*struct nk_color sapphire = nk_rgba(133, 193, 220, 255);*/
		/*struct nk_color blue = nk_rgba(140, 170, 238, 255);*/
		struct nk_color lavender = nk_rgba(186, 187, 241, 255);
		struct nk_color text = nk_rgba(198, 208, 245, 255);
		/*struct nk_color subtext1 = nk_rgba(181, 191, 226, 255);*/
		/*struct nk_color subtext0 = nk_rgba(165, 173, 206, 255);*/
		struct nk_color overlay2 = nk_rgba(148, 156, 187, 255);
		struct nk_color overlay1 = nk_rgba(131, 139, 167, 255);
		struct nk_color overlay0 = nk_rgba(115, 121, 148, 255);
		struct nk_color surface2 = nk_rgba(98, 104, 128, 255);
		struct nk_color surface1 = nk_rgba(81, 87, 109, 255);
		struct nk_color surface0 = nk_rgba(65, 69, 89, 255);
		struct nk_color base = nk_rgba(48, 52, 70, 255);
		struct nk_color mantle = nk_rgba(41, 44, 60, 255);
		/*struct nk_color crust = nk_rgba(35, 38, 52, 255);*/
		fck_ui_cached_colour_table[NK_COLOR_TEXT] = text;
		fck_ui_cached_colour_table[NK_COLOR_WINDOW] = base;
		fck_ui_cached_colour_table[NK_COLOR_HEADER] = mantle;
		fck_ui_cached_colour_table[NK_COLOR_BORDER] = mantle;
		fck_ui_cached_colour_table[NK_COLOR_BUTTON] = surface0;
		fck_ui_cached_colour_table[NK_COLOR_BUTTON_HOVER] = overlay1;
		fck_ui_cached_colour_table[NK_COLOR_BUTTON_ACTIVE] = overlay0;
		fck_ui_cached_colour_table[NK_COLOR_TOGGLE] = surface2;
		fck_ui_cached_colour_table[NK_COLOR_TOGGLE_HOVER] = overlay2;
		fck_ui_cached_colour_table[NK_COLOR_TOGGLE_CURSOR] = pink;
		fck_ui_cached_colour_table[NK_COLOR_SELECT] = surface0;
		fck_ui_cached_colour_table[NK_COLOR_SELECT_ACTIVE] = overlay0;
		fck_ui_cached_colour_table[NK_COLOR_SLIDER] = surface1;
		fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR] = green;
		fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR_HOVER] = green;
		fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = green;
		fck_ui_cached_colour_table[NK_COLOR_PROPERTY] = surface0;
		fck_ui_cached_colour_table[NK_COLOR_EDIT] = surface0;
		fck_ui_cached_colour_table[NK_COLOR_EDIT_CURSOR] = pink;
		fck_ui_cached_colour_table[NK_COLOR_COMBO] = surface0;
		fck_ui_cached_colour_table[NK_COLOR_CHART] = surface0;
		fck_ui_cached_colour_table[NK_COLOR_CHART_COLOR] = lavender;
		fck_ui_cached_colour_table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = pink;
		fck_ui_cached_colour_table[NK_COLOR_SCROLLBAR] = surface0;
		fck_ui_cached_colour_table[NK_COLOR_SCROLLBAR_CURSOR] = overlay0;
		fck_ui_cached_colour_table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = lavender;
		fck_ui_cached_colour_table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = lavender;
		fck_ui_cached_colour_table[NK_COLOR_TAB_HEADER] = surface0;
		fck_ui_cached_colour_table[NK_COLOR_KNOB] = fck_ui_cached_colour_table[NK_COLOR_SLIDER];
		fck_ui_cached_colour_table[NK_COLOR_KNOB_CURSOR] = pink;
		fck_ui_cached_colour_table[NK_COLOR_KNOB_CURSOR_HOVER] = pink;
		fck_ui_cached_colour_table[NK_COLOR_KNOB_CURSOR_ACTIVE] = pink;
		nk_style_from_table(ctx, fck_ui_cached_colour_table);
	}
	else if (theme == THEME_CATPPUCCIN_MACCHIATO)
	{
		/*struct nk_color rosewater = nk_rgba(244, 219, 214, 255);*/
		/*struct nk_color flamingo = nk_rgba(240, 198, 198, 255);*/
		struct nk_color pink = nk_rgba(245, 189, 230, 255);
		/*struct nk_color mauve = nk_rgba(198, 160, 246, 255);*/
		/*struct nk_color red = nk_rgba(237, 135, 150, 255);*/
		/*struct nk_color maroon = nk_rgba(238, 153, 160, 255);*/
		/*struct nk_color peach = nk_rgba(245, 169, 127, 255);*/
		struct nk_color yellow = nk_rgba(238, 212, 159, 255);
		struct nk_color green = nk_rgba(166, 218, 149, 255);
		/*struct nk_color teal = nk_rgba(139, 213, 202, 255);*/
		/*struct nk_color sky = nk_rgba(145, 215, 227, 255);*/
		/*struct nk_color sapphire = nk_rgba(125, 196, 228, 255);*/
		/*struct nk_color blue = nk_rgba(138, 173, 244, 255);*/
		struct nk_color lavender = nk_rgba(183, 189, 248, 255);
		struct nk_color text = nk_rgba(202, 211, 245, 255);
		/*struct nk_color subtext1 = nk_rgba(184, 192, 224, 255);*/
		/*struct nk_color subtext0 = nk_rgba(165, 173, 203, 255);*/
		struct nk_color overlay2 = nk_rgba(147, 154, 183, 255);
		struct nk_color overlay1 = nk_rgba(128, 135, 162, 255);
		struct nk_color overlay0 = nk_rgba(110, 115, 141, 255);
		struct nk_color surface2 = nk_rgba(91, 96, 120, 255);
		struct nk_color surface1 = nk_rgba(73, 77, 100, 255);
		struct nk_color surface0 = nk_rgba(54, 58, 79, 255);
		struct nk_color base = nk_rgba(36, 39, 58, 255);
		struct nk_color mantle = nk_rgba(30, 32, 48, 255);
		/*struct nk_color crust = nk_rgba(24, 25, 38, 255);*/
		fck_ui_cached_colour_table[NK_COLOR_TEXT] = text;
		fck_ui_cached_colour_table[NK_COLOR_WINDOW] = base;
		fck_ui_cached_colour_table[NK_COLOR_HEADER] = mantle;
		fck_ui_cached_colour_table[NK_COLOR_BORDER] = mantle;
		fck_ui_cached_colour_table[NK_COLOR_BUTTON] = surface0;
		fck_ui_cached_colour_table[NK_COLOR_BUTTON_HOVER] = overlay1;
		fck_ui_cached_colour_table[NK_COLOR_BUTTON_ACTIVE] = overlay0;
		fck_ui_cached_colour_table[NK_COLOR_TOGGLE] = surface2;
		fck_ui_cached_colour_table[NK_COLOR_TOGGLE_HOVER] = overlay2;
		fck_ui_cached_colour_table[NK_COLOR_TOGGLE_CURSOR] = yellow;
		fck_ui_cached_colour_table[NK_COLOR_SELECT] = surface0;
		fck_ui_cached_colour_table[NK_COLOR_SELECT_ACTIVE] = overlay0;
		fck_ui_cached_colour_table[NK_COLOR_SLIDER] = surface1;
		fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR] = green;
		fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR_HOVER] = green;
		fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = green;
		fck_ui_cached_colour_table[NK_COLOR_PROPERTY] = surface0;
		fck_ui_cached_colour_table[NK_COLOR_EDIT] = surface0;
		fck_ui_cached_colour_table[NK_COLOR_EDIT_CURSOR] = pink;
		fck_ui_cached_colour_table[NK_COLOR_COMBO] = surface0;
		fck_ui_cached_colour_table[NK_COLOR_CHART] = surface0;
		fck_ui_cached_colour_table[NK_COLOR_CHART_COLOR] = lavender;
		fck_ui_cached_colour_table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = yellow;
		fck_ui_cached_colour_table[NK_COLOR_SCROLLBAR] = surface0;
		fck_ui_cached_colour_table[NK_COLOR_SCROLLBAR_CURSOR] = overlay0;
		fck_ui_cached_colour_table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = lavender;
		fck_ui_cached_colour_table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = lavender;
		fck_ui_cached_colour_table[NK_COLOR_TAB_HEADER] = surface0;
		fck_ui_cached_colour_table[NK_COLOR_KNOB] = fck_ui_cached_colour_table[NK_COLOR_SLIDER];
		fck_ui_cached_colour_table[NK_COLOR_KNOB_CURSOR] = pink;
		fck_ui_cached_colour_table[NK_COLOR_KNOB_CURSOR_HOVER] = pink;
		fck_ui_cached_colour_table[NK_COLOR_KNOB_CURSOR_ACTIVE] = pink;
		nk_style_from_table(ctx, fck_ui_cached_colour_table);
	}
	else if (theme == THEME_CATPPUCCIN_MOCHA)
	{
		/*struct nk_color rosewater = nk_rgba(245, 224, 220, 255);*/
		/*struct nk_color flamingo = nk_rgba(242, 205, 205, 255);*/
		struct nk_color pink = nk_rgba(245, 194, 231, 255);
		/*struct nk_color mauve = nk_rgba(203, 166, 247, 255);*/
		/*struct nk_color red = nk_rgba(243, 139, 168, 255);*/
		/*struct nk_color maroon = nk_rgba(235, 160, 172, 255);*/
		/*struct nk_color peach = nk_rgba(250, 179, 135, 255);*/
		/*struct nk_color yellow = nk_rgba(249, 226, 175, 255);*/
		struct nk_color green = nk_rgba(166, 227, 161, 255);
		/*struct nk_color teal = nk_rgba(148, 226, 213, 255);*/
		/*struct nk_color sky = nk_rgba(137, 220, 235, 255);*/
		/*struct nk_color sapphire = nk_rgba(116, 199, 236, 255);*/
		/*struct nk_color blue = nk_rgba(137, 180, 250, 255);*/
		struct nk_color lavender = nk_rgba(180, 190, 254, 255);
		struct nk_color text = nk_rgba(205, 214, 244, 255);
		/*struct nk_color subtext1 = nk_rgba(186, 194, 222, 255);*/
		/*struct nk_color subtext0 = nk_rgba(166, 173, 200, 255);*/
		struct nk_color overlay2 = nk_rgba(147, 153, 178, 255);
		struct nk_color overlay1 = nk_rgba(127, 132, 156, 255);
		struct nk_color overlay0 = nk_rgba(108, 112, 134, 255);
		struct nk_color surface2 = nk_rgba(88, 91, 112, 255);
		struct nk_color surface1 = nk_rgba(69, 71, 90, 255);
		struct nk_color surface0 = nk_rgba(49, 50, 68, 255);
		struct nk_color base = nk_rgba(30, 30, 46, 255);
		struct nk_color mantle = nk_rgba(24, 24, 37, 255);
		/*struct nk_color crust = nk_rgba(17, 17, 27, 255);*/
		fck_ui_cached_colour_table[NK_COLOR_TEXT] = text;
		fck_ui_cached_colour_table[NK_COLOR_WINDOW] = base;
		fck_ui_cached_colour_table[NK_COLOR_HEADER] = mantle;
		fck_ui_cached_colour_table[NK_COLOR_BORDER] = mantle;
		fck_ui_cached_colour_table[NK_COLOR_BUTTON] = surface0;
		fck_ui_cached_colour_table[NK_COLOR_BUTTON_HOVER] = overlay1;
		fck_ui_cached_colour_table[NK_COLOR_BUTTON_ACTIVE] = overlay0;
		fck_ui_cached_colour_table[NK_COLOR_TOGGLE] = surface2;
		fck_ui_cached_colour_table[NK_COLOR_TOGGLE_HOVER] = overlay2;
		fck_ui_cached_colour_table[NK_COLOR_TOGGLE_CURSOR] = lavender;
		fck_ui_cached_colour_table[NK_COLOR_SELECT] = surface0;
		fck_ui_cached_colour_table[NK_COLOR_SELECT_ACTIVE] = overlay0;
		fck_ui_cached_colour_table[NK_COLOR_SLIDER] = surface1;
		fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR] = green;
		fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR_HOVER] = green;
		fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = green;
		fck_ui_cached_colour_table[NK_COLOR_PROPERTY] = surface0;
		fck_ui_cached_colour_table[NK_COLOR_EDIT] = surface0;
		fck_ui_cached_colour_table[NK_COLOR_EDIT_CURSOR] = lavender;
		fck_ui_cached_colour_table[NK_COLOR_COMBO] = surface0;
		fck_ui_cached_colour_table[NK_COLOR_CHART] = surface0;
		fck_ui_cached_colour_table[NK_COLOR_CHART_COLOR] = lavender;
		fck_ui_cached_colour_table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = pink;
		fck_ui_cached_colour_table[NK_COLOR_SCROLLBAR] = surface0;
		fck_ui_cached_colour_table[NK_COLOR_SCROLLBAR_CURSOR] = overlay0;
		fck_ui_cached_colour_table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = lavender;
		fck_ui_cached_colour_table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = pink;
		fck_ui_cached_colour_table[NK_COLOR_TAB_HEADER] = surface0;
		fck_ui_cached_colour_table[NK_COLOR_KNOB] = fck_ui_cached_colour_table[NK_COLOR_SLIDER];
		fck_ui_cached_colour_table[NK_COLOR_KNOB_CURSOR] = pink;
		fck_ui_cached_colour_table[NK_COLOR_KNOB_CURSOR_HOVER] = pink;
		fck_ui_cached_colour_table[NK_COLOR_KNOB_CURSOR_ACTIVE] = pink;
		nk_style_from_table(ctx, fck_ui_cached_colour_table);
	}
	else
	{
		nk_style_default(ctx);
	}
	return fck_ui_cached_colour_table;
}