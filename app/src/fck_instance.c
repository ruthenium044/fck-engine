#include "fck_instance.h"

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_render.h>

#include <fck_hash.h>

#include "fck_ui.h"

#include "nk_style.c"

typedef enum fck_ui_flags
{
	FCK_UI_WINDOW_DOCKABLE = 1 << 0,
} fck_ui_flags;

typedef struct fck_ui_window
{
	struct nk_rect rect;
	nk_flags nk_flags;
	fck_ui_flags fck_flags;
	const char *title;
} fck_ui_window;

typedef int (*fck_ui_user_window_draw_content_function)(struct fck_ui *ui, fck_ui_window *window, void *userdata);

typedef struct fck_ui_user_window
{
	fck_ui_window window;
	void *userdata;
	fck_ui_user_window_draw_content_function on_content;
} fck_ui_user_window;

typedef struct fck_ui_user_window_handle
{
	fckc_size_t index;
} fck_ui_user_window_handle;

typedef struct fck_ui_window_manager
{
	fckc_size_t capacity;
	fckc_size_t count;

	fckc_u16 currently_editing;

	fck_ui_user_window user_windows[1];
} fck_ui_window_manager;

fck_ui_window_manager *fck_ui_window_manager_alloc(fckc_size_t capacity)
{
	fck_ui_window_manager *manager;
	fckc_size_t header_size = sizeof(*manager);
	fckc_size_t element_size = sizeof(manager->user_windows) * (capacity - 1);
	manager = (fck_ui_window_manager *)SDL_malloc(header_size + element_size);
	manager->capacity = capacity;
	manager->currently_editing = 0;
	manager->count = 0;
	return manager;
}

void fck_ui_window_manager_free(fck_ui_window_manager *manager)
{
	SDL_free(manager);
}

fck_ui_user_window_handle fck_ui_window_manager_create(fck_ui_window_manager *manager, const char *title, void *userdata,
                                                       fck_ui_user_window_draw_content_function on_content)
{
	SDL_assert(manager->count < manager->capacity && "Limit reached");
	fckc_size_t index = manager->count;
	fck_ui_user_window *user = manager->user_windows + index;
	SDL_zerop(user);

	manager->count = manager->count + 1;
	user->on_content = on_content;
	user->userdata = userdata;
	user->window.rect = nk_rect(0.0f, 0.0f, 0.0f, 0.0f);
	user->window.title = title;
	user->window.nk_flags =
		NK_WINDOW_HIDDEN | NK_WINDOW_TITLE | NK_WINDOW_BORDER | NK_WINDOW_SCALABLE | NK_WINDOW_MOVABLE | NK_WINDOW_CLOSABLE;

	return (fck_ui_user_window_handle){index};
}

fck_ui_user_window *fck_ui_window_manager_view(fck_ui_window_manager *manager, fck_ui_user_window_handle handle)
{
	SDL_assert(handle.index < manager->count);
	fck_ui_user_window *user = manager->user_windows + handle.index;
	return user;
}

int fck_ui_window_toggle(struct fck_ui *ui, fck_ui_window *window)
{
	fck_ui_ctx *ctx = fck_ui_context(ui);

	if (window->nk_flags & NK_WINDOW_HIDDEN)
	{
		window->nk_flags = (window->nk_flags & ~NK_WINDOW_HIDDEN);
		nk_window_show(ctx, window->title, NK_SHOWN);
		nk_window_set_focus(ctx, window->title);
		return 1;
	}
	else
	{
		nk_window_set_focus(ctx, window->title);
		// nk_window_show(ctx, window->title, NK_HIDDEN);
		// window->nk_flags = (window->nk_flags | NK_WINDOW_HIDDEN);
		return 0;
	}
}

void fck_ui_window_manager_hide_all(struct fck_ui *ui, fck_ui_window_manager *manager)
{
	fck_ui_ctx *ctx = fck_ui_context(ui);

	for (fckc_size_t index = 0; index < manager->count; index++)
	{
		fck_ui_window *window = &manager->user_windows[index].window;

		nk_window_show(ctx, window->title, NK_HIDDEN);
		window->nk_flags = (window->nk_flags | NK_WINDOW_HIDDEN);
	}
}

