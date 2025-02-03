
#define NK_IMPLEMENTATION
#include "fck_ui.h"


#include "SDL3/sdl.h"
#include "core/fck_engine.h"
#include "ecs/fck_ecs.h"

/*
 * ==============================================================
 *
 *                          IMPLEMENTATION
 *
 * ===============================================================
 */
#include <stdlib.h>
#include <string.h>

struct nk_sdl_device
{
	struct nk_buffer cmds;
	struct nk_draw_null_texture tex_null;
	SDL_Texture *font_tex;
};

struct nk_sdl_vertex
{
	float position[2];
	float uv[2];
	float col[4];
};

struct nk_sdl
{
	struct nk_sdl_device ogl;
	struct nk_font_atlas atlas;
	struct nk_context ctx;
	Uint64 time_of_last_frame;
};


static void nk_sdl_device_upload_atlas(fck_ecs *ecs, const void *image, int width, int height)
{
	fck_ui *ui = fck_ecs_unique_view<fck_ui>(ecs);
	if (ui == nullptr)
	{
		return;
	}

	fck_engine *engine = fck_ecs_unique_view<fck_engine>(ecs);
	if (engine == nullptr)
	{
		return;
	}

	struct nk_sdl_device *dev = &ui->sdl->ogl;

	SDL_Texture *g_SDLFontTexture = SDL_CreateTexture(engine->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, width, height);
	if (g_SDLFontTexture == NULL)
	{
		SDL_Log("error creating texture");
		return;
	}
	SDL_UpdateTexture(g_SDLFontTexture, NULL, image, 4 * width);
	SDL_SetTextureBlendMode(g_SDLFontTexture, SDL_BLENDMODE_BLEND);
	dev->font_tex = g_SDLFontTexture;
}

static void nk_sdl_clipboard_paste(nk_handle usr, struct nk_text_edit *edit)
{
	const char *text = SDL_GetClipboardText();
	if (text)
		nk_textedit_paste(edit, text, nk_strlen(text));
	(void)usr;
}

static void nk_sdl_clipboard_copy(nk_handle usr, const char *text, int len)
{
	char *str = 0;
	(void)usr;
	if (!len)
		return;
	str = (char *)malloc((size_t)len + 1);
	if (!str)
		return;
	memcpy(str, text, (size_t)len);
	str[len] = '\0';
	SDL_SetClipboardText(str);
	free(str);
}

static void nk_sdl_font_stash_begin(fck_ecs *ecs, struct nk_font_atlas **atlas)
{
	fck_ui *ui = fck_ecs_unique_view<fck_ui>(ecs);
	if (ui == nullptr)
	{
		return;
	}
	nk_font_atlas_init_default(&ui->sdl->atlas);
	nk_font_atlas_begin(&ui->sdl->atlas);
	*atlas = &ui->sdl->atlas;
}

static void nk_sdl_font_stash_end(fck_ecs *ecs)
{
	fck_ui *ui = fck_ecs_unique_view<fck_ui>(ecs);
	if (ui == nullptr)
	{
		return;
	}
	const void *image;
	int w, h;
	image = nk_font_atlas_bake(&ui->sdl->atlas, &w, &h, NK_FONT_ATLAS_RGBA32);
	nk_sdl_device_upload_atlas(ecs, image, w, h);
	nk_font_atlas_end(&ui->sdl->atlas, nk_handle_ptr(ui->sdl->ogl.font_tex), &ui->sdl->ogl.tex_null);
	if (ui->sdl->atlas.default_font)
		nk_style_set_font(&ui->sdl->ctx, &ui->sdl->atlas.default_font->handle);
}

struct nk_context* fck_ui_init_internal(fck_ui* ui)
{
	ui->sdl = (nk_sdl*)SDL_malloc(sizeof(*ui->sdl));
	ui->sdl->time_of_last_frame = SDL_GetTicks();
	nk_init_default(&ui->sdl->ctx, 0);
	ui->sdl->ctx.clip.copy = nk_sdl_clipboard_copy;
	ui->sdl->ctx.clip.paste = nk_sdl_clipboard_paste;
	ui->sdl->ctx.clip.userdata = nk_handle_ptr(0);
	nk_buffer_init_default(&ui->sdl->ogl.cmds);
	return &ui->sdl->ctx;
}

void fck_ui_free(fck_ui* ui)
{
	struct nk_sdl_device* dev = &ui->sdl->ogl;
	nk_font_atlas_clear(&ui->sdl->atlas);
	nk_free(&ui->sdl->ctx);
	SDL_DestroyTexture(dev->font_tex);
	/* glDeleteTextures(1, &dev->font_tex); */
	nk_buffer_free(&dev->cmds);
	memset(&ui->sdl, 0, sizeof(ui->sdl));

	SDL_free(ui->sdl);
}

