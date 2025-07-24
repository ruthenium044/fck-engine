
#include "fck_ui_window_manager.h"

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_stdinc.h>

#include "fck_ui.h"
#include "fckc_inttypes.h"

#define FCK_UI_WINDOW_MANAGER_ASSERT(expr) SDL_assert(expr)
#define FCK_UI_WINDOW_MANAGER_ZERO_POINTER(x) SDL_zerop(x)
#define FCK_UI_WINDOW_MANAGER_MALLOC(size) SDL_malloc(size)
#define FCK_UI_WINDOW_MANAGER_FREE(ptr) SDL_free(ptr)

typedef struct fck_ui_window_manager
{
	fckc_size_t capacity;
	fckc_size_t count;

	fckc_u16 currently_editing;

	fck_ui_window user_windows[1];
} fck_ui_window_manager;

fck_ui_window_manager *fck_ui_window_manager_alloc(fckc_size_t capacity)
{
	fck_ui_window_manager *manager;
	fckc_size_t header_size = sizeof(*manager);
	fckc_size_t element_size = sizeof(manager->user_windows) * (capacity - 1);
	manager = (fck_ui_window_manager *)FCK_UI_WINDOW_MANAGER_MALLOC(header_size + element_size);
	manager->capacity = capacity;
	manager->currently_editing = 0;
	manager->count = 0;
	return manager;
}

void fck_ui_window_manager_free(fck_ui_window_manager *manager)
{
	FCK_UI_WINDOW_MANAGER_FREE(manager);
}

fck_ui_window *fck_ui_window_manager_create(fck_ui_window_manager *manager, const char *title, void *userdata,
                                            fck_ui_user_window_draw_content_function on_content)
{
	FCK_UI_WINDOW_MANAGER_ASSERT(manager->count < manager->capacity && "Limit reached");
	fckc_size_t index = manager->count;
	fck_ui_window *window = manager->user_windows + index;
	FCK_UI_WINDOW_MANAGER_ZERO_POINTER(window);

	manager->count = manager->count + 1;
	window->x = window->y = window->w = window->h = 0.0f;
	window->on_content = on_content;
	window->userdata = userdata;
	window->title = title;
	window->flags = NK_WINDOW_HIDDEN | NK_WINDOW_TITLE | NK_WINDOW_BORDER | NK_WINDOW_SCALABLE | NK_WINDOW_MOVABLE | NK_WINDOW_CLOSABLE;

	return window;
}

fck_ui_window *fck_ui_window_manager_view(fck_ui_window_manager *manager, fckc_size_t index)
{
	FCK_UI_WINDOW_MANAGER_ASSERT(index < manager->count);
	fck_ui_window *user = manager->user_windows + index;
	return user;
}

int fck_ui_window_toggle(fck_ui *ui, fck_ui_window *window)
{
	fck_ui_ctx *ctx = fck_ui_context(ui);

	if (window->flags & NK_WINDOW_HIDDEN)
	{
		window->flags = (window->flags & ~NK_WINDOW_HIDDEN);
		nk_window_show(ctx, window->title, NK_SHOWN);
		nk_window_set_focus(ctx, window->title);
		return 1;
	}
	else
	{
		nk_window_show(ctx, window->title, NK_HIDDEN);
		window->flags = (window->flags | NK_WINDOW_HIDDEN);
		return 0;
	}
}

void fck_ui_window_manager_hide_all(fck_ui *ui, fck_ui_window_manager *manager)
{
	fck_ui_ctx *ctx = fck_ui_context(ui);

	for (fckc_size_t index = 0; index < manager->count; index++)
	{
		fck_ui_window *window = &manager->user_windows[index];

		nk_window_show(ctx, window->title, NK_HIDDEN);
		window->flags = (window->flags | NK_WINDOW_HIDDEN);
	}
}

fckc_size_t fck_ui_window_manager_count(fck_ui_window_manager *manager)
{
	return manager->count;
}