void fck_ui_window_manager_tick(struct fck_ui *ui, fck_ui_window_manager *manager, int x, int y, int w, int h)
{
	fck_ui_ctx *ctx = fck_ui_context(ui);

	for (fckc_size_t index = 0; index < manager->count; index++)
	{
		fck_ui_user_window *user_window = manager->user_windows + index;
		fck_ui_window *window = &user_window->window;
		window->rect.x = x;
		window->rect.y = y;
		window->rect.w = w;
		window->rect.h = h;
		if (nk_begin(ctx, window->title, window->rect, window->nk_flags))
		{
			struct nk_panel *panel = nk_window_get_panel(ctx);
			struct nk_rect title_bounds = panel->bounds;
			title_bounds.h = panel->header_height;
			title_bounds.y = title_bounds.y - title_bounds.h;
			const struct nk_mouse_button *btn = &ctx->input.mouse.buttons[NK_BUTTON_DOUBLE];
			if (btn->clicked && btn->down)
			{
				if (nk_input_is_mouse_hovering_rect(&ctx->input, title_bounds))
				{
					nk_window_set_bounds(ctx, window->title, window->rect);
				}
			}
			if (!user_window->on_content(ui, window, user_window->userdata))
			{
			}
		}
		else
		{
			window->nk_flags = (window->nk_flags | NK_WINDOW_HIDDEN);
		}
		nk_end(ctx);
	}
}

nk_bool fck_ui_window_begin(struct fck_ui *ui, fck_ui_window *window, const char *title)
{
	fck_ui_ctx *ctx = fck_ui_context(ui);
	return nk_begin(ctx, title, window->rect, window->nk_flags);
}

void fck_ui_window_handle_flag(fck_ui_ctx *ctx, fck_ui_window *window, const char *title, enum nk_panel_flags flag)
{
	char buffer[128];
	int result = SDL_snprintf(buffer, sizeof(buffer), "%s - %s", title, (window->nk_flags & flag) == flag ? "ON" : "OFF");
	SDL_assert(result <= sizeof(buffer));
	if (nk_button_label(ctx, buffer))
	{
		window->nk_flags = window->nk_flags ^ flag;
	}
}

int fck_ui_window_content(struct fck_ui *ui, fck_ui_window *window, void *userdata)
{
	static float value = 0.6f;
	static int i = 20;

	fck_ui_ctx *ctx = fck_ui_context(ui);

	nk_layout_row_dynamic(ctx, 30, 2);

	fck_ui_window_handle_flag(ctx, window, "NK_WINDOW_BORDER", NK_WINDOW_BORDER);
	fck_ui_window_handle_flag(ctx, window, "NK_WINDOW_MOVABLE", NK_WINDOW_MOVABLE);
	fck_ui_window_handle_flag(ctx, window, "NK_WINDOW_SCALABLE", NK_WINDOW_SCALABLE);
	fck_ui_window_handle_flag(ctx, window, "NK_WINDOW_CLOSABLE", NK_WINDOW_CLOSABLE);
	fck_ui_window_handle_flag(ctx, window, "NK_WINDOW_MINIMIZABLE", NK_WINDOW_MINIMIZABLE);
	fck_ui_window_handle_flag(ctx, window, "NK_WINDOW_NO_SCROLLBAR", NK_WINDOW_NO_SCROLLBAR);
	fck_ui_window_handle_flag(ctx, window, "NK_WINDOW_TITLE", NK_WINDOW_TITLE);
	fck_ui_window_handle_flag(ctx, window, "NK_WINDOW_SCROLL_AUTO_HIDE", NK_WINDOW_SCROLL_AUTO_HIDE);
	fck_ui_window_handle_flag(ctx, window, "NK_WINDOW_BACKGROUND", NK_WINDOW_BACKGROUND);
	fck_ui_window_handle_flag(ctx, window, "NK_WINDOW_SCALE_LEFT", NK_WINDOW_SCALE_LEFT);

	/* custom widget pixel width */
	nk_layout_row_begin(ctx, NK_STATIC, 30, 2);
	{
		nk_layout_row_push(ctx, 50);
		nk_label(ctx, "Volume:", NK_TEXT_LEFT);
		nk_layout_row_push(ctx, 110);
		nk_slider_float(ctx, 0, &value, 1.0f, 0.1f);
	}
	nk_layout_row_end(ctx);
	return 0;
}