void fck_ui_handle_grab(fck_ecs *ecs)
{
	fck_ui *ui = fck_ecs_unique_view<fck_ui>(ecs);
	if (ui == nullptr)
	{
		return;
	}

	fck_engine *engine = fck_ecs_unique_view<fck_engine>(ecs);
	if (engine == nullptr)
	{
		return;
	}

	struct nk_context *ctx = &ui->sdl->ctx;
	if (ctx->input.mouse.grab)
	{
		SDL_SetWindowRelativeMouseMode(engine->window, true);
	}
	else if (ctx->input.mouse.ungrab)
	{
		/* better support for older SDL by setting mode first; causes an extra mouse motion event */
		SDL_SetWindowRelativeMouseMode(engine->window, false);
		SDL_WarpMouseInWindow(engine->window, (int)ctx->input.mouse.prev.x, (int)ctx->input.mouse.prev.y);
	}
	else if (ctx->input.mouse.grabbed)
	{
		ctx->input.mouse.pos.x = ctx->input.mouse.prev.x;
		ctx->input.mouse.pos.y = ctx->input.mouse.prev.y;
	}
}

int fck_ui_handle_event(fck_ecs *ecs, SDL_Event *evt)
{
	fck_ui *ui = fck_ecs_unique_view<fck_ui>(ecs);
	if (ui == nullptr)
	{
		return -1;
	}
	struct nk_context *ctx = &ui->sdl->ctx;

	switch (evt->type)
	{
	case SDL_EVENT_KEY_UP: /* KEYUP & KEYDOWN share same routine */
	case SDL_EVENT_KEY_DOWN: {
		int down = evt->type == SDL_EVENT_KEY_UP;
		const bool *state = SDL_GetKeyboardState(0);
		switch (evt->key.key)
		{
		case SDLK_RSHIFT: /* RSHIFT & LSHIFT share same routine */
		case SDLK_LSHIFT:
			nk_input_key(ctx, NK_KEY_SHIFT, down);
			break;
		case SDLK_DELETE:
			nk_input_key(ctx, NK_KEY_DEL, down);
			break;
		case SDLK_RETURN:
			nk_input_key(ctx, NK_KEY_ENTER, down);
			break;
		case SDLK_TAB:
			nk_input_key(ctx, NK_KEY_TAB, down);
			break;
		case SDLK_BACKSPACE:
			nk_input_key(ctx, NK_KEY_BACKSPACE, down);
			break;
		case SDLK_HOME:
			nk_input_key(ctx, NK_KEY_TEXT_START, down);
			nk_input_key(ctx, NK_KEY_SCROLL_START, down);
			break;
		case SDLK_END:
			nk_input_key(ctx, NK_KEY_TEXT_END, down);
			nk_input_key(ctx, NK_KEY_SCROLL_END, down);
			break;
		case SDLK_PAGEDOWN:
			nk_input_key(ctx, NK_KEY_SCROLL_DOWN, down);
			break;
		case SDLK_PAGEUP:
			nk_input_key(ctx, NK_KEY_SCROLL_UP, down);
			break;
		case SDLK_Z:
			nk_input_key(ctx, NK_KEY_TEXT_UNDO, down && state[SDL_SCANCODE_LCTRL]);
			break;
		case SDLK_R:
			nk_input_key(ctx, NK_KEY_TEXT_REDO, down && state[SDL_SCANCODE_LCTRL]);
			break;
		case SDLK_C:
			nk_input_key(ctx, NK_KEY_COPY, down && state[SDL_SCANCODE_LCTRL]);
			break;
		case SDLK_V:
			nk_input_key(ctx, NK_KEY_PASTE, down && state[SDL_SCANCODE_LCTRL]);
			break;
		case SDLK_X:
			nk_input_key(ctx, NK_KEY_CUT, down && state[SDL_SCANCODE_LCTRL]);
			break;
		case SDLK_B:
			nk_input_key(ctx, NK_KEY_TEXT_LINE_START, down && state[SDL_SCANCODE_LCTRL]);
			break;
		case SDLK_E:
			nk_input_key(ctx, NK_KEY_TEXT_LINE_END, down && state[SDL_SCANCODE_LCTRL]);
			break;
		case SDLK_UP:
			nk_input_key(ctx, NK_KEY_UP, down);
			break;
		case SDLK_DOWN:
			nk_input_key(ctx, NK_KEY_DOWN, down);
			break;
		case SDLK_LEFT:
			if (state[SDL_SCANCODE_LCTRL])
				nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT, down);
			else
				nk_input_key(ctx, NK_KEY_LEFT, down);
			break;
		case SDLK_RIGHT:
			if (state[SDL_SCANCODE_LCTRL])
				nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, down);
			else
				nk_input_key(ctx, NK_KEY_RIGHT, down);
			break;
		}
	}
		return 1;

	case SDL_EVENT_MOUSE_BUTTON_UP: /* MOUSEBUTTONUP & MOUSEBUTTONDOWN share same routine */
	case SDL_EVENT_MOUSE_BUTTON_DOWN: {
		int down = evt->type == SDL_EVENT_MOUSE_BUTTON_DOWN;
		const int x = evt->button.x, y = evt->button.y;
		switch (evt->button.button)
		{
		case SDL_BUTTON_LEFT:
			if (evt->button.clicks > 1)
				nk_input_button(ctx, NK_BUTTON_DOUBLE, x, y, down);
			nk_input_button(ctx, NK_BUTTON_LEFT, x, y, down);
			break;
		case SDL_BUTTON_MIDDLE:
			nk_input_button(ctx, NK_BUTTON_MIDDLE, x, y, down);
			break;
		case SDL_BUTTON_RIGHT:
			nk_input_button(ctx, NK_BUTTON_RIGHT, x, y, down);
			break;
		}
	}
		return 1;

	case SDL_EVENT_MOUSE_MOTION:
		if (ctx->input.mouse.grabbed)
		{
			int x = (int)ctx->input.mouse.prev.x, y = (int)ctx->input.mouse.prev.y;
			nk_input_motion(ctx, x + evt->motion.xrel, y + evt->motion.yrel);
		}
		else
			nk_input_motion(ctx, evt->motion.x, evt->motion.y);
		return 1;

	case SDL_EVENT_TEXT_INPUT: {
		nk_glyph glyph;
		memcpy(glyph, evt->text.text, NK_UTF_SIZE);
		nk_input_glyph(ctx, glyph);
	}
		return 1;

	case SDL_EVENT_MOUSE_WHEEL:
		nk_input_scroll(ctx, nk_vec2((float)evt->wheel.x, (float)evt->wheel.y));
		return 1;
	}
	return 0;
}