fck_ui_window_manager_text_input_signal_type fck_ui_window_manager_query_text_input_signal(fck_ui *ui,
                                                                                           fck_ui_window_manager *window_manager)
{
	fck_ui_ctx *ctx = fck_ui_context(ui);

	int currently_editing = 0;
	for (fckc_size_t index = 0; index < window_manager->count; index++)
	{
		fck_ui_window *window = window_manager->user_windows + index;
		struct nk_window *win = nk_window_find(ctx, window->title);
		if (win != NULL && win->edit.active)
		{
			currently_editing = currently_editing + 1;
		}
	}
	fck_ui_window_manager_text_input_signal_type signal = FCK_UI_WINDOW_MANAGER_TEXT_INPUT_SIGNAL_NONE;
	if (currently_editing != 0 && window_manager->currently_editing == 0)
	{
		signal = FCK_UI_WINDOW_MANAGER_TEXT_INPUT_SIGNAL_START;
	}
	if (currently_editing == 0 && window_manager->currently_editing != 0)
	{
		signal = FCK_UI_WINDOW_MANAGER_TEXT_INPUT_SIGNAL_STOP;
	}

	window_manager->currently_editing = currently_editing;
	return signal;
}

void fck_ui_window_manager_header(fck_ui* ui, fck_ui_window_manager* manager, struct nk_rect* canvas_rect)
{
	// BIG TODO:
	fck_ui_ctx* ctx = fck_ui_context(ui);

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

void fck_ui_window_manager_footer(fck_ui* ui, fck_ui_window_manager* manager, struct nk_rect* canvas_rect)
{
	fck_ui_window_manager* window_manager = manager;
	fckc_size_t window_count = fck_ui_window_manager_count(window_manager);
	fck_ui_ctx* ctx = fck_ui_context(ui);

	const char footer_title[] = "fck_overlay_footer";

	if (nk_begin(ctx, footer_title, nk_rect(0, canvas_rect->h, canvas_rect->w, 40), NK_WINDOW_BORDER))
	{
		nk_menubar_begin(ctx);

		float height = nk_window_get_height(ctx) + nk_window_get_panel(ctx)->border;
		canvas_rect->h = canvas_rect->h - height;

		float ratios[16];
		ratios[0] = 50.0f;
		for (fckc_size_t index = 0; index < window_count; index++)
		{
			ratios[index + 1] = 140.0f;
		}

		nk_layout_row(ctx, NK_STATIC, 30, window_count + 1, ratios);

		if (nk_button_symbol(ctx, NK_SYMBOL_RECT_OUTLINE))
		{
			fck_ui_window_manager_hide_all(ui, window_manager);
		}

		for (fckc_size_t index = 0; index < window_count; index++)
		{
			fck_ui_window* window = fck_ui_window_manager_view(window_manager, index);
			const char* title = window->title;
			enum nk_symbol_type symbol = (window->flags & NK_WINDOW_HIDDEN) ? NK_SYMBOL_CIRCLE_OUTLINE : NK_SYMBOL_CIRCLE_SOLID;
			if (nk_button_symbol_label(ctx, symbol, title, NK_TEXT_ALIGN_LEFT))
			{
				if (fck_ui_window_toggle(ui, window))
				{
					nk_window_set_bounds(ctx, title, *canvas_rect);
				}
			}
		}

		nk_menubar_end(ctx);
	}

	nk_end(ctx);
}

void fck_ui_window_manager_tick(fck_ui* ui, fck_ui_window_manager* manager, int x, int y, int w, int h)
{
	fck_ui_ctx* ctx = fck_ui_context(ui);

	struct nk_rect canvas_rect = nk_rect(x, y, w, h);

	fck_ui_window_manager_header(ui, manager, &canvas_rect);
	fck_ui_window_manager_footer(ui, manager, &canvas_rect);

	for (fckc_size_t index = 0; index < manager->count; index++)
	{
		fck_ui_window* window = manager->user_windows + index;
		window->x = x;
		window->y = y;
		window->w = w;
		window->h = h;
		struct nk_rect rect = nk_rect(x, y, w, h);
		if (nk_begin(ctx, window->title, rect, window->flags))
		{
			struct nk_panel* panel = nk_window_get_panel(ctx);
			struct nk_rect title_bounds = panel->bounds;
			title_bounds.h = panel->header_height;
			title_bounds.y = title_bounds.y - title_bounds.h;
			const struct nk_mouse_button* btn = &ctx->input.mouse.buttons[NK_BUTTON_DOUBLE];
			if (btn->clicked && btn->down)
			{
				if (nk_input_is_mouse_hovering_rect(&ctx->input, title_bounds))
				{
					nk_window_set_bounds(ctx, window->title, rect);
				}
			}
			if (!window->on_content(ui, window, window->userdata))
			{
				// TODO?
			}
		}
		else
		{
			window->flags = (window->flags | NK_WINDOW_HIDDEN);
		}
		nk_end(ctx);
	}
}