int fck_ui_window_content2(struct fck_ui *ui, fck_ui_window *window, void *userdata)
{
	static float value = 0.6f;
	static int i = 20;

	fck_ui_ctx *ctx = fck_ui_context(ui);

	nk_layout_row_dynamic(ctx, 30, 2);

	/* custom widget pixel width */
	nk_layout_row_begin(ctx, NK_STATIC, 30, 2);
	{
		nk_layout_row_push(ctx, 50);
		nk_label(ctx, "Volume:", NK_TEXT_LEFT);
		nk_layout_row_push(ctx, 110);
		nk_slider_float(ctx, 0, &value, 1.0f, 0.1f);
	}
	nk_layout_row_end(ctx);
	return 0;
}

int fck_ui_window_overview(struct fck_ui *ui, fck_ui_window *window, void *userdata)
{
	overview(fck_ui_context(ui));
	return 0;
}

void fck_ui_window_end(struct fck_ui *ui, fck_ui_window *window)
{
	fck_ui_ctx *ctx = fck_ui_context(ui);
	nk_end(ctx);
}

void fck_instance_control_text_input(fck_instance *instance)
{
	fck_ui_ctx *ctx = fck_ui_context(instance->ui);
	fck_ui_window_manager *window_manager = instance->window_manager;

	int currently_editing = 0;
	for (fckc_size_t index = 0; index < window_manager->count; index++)
	{
		fck_ui_user_window *user_window = window_manager->user_windows + index;
		fck_ui_window *window = &user_window->window;
		struct nk_window *win = nk_window_find(ctx, window->title);
		if (win != NULL && win->edit.active)
		{
			currently_editing = currently_editing + 1;
		}
	}
	if (currently_editing != 0 && window_manager->currently_editing == 0)
	{
		SDL_StartTextInput(instance->window);
	}
	if (currently_editing == 0 && window_manager->currently_editing != 0)
	{
		SDL_StopTextInput(instance->window);
	}

	window_manager->currently_editing = currently_editing;
}

static void fck_overlay_header(fck_instance *instance, struct nk_rect *canvas_rect)
{
	fck_ui_ctx *ctx = fck_ui_context(instance->ui);

	if (nk_begin(ctx, "fck_overlay_header", nk_rect(0, 0, canvas_rect->w, 40), NK_WINDOW_BORDER))
	{
		float height = nk_window_get_height(ctx) + nk_window_get_panel(ctx)->border;
		canvas_rect->h = canvas_rect->h - height;
		canvas_rect->y = canvas_rect->y + height;

		nk_menubar_begin(ctx);

		nk_layout_row_static(ctx, 30, 100, 2);

		if (nk_menu_begin_label(ctx, "File", NK_TEXT_LEFT, nk_vec2(100, 200)))
		{
			nk_layout_row_dynamic(ctx, 25, 1);
			if (nk_menu_item_label(ctx, "New", NK_TEXT_LEFT))
			{
			}
			if (nk_menu_item_label(ctx, "Open", NK_TEXT_LEFT))
			{
			}
			if (nk_menu_item_label(ctx, "Save", NK_TEXT_LEFT))
			{
			}
			nk_menu_end(ctx);
		}
		if (nk_menu_begin_label(ctx, "Edit", NK_TEXT_LEFT, nk_vec2(100, 200)))
		{
			nk_layout_row_dynamic(ctx, 25, 1);
			if (nk_menu_item_label(ctx, "Undo", NK_TEXT_LEFT))
			{
			}
			if (nk_menu_item_label(ctx, "Redo", NK_TEXT_LEFT))
			{
			}
			nk_menu_end(ctx);
		}

		nk_menubar_end(ctx);
	}
	nk_end(ctx);
}