void fck_ui_setup(fck_ecs* ecs, fck_system_once_info*)
{
	float font_scale = 1;

	fck_ui* ui = fck_ecs_unique_create<fck_ui>(ecs, fck_ui_free);
	
	ui->ctx = fck_ui_init_internal(ui);

	{
		struct nk_font_atlas* atlas;
		struct nk_font_config config = nk_font_config(0);
		struct nk_font* font;

		/* set up the font atlas and add desired font; note that font sizes are
		 * multiplied by font_scale to produce better results at higher DPIs */
		nk_sdl_font_stash_begin(ecs, &atlas);
		font = nk_font_atlas_add_default(atlas, 13 * font_scale, &config);
		/*font = nk_font_atlas_add_from_file(atlas, "../../../extra_font/DroidSans.ttf", 14 * font_scale, &config);*/
		/*font = nk_font_atlas_add_from_file(atlas, "../../../extra_font/Roboto-Regular.ttf", 16 * font_scale, &config);*/
		/*font = nk_font_atlas_add_from_file(atlas, "../../../extra_font/kenvector_future_thin.ttf", 13 * font_scale, &config);*/
		/*font = nk_font_atlas_add_from_file(atlas, "../../../extra_font/ProggyClean.ttf", 12 * font_scale, &config);*/
		/*font = nk_font_atlas_add_from_file(atlas, "../../../extra_font/ProggyTiny.ttf", 10 * font_scale, &config);*/
		/*font = nk_font_atlas_add_from_file(atlas, "../../../extra_font/Cousine-Regular.ttf", 13 * font_scale, &config);*/
		nk_sdl_font_stash_end(ecs);

		/* this hack makes the font appear to be scaled down to the desired
		 * size and is only necessary when font_scale > 1 */
		font->handle.height /= font_scale;
		/*nk_style_load_all_cursors(ctx, atlas->cursors);*/
		nk_style_set_font(&ui->sdl->ctx, &font->handle);
	}
}