static void fck_overlay_footer(fck_instance *instance, struct nk_rect *canvas_rect)
{
	fck_ui_window_manager *window_manager = instance->window_manager;
	fck_ui_ctx *ctx = fck_ui_context(instance->ui);

	const char footer_title[] = "fck_overlay_footer";

	if (nk_begin(ctx, footer_title, nk_rect(0, canvas_rect->h, canvas_rect->w, 40), NK_WINDOW_BORDER))
	{
		nk_menubar_begin(ctx);

		float height = nk_window_get_height(ctx) + nk_window_get_panel(ctx)->border;
		canvas_rect->h = canvas_rect->h - height;

		float ratios[16];
		ratios[0] = 50.0f;
		for (fckc_size_t index = 0; index < window_manager->count; index++)
		{
			ratios[index + 1] = 140.0f;
		}

		nk_layout_row(ctx, NK_STATIC, 30, window_manager->count + 1, ratios);

		if (nk_button_symbol(ctx, NK_SYMBOL_RECT_OUTLINE))
		{
			fck_ui_window_manager_hide_all(instance->ui, window_manager);
		}

		for (fckc_size_t index = 0; index < window_manager->count; index++)
		{
			fck_ui_user_window *user_window = fck_ui_window_manager_view(window_manager, (fck_ui_user_window_handle){index});
			fck_ui_window *window = &user_window->window;
			const char *title = window->title;
			enum nk_symbol_type symbol = (window->nk_flags & NK_WINDOW_HIDDEN) ? NK_SYMBOL_CIRCLE_OUTLINE : NK_SYMBOL_CIRCLE_SOLID;
			if (nk_button_symbol_label(ctx, symbol, title, NK_TEXT_ALIGN_LEFT))
			{
				if (fck_ui_window_toggle(instance->ui, window))
				{
					nk_window_set_bounds(ctx, title, *canvas_rect);
				}
			}
		}

		nk_menubar_end(ctx);
	}

	nk_end(ctx);
}

fck_instance_result fck_instance_overlay(fck_instance *instance)
{
	int width;
	int height;

	if (!SDL_GetWindowSize(instance->window, &width, &height))
	{
		return FCK_INSTANCE_FAILURE;
	}
	struct nk_rect canvas_rect = nk_rect(0, 0, width, height);

	fck_overlay_header(instance, &canvas_rect);
	fck_overlay_footer(instance, &canvas_rect);

	fck_ui_window_manager *window_manager = instance->window_manager;
	fck_ui_window_manager_tick(instance->ui, window_manager, canvas_rect.x, canvas_rect.y, canvas_rect.w, canvas_rect.h);

	fck_instance_control_text_input(instance);

	return FCK_INSTANCE_CONTINUE;
}

fck_instance *fck_instance_alloc(const char *title, int with, int height, SDL_WindowFlags window_flags, const char *renderer_name)
{
	fck_instance *app = (fck_instance *)SDL_malloc(sizeof(fck_instance));
	app->window = SDL_CreateWindow(title, 1920, 1080, SDL_WINDOW_RESIZABLE);
	app->renderer = SDL_CreateRenderer(app->window, renderer_name);
	app->ui = fck_ui_alloc(app->renderer);
	app->window_manager = fck_ui_window_manager_alloc(16);

	fck_ui_window_manager_create(app->window_manager, "Nk Overview", NULL, fck_ui_window_overview);
	fck_ui_window_manager_create(app->window_manager, "World", NULL, fck_ui_window_content);
	fck_ui_window_manager_create(app->window_manager, "Network", NULL, fck_ui_window_content2);
	fck_ui_window_manager_create(app->window_manager, "Graphics", NULL, fck_ui_window_content);
	fck_ui_window_manager_create(app->window_manager, "Physics", NULL, fck_ui_window_content2);

	set_style(fck_ui_context(app->ui), THEME_DRACULA);

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