void fck_ui_render(struct fck_ecs* ecs, bool use_anti_aliasing)
{
	fck_ui* ui = fck_ecs_unique_view<fck_ui>(ecs);
	if (ui == nullptr)
	{
		return;
	}

	fck_engine* engine = fck_ecs_unique_view<fck_engine>(ecs);
	if (engine == nullptr)
	{
		return;
	}

	nk_anti_aliasing AA = use_anti_aliasing ? NK_ANTI_ALIASING_ON : NK_ANTI_ALIASING_OFF;

	/* setup global state */
	struct nk_sdl_device* dev = &ui->sdl->ogl;

	{
		SDL_Rect saved_clip;
#ifdef NK_SDL_CLAMP_CLIP_RECT
		SDL_Rect viewport;
#endif
		bool clipping_enabled;
		int vs = sizeof(struct nk_sdl_vertex);
		size_t vp = offsetof(struct nk_sdl_vertex, position);
		size_t vt = offsetof(struct nk_sdl_vertex, uv);
		size_t vc = offsetof(struct nk_sdl_vertex, col);

		/* convert from command queue into draw list and draw to screen */
		const struct nk_draw_command* cmd;
		const nk_draw_index* offset = NULL;
		struct nk_buffer vbuf, ebuf;

		/* fill converting configuration */
		struct nk_convert_config config;
		static const struct nk_draw_vertex_layout_element vertex_layout[] = {
			{NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_sdl_vertex, position)},
			{NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_sdl_vertex, uv)},
			{NK_VERTEX_COLOR, NK_FORMAT_R32G32B32A32_FLOAT, NK_OFFSETOF(struct nk_sdl_vertex, col)},
			{NK_VERTEX_LAYOUT_END} };

		Uint64 now = SDL_GetTicks();
		ui->sdl->ctx.delta_time_seconds = (float)(now - ui->sdl->time_of_last_frame) / 1000;
		ui->sdl->time_of_last_frame = now;

		NK_MEMSET(&config, 0, sizeof(config));
		config.vertex_layout = vertex_layout;
		config.vertex_size = sizeof(struct nk_sdl_vertex);
		config.vertex_alignment = NK_ALIGNOF(struct nk_sdl_vertex);
		config.tex_null = dev->tex_null;
		config.circle_segment_count = 22;
		config.curve_segment_count = 22;
		config.arc_segment_count = 22;
		config.global_alpha = 1.0f;
		config.shape_AA = AA;
		config.line_AA = AA;

		/* convert shapes into vertexes */
		nk_buffer_init_default(&vbuf);
		nk_buffer_init_default(&ebuf);
		nk_convert(&ui->sdl->ctx, &dev->cmds, &vbuf, &ebuf, &config);

		/* iterate over and execute each draw command */
		offset = (const nk_draw_index*)nk_buffer_memory_const(&ebuf);

		clipping_enabled = SDL_RenderClipEnabled(engine->renderer);
		SDL_GetRenderClipRect(engine->renderer, &saved_clip);
#ifdef NK_SDL_CLAMP_CLIP_RECT
		SDL_RenderGetViewport(sdl->renderer, &viewport);
#endif

		nk_draw_foreach(cmd, &ui->sdl->ctx, &dev->cmds)
		{
			if (!cmd->elem_count)
				continue;

			{
				SDL_Rect r;
				r.x = cmd->clip_rect.x;
				r.y = cmd->clip_rect.y;
				r.w = cmd->clip_rect.w;
				r.h = cmd->clip_rect.h;
#ifdef NK_SDL_CLAMP_CLIP_RECT
				if (r.x < 0)
				{
					r.w += r.x;
					r.x = 0;
				}
				if (r.y < 0)
				{
					r.h += r.y;
					r.y = 0;
				}
				if (r.h > viewport.h)
				{
					r.h = viewport.h;
				}
				if (r.w > viewport.w)
				{
					r.w = viewport.w;
				}
#endif
				SDL_GetRenderClipRect(engine->renderer, &r);
			}

			{
				const void* vertices = nk_buffer_memory_const(&vbuf);

			
				SDL_RenderGeometryRaw(engine->renderer, (SDL_Texture*)cmd->texture.ptr, (const float*)((const nk_byte*)vertices + vp),
					vs, (const SDL_FColor*)((const nk_byte*)vertices + vc), vs, (const float*)((const nk_byte*)vertices + vt), vs, (vbuf.needed / vs),
					(void*)offset, cmd->elem_count, 2);

				offset += cmd->elem_count;
			}
		}

		SDL_GetRenderClipRect(engine->renderer, &saved_clip);
		if (!clipping_enabled)
		{
			SDL_SetRenderClipRect(engine->renderer, NULL);
		}

		nk_clear(&ui->sdl->ctx);
		nk_buffer_clear(&dev->cmds);
		nk_buffer_free(&vbuf);
		nk_buffer_free(&ebuf);
	}
}
