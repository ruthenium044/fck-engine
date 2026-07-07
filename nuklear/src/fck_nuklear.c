// Static configration for nuklear
// Static configration for nuklear

#include "fck_nuklear.h"

#include <fck_os.h>
#include <fckc_assert.h>
#include <fckc_inttypes.h>

#include <fck_apis.h>
#include <fck_hash.h>
#include <fck_input.h>
#include <fck_mouse.h>
#include <fck_pkey.h>
#include <fck_shader.h>
#include <kll.h>
#include <kll_malloc.h>
#include <sht_render.h>

#include "fck_gfx.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_KEYSTATE_BASED_INPUT
#define NK_UINT_DRAW_INDEX
#define NK_IMPLEMENTATION
#include "fckc_math.h"
#include "nuklear.h"

static fck_api_registry *apis;

typedef struct fck_nk_screen
{
	float width;
	float height;
} fck_nk_screen;

typedef struct fck_nk_vertex
{
	float x;
	float y;
	float u;
	float v;
	float r;
	float g;
	float b;
	float a;
} fck_nk_vertex;

typedef struct fck_nk_hamburger
{
	fck_nk_hamburger_item *items;
	fck_nk_hamburger_item *items_last;
} fck_nk_hamburger;

typedef struct fck_nk_pie
{
	float x;
	float y;

	fck_nk_pie_item root;

	fck_nk_pie_item *hovered;

	fckc_u32 active;
} fck_nk_pie;

typedef struct fck_nk_selection
{
	// TODO: We need to be able to "click through"
	const void *pointer;
	struct nk_rect rect;

	fckc_u32 index;
} fck_nk_selection;

typedef struct fck_nk_hovered
{
	const void *last;
	fckc_u32 track;
	fckc_u32 count;
} fck_nk_hovered;

typedef struct fck_nk_control_point
{
	const void *current;
	const void *last_hovered;
	struct nk_vec2 offset;
} fck_nk_control_point;

// TODO: Revisit this and make it better... Not now though
typedef struct fck_nk_control_state
{
	fck_nk_control_point point;

	fck_nk_hovered hovered;
	fck_nk_selection selection;
} fck_nk_control_state;

struct fck_nk_panel_item;
typedef struct fck_nk_panel_item
{
	fck_hash_int hash;
	char name[420];

	struct fck_nk_panel_item *next;
} fck_nk_panel_item;

typedef struct fck_nk_panel_state
{
	// TODO: Over-engineer this
	fck_nk_panel_item *root;
	fck_nk_panel_item *active;

	fck_nk_panel_item items[32];
	fckc_size_t items_count;

	nk_bool open;
} fck_nk_panel_state;

typedef enum fck_nk_os_group_type
{
	fck_nk_os_group_panel,
	fck_nk_os_group_canvas,
	fck_nk_os_group_count,
} fck_nk_os_group_type;

typedef struct fck_nk_os_window
{
	fck_window window;
	fck_nk_hamburger burger;
	fck_nk_pie pie;
	fck_nk_control control;

	fck_nk_panel_state panel;

	fck_nk_control_state control_state;
} fck_nk_os_window;

typedef struct fck_nk_private
{
	fck_nk_os_window os;

	sht_driver driver;

	struct nk_font_atlas atlas;
	struct nk_font *default_font;
	struct nk_draw_null_texture null_texture;

	struct nk_buffer commands;
	struct nk_context *ctx;

	sht_buffer indices[sht_frame_count];
	sht_sampler sampler;
	sht_image font_image;
	sht_image_view font_view;
	fck_gfx gfx;

	fckc_u64 time_last_frame;
} fck_nk_private;

static fck_nk_panel_item *fck_nk_panel_state_find(fck_nk_panel_state *state, const char *name)
{
	const fckc_size_t len = strlen(name);
	const fck_hash_int hash = fck_hash(name, len);
	for (fckc_size_t index = 0; index < fck_arraysize(state->items); index++)
	{
		const fck_hash_int slot = (slot + index) % fck_arraysize(state->items);
		fck_nk_panel_item *item = state->items + slot;
		if (item->hash == 0)
		{
			item->hash = hash;
			memcpy(item->name, name, len);
			item->name[len] = 0;

			if (state->root == NULL)
			{
				state->root = item;
			}
			else
			{
				fck_nk_panel_item *current = state->root;
				for (;;)
				{
					if (current->next == NULL)
					{
						current->next = item;
						break;
					}
				}
			}

			return item;
		}

		if (item->hash == hash && strcmp(item->name, name) == 0)
		{
			return item;
		}
	}
	return NULL;
}

static sht_image fck_nk_bake_font(sht_driver driver, const void *pixels, sht_format format, int width, int height)
{
	sht_memory *memory = driver.vt->memory(driver);
	const sht_image_configuration config = {
		.format = format,
		.height = to_u32(height),
		.width = to_u32(width),
		.transfer = sht_transfer_target,
		.usage = sht_image_usage_sampled,
	};

	sht_image image = memory->image->create(memory->bump, &config, sht_memory_gpu);
	if (!memory->image->is_ok(&image))
	{
		return image;
	}
	const fckc_size_t size = (fckc_size_t)width * height * 4;
	driver.vt->upload_image(driver, &image, pixels, size);
	return image;
}

static fck_nk_os_window fck_nk_os_window_create(fck_window window)
{
	fck_nk_os_window os_window = {.window = window};
	return os_window;
}

static fck_nk fck_nk_api_create(kll_allocator *allocator, fck_window *window, sht_driver *driver)
{
	sht_memory *memory = driver->vt->memory(*driver);

	fck_nk_private *nk;
	const fckc_size_t offset = fckc_align(sizeof(*nk), alignof(struct nk_context));
	nk = (fck_nk_private *)kll_malloc(allocator, offset + sizeof(*nk->ctx));
	nk->ctx = (struct nk_context *)fckc_pointer_add(nk, offset);
	// OS feature set for the window
	nk->os = fck_nk_os_window_create(*window);
	nk->driver = *driver;

	sht_buffer_configuration config = sht_buffer_retained(sht_buffer_usage_index, fck_megabytes(1));
	for (fckc_size_t index = 0; index < fck_arraysize(nk->indices); index++)
	{
		nk->indices[index] = memory->malloc(memory->bump, &config, sht_memory_cpu);
	}

	nk->sampler = driver->vt->create_sampler(*driver, sht_filter_linear);

	nk_init_default(nk->ctx, &nk->default_font->handle);
	nk->ctx->clip.userdata = nk_handle_ptr(0);

	nk_font_atlas_init_default(&nk->atlas);
	nk_font_atlas_begin(&nk->atlas);

	const struct nk_font_config font_config = nk_font_config(0);
	nk->default_font = nk_font_atlas_add_default(&nk->atlas, 13, &font_config);

	int default_font_width = 0;
	int default_font_height = 0;

	const void *default_font_pixels = nk_font_atlas_bake(&nk->atlas, &default_font_width, &default_font_height, NK_FONT_ATLAS_RGBA32);

	const sht_format font_format = sht_format_r8g8b8a8_unorm;
	nk->font_image = fck_nk_bake_font(*driver, default_font_pixels, font_format, default_font_width, default_font_height);
	nk->font_view = memory->image->view(memory->bump, nk->font_image, font_format);

	nk_font_atlas_end(&nk->atlas, nk_handle_ptr(&nk->font_view), &nk->null_texture);

	nk_style_set_font(nk->ctx, &nk->default_font->handle);

	nk_buffer_init_default(&nk->commands);

	const fck_gfx_shader vertex = {
		.name = "nuklear-vertex",
		.path = fck_nuklear_resource_path "vertex.vert",
	};

	const fck_gfx_shader fragment = {
		.name = "nuklear-fragment",
		.path = fck_nuklear_resource_path "fragment.frag",
	};

	const fck_gfx_create_info create_info = {
		.vertex = &vertex,
		.fragment = &fragment,
	};

	fck_gfx_api *gfx = (fck_gfx_api *)apis->find(fck_gfx_api_name);

	nk->gfx = gfx->create(kll->system, driver, &create_info);
	return (fck_nk){.handle = nk};
}

static void fck_nk_api_input(fck_nk nk, fck_input *input)
{
	fck_input_source *mouse = NULL;
	fck_input_source *keyboard = NULL;
	{
		fck_input_source **sources;
		const fckc_size_t count = input->sources(&sources);
		for (fckc_size_t index = 0; index < count; index++)
		{
			fck_input_source *source = sources[index];
			if (mouse == NULL && source->type == fck_input_source_mouse)
			{
				mouse = source;
			}
			if (keyboard == NULL && source->type == fck_input_source_keyboard)
			{
				keyboard = source;
			}
		}
	}

	// TODO: Text input...

	fck_nk_private *nki = (fck_nk_private *)nk.handle;
	nk_input_begin(nki->ctx);
	{
		fckc_u32 ids[] = {fck_mouse_left, fck_mouse_middle, fck_mouse_right, fck_mouse_position, fck_mouse_wheel};
		fck_input_data states[fck_arraysize(ids)];
		const fckc_size_t result = mouse->states(0, ids, states, fck_arraysize(ids));
		const fck_input_data *left = states + 0;
		const fck_input_data *middle = states + 1;
		const fck_input_data *right = states + 2;
		const fck_input_data *position = states + 3;
		const fck_input_data *wheel = states + 4;
		nk_input_button(nki->ctx, NK_BUTTON_LEFT, nki->ctx->input.mouse.pos.x, nki->ctx->input.mouse.pos.y, left->scalar > 0.0f);
		nk_input_button(nki->ctx, NK_BUTTON_MIDDLE, position->floats[0], position->floats[1], middle->scalar > 0.0f);
		nk_input_button(nki->ctx, NK_BUTTON_RIGHT, position->floats[0], position->floats[1], right->scalar > 0.0f);
		nk_input_button(nki->ctx, NK_BUTTON_RIGHT, position->floats[0], position->floats[1], right->scalar > 0.0f);
		nk_input_scroll(nki->ctx, nk_vec2(wheel->floats[0], wheel->floats[1]));
		nk_input_motion(nki->ctx, position->floats[0], position->floats[1]);
	}
	nk_input_end(nki->ctx);
}

static int fck_nk_api_begin(fck_nk nke)
{
	fck_nk_private *nk = (fck_nk_private *)nke.handle;

	struct nk_context *ctx = nk->ctx;
	fck_nk_os_window *os_window = &nk->os;

	const fck_window window = os_window->window;
	fck_nk_hamburger_item *menu_items = os_window->burger.items;

	const char *title = os->win->title(window, NULL);

	int window_width, window_height;
	os->win->size(window, &window_width, &window_height);

	const fck_window_configuration config = {
		.title_bar_height = 35.0f,
		.resize_line_width = 4.0f,
		.menu_area_width = 35.0f * 2.0f,
		.button_area_width = 35.0f * 2.0f,
	};
	os->win->configuration(window, &config);

	const fck_window_configuration *configuration = os->win->configuration(window, NULL);

	nk_style_push_style_item(ctx, &ctx->style.window.fixed_background, ctx->style.button.normal);
	nk_style_push_vec2(ctx, &ctx->style.window.padding, nk_vec2(0, 0));
	nk_style_push_vec2(ctx, &ctx->style.window.group_padding, nk_vec2(0, 0));
	nk_style_push_vec2(ctx, &ctx->style.window.spacing, nk_vec2(0, 0));

	os_window->control.menu = 0;
	os_window->control.close = 0;
	os_window->control.minimise = 0;

	if (nk_begin(ctx, "Window Header", nk_rect(0, 0, window_width, configuration->title_bar_height),
	             NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BACKGROUND))
	{
		nk_style_push_float(ctx, &ctx->style.button.rounding, 0.0f);
		nk_style_push_float(ctx, &ctx->style.button.border, 0.0f);

		const float menu_button_width = configuration->menu_area_width / 2.0f;
		const float button_width = configuration->button_area_width / 2.0f;
		nk_layout_row_template_begin(ctx, configuration->title_bar_height);
		nk_layout_row_template_push_static(ctx, menu_button_width);
		nk_layout_row_template_push_static(ctx, menu_button_width);
		nk_layout_row_template_push_dynamic(ctx);
		nk_layout_row_template_push_static(ctx, button_width);
		nk_layout_row_template_push_static(ctx, button_width);
		nk_layout_row_template_end(ctx);

		// Pretend the menu button is a normal button
		const struct nk_style_button menu_button_style = ctx->style.menu_button;
		ctx->style.menu_button = ctx->style.button;

		nk_style_push_vec2(ctx, &ctx->style.menu_button.padding, nk_vec2(7.5f, 10.0f));
		if (nk_menu_begin_symbol(ctx, "Window Header Menu", NK_SYMBOL_HAMBURGER, nk_vec2(120, 200)))
		{
			nk_layout_row_dynamic(ctx, 25, 1);
			fck_nk_hamburger_item *current = menu_items;
			while (current)
			{
				switch (current->type)
				{
				case fck_nk_hamburger_item_bool: {

					const struct nk_style_button contextual_button_style = ctx->style.contextual_button;
					if (current->value)
					{
						ctx->style.contextual_button = ctx->style.menu_button;
					}
					if (nk_menu_item_label(ctx, current->name, NK_TEXT_LEFT))
					{
						current->value = !current->value;
					}
					ctx->style.contextual_button = contextual_button_style;
				}
				break;
				case fck_nk_hamburger_item_button:
					current->value = to_int(nk_menu_item_label(ctx, current->name, NK_TEXT_LEFT));
					break;
				}
				current = current->next;
			}
			os_window->control.menu = 1;
			nk_menu_end(ctx);
		}
		nk_style_pop_vec2(ctx);

		nk_style_push_vec2(ctx, &ctx->style.button.padding, nk_vec2(12.5f, 12.5f));
		const enum nk_symbol_type symbol = os_window->control.body ? NK_SYMBOL_TRIANGLE_DOWN : NK_SYMBOL_TRIANGLE_UP_OUTLINE;
		if (nk_button_symbol(ctx, symbol))
		{
			os_window->control.body = ~os_window->control.body;
		}
		nk_style_pop_vec2(ctx);

		ctx->style.menu_button = menu_button_style;

		nk_label(ctx, title, NK_TEXT_ALIGN_CENTERED | NK_TEXT_ALIGN_MIDDLE);

		if (nk_button_symbol(ctx, NK_SYMBOL_MINUS))
		{
			os_window->control.minimise = 1;
		}

		if (nk_button_symbol(ctx, NK_SYMBOL_X))
		{
			os_window->control.close = 1;
		}
		nk_style_pop_float(ctx);
		nk_style_pop_float(ctx);

		nk_end(ctx);
	}
	nk_style_pop_vec2(ctx);
	nk_style_pop_vec2(ctx);
	nk_style_pop_vec2(ctx);
	nk_style_pop_style_item(ctx);

	const int height = window_height - configuration->title_bar_height;
	if (os_window->control.body)
	{
		if (os_window->control.menu)
		{
			// NOTE: This resets the input so it does not interfere with the body's nk_window
			nk_input_begin(ctx);
			nk_input_end(ctx);
		}
		const nk_flags body_flags = NK_WINDOW_BACKGROUND | NK_WINDOW_NO_SCROLLBAR;
		if (nk_begin(ctx, "Window Body", nk_rect(0, configuration->title_bar_height, window_width, height), body_flags))
		{
			return os_window->control.body;
		}
		// We should not end up here
	}
	return os_window->control.body;
}

static fck_nk_pie_item *fck_nk_pie_item_is_part_of(fck_nk_pie_item *item, fck_nk_pie_item *target)
{
	fck_nk_pie_item *current = item;
	while (current)
	{
		if (current == target)
		{
			return current;
		}
		current = current->parent;
	}
	return NULL;
}

static fck_nk_pie_item *fck_nk_pie_fan(struct nk_context *ctx, fck_nk_pie *pie, fck_nk_pie_item *active, fck_nk_pie_item *pie_items,
                                       struct nk_vec2 center, float offset_angle, float angle_step, float radius_offset, float radius)
{
	int count = 0;
	fck_nk_pie_item *current = pie_items;
	while (current)
	{
		count = count + 1;
		current = current->next;
	}

	struct nk_input *input = &ctx->input;
	const struct nk_vec2 mouse_pos = input->mouse.pos;

	const float dx = mouse_pos.x - center.x;
	const float dy = mouse_pos.y - center.y;

	const float dist = sqrtf(dx * dx + dy * dy);
	int hovered = -1;

	fck_nk_pie_item *current_hovered = NULL;

	angle_step = angle_step / (float)count;
	if (dist > radius_offset && dist < radius)
	{
		float mouse_angle = nk_atan2(dy, dx);
		mouse_angle = mouse_angle - offset_angle;
		if (mouse_angle < 0.0f)
		{
			mouse_angle += 2.0f * NK_PI;
		}
		hovered = (int)(mouse_angle / angle_step);
	}

	int index = 0;
	current = pie_items;
	while (current)
	{
		const float start_angle = offset_angle + ((float)index * angle_step);
		struct nk_color slice_color = nk_rgba(45, 45, 45, 230);

		if (hovered == -1)
		{
			if (fck_nk_pie_item_is_part_of(pie->hovered, current))
			{
				struct fck_nk_pie_item *children = current->items;
				if (children)
				{
					struct fck_nk_pie_item *result =
						fck_nk_pie_fan(ctx, pie, pie->hovered, children, center, start_angle, angle_step, radius, radius * 1.25f);
					if (result)
					{
						current_hovered = result;
					}
				}
			}
		}

		if (index == hovered)
		{
			current_hovered = current;
			slice_color = nk_rgba(0, 150, 255, 255);
			struct fck_nk_pie_item *children = current->items;
			if (children)
			{
				// Preview Children
				fck_nk_pie_fan(ctx, pie, active, children, center, start_angle, angle_step, radius, radius * 1.25f);
			}
		}

		struct nk_command_buffer *canvas = nk_window_get_canvas(ctx);
		nk_fill_arc(canvas, center.x, center.y, radius, start_angle, start_angle + angle_step, slice_color);
		nk_stroke_arc(canvas, center.x, center.y, radius, start_angle, start_angle + angle_step, 1.5f, nk_rgba(100, 100, 100, 255));

		const float text_angle = start_angle + (angle_step / 2.0f);
		const struct nk_user_font *font = ctx->style.font;
		struct nk_vec2 text_pos;
		text_pos.x = center.x + (((radius_offset * 0.35f) + (radius * 0.65f)) * nk_cos(text_angle));
		text_pos.y = center.y + (((radius_offset * 0.35f) + (radius * 0.65f)) * nk_sin(text_angle));

		const float text_width = font->width(font->userdata, font->height, current->name, nk_strlen(current->name));
		text_pos.x -= text_width / 2.0f;
		text_pos.y -= font->height / 2.0f;

		nk_draw_text(canvas, nk_rect(text_pos.x, text_pos.y, text_width, font->height), current->name, nk_strlen(current->name), font,
		             nk_rgba(0, 0, 0, 0), nk_rgba(255, 255, 255, 255));

		index = index + 1;
		current = current->next;
	}

	return current_hovered;
}

static const fck_nk_pie_item *fck_nk_pie_execute(fck_nk nk, float radius)
{
	fck_nk_private *nk_internal = (fck_nk_private *)nk.handle;
	fck_nk_pie *pie = &nk_internal->os.pie;

	struct nk_context *ctx = nk_internal->ctx;
	struct nk_command_buffer *canvas = nk_window_get_canvas(ctx);
	struct nk_input *input = &ctx->input;

	if (!nk_input_is_mouse_released(input, NK_BUTTON_RIGHT))
	{
		if (!nk_input_is_mouse_down(input, NK_BUTTON_RIGHT))
		{
			fck_nk_pie_item *selected = pie->hovered;
			if (selected)
			{
				selected->value = 1;
			}
			pie->hovered = NULL;
			pie->active = 0;
			return selected;
		}
	}

	if (pie->active == 0)
	{
		pie->x = input->mouse.pos.x;
		pie->y = input->mouse.pos.y;
	}
	pie->active = 1;

	const struct nk_vec2 center = nk_vec2(pie->x, pie->y);
	fck_nk_pie_item *pie_items = pie->root.items;
	if (pie_items == NULL)
	{
		return NULL;
	}

	int count = 0;
	fck_nk_pie_item *current = pie_items;
	while (current)
	{
		count = count + 1;
		current = current->next;
	}

	const float angle_step = (2.0f * NK_PI) / (float)count;
	const struct nk_vec2 mouse_pos = input->mouse.pos;

	const float dx = mouse_pos.x - center.x;
	const float dy = mouse_pos.y - center.y;
	const float distance = sqrtf(dx * dx + dy * dy);

	int hovered = -1;
	if (distance > 15.0f && distance < radius)
	{
		float mouse_angle = nk_atan2(dy, dx);
		if (mouse_angle < 0.0f)
		{
			mouse_angle += 2.0f * NK_PI;
		}
		hovered = (int)((mouse_angle) / angle_step) % count;
	}

	fck_nk_pie_item *selected = NULL;
	fck_nk_pie_item *current_hovered = NULL;
	current = pie_items;
	int index = 0;
	while (current)
	{
		const float start_angle = ((float)index * angle_step);
		if (hovered == -1)
		{
			if (fck_nk_pie_item_is_part_of(pie->hovered, current))
			{
				struct fck_nk_pie_item *children = current->items;
				if (children)
				{
					struct fck_nk_pie_item *result =
						fck_nk_pie_fan(ctx, pie, pie->hovered, children, center, start_angle, angle_step, radius, radius * 1.5f);
					if (result)
					{
						current_hovered = result;
					}
				}
			}
		}

		struct nk_color slice_color = nk_rgba(45, 45, 45, 230);
		if (index == hovered)
		{
			slice_color = nk_rgba(0, 150, 255, 255);
			struct fck_nk_pie_item *children = current->items;
			if (children)
			{
				// Preview Children
				fck_nk_pie_fan(ctx, pie, current, children, center, start_angle, angle_step, radius, radius * 1.5f);
			}
			current_hovered = current;
		}

		{
			const float text_angle = start_angle + (angle_step / 2.0f);
			const struct nk_user_font *font = ctx->style.font;
			struct nk_vec2 text_pos;
			text_pos.x = center.x + (radius * 0.65f) * nk_cos(text_angle);
			text_pos.y = center.y + (radius * 0.65f) * nk_sin(text_angle);

			nk_fill_arc(canvas, center.x, center.y, radius, start_angle, start_angle + angle_step, slice_color);
			nk_stroke_arc(canvas, center.x, center.y, radius, start_angle, start_angle + angle_step, 1.5f, nk_rgba(100, 100, 100, 255));

			const float text_width = font->width(font->userdata, font->height, current->name, nk_strlen(current->name));
			text_pos.x -= text_width / 2.0f;
			text_pos.y -= font->height / 2.0f;

			nk_draw_text(canvas, nk_rect(text_pos.x, text_pos.y, text_width, font->height), current->name, nk_strlen(current->name), font,
			             nk_rgba(0, 0, 0, 0), nk_rgba(255, 255, 255, 255));
		}
		index = index + 1;
		current = current->next;
	}

	pie->hovered = current_hovered;

	nk_fill_circle(canvas, nk_rect(center.x - 15.0f, center.y - 15.0f, 30.0f, 30.0f), nk_rgba(30, 30, 30, 255));
	nk_stroke_circle(canvas, nk_rect(center.x - 15.0f, center.y - 15.0f, 30.0f, 30.0f), 1.5f, nk_rgba(100, 100, 100, 255));
	return selected;
}

static void fck_nk_api_end(fck_nk nke)
{
	fck_nk_private *nk = (fck_nk_private *)nke.handle;
	fck_nk_selection *selection = &nk->os.control_state.selection;
	fck_nk_hovered *hovered = &nk->os.control_state.hovered;
	fck_nk_control_point *point = &nk->os.control_state.point;

	const struct nk_input *input = &nk->ctx->input;
	if (nk->os.control_state.point.current)
	{
		struct nk_rect *target = &selection->rect;
		target->x = input->mouse.pos.x - (target->w * 0.5f);
		target->y = input->mouse.pos.y - (target->h * 0.5f);
	}

	if (nk->os.control.body)
	{
		if (input->mouse.pos.x > nk->ctx->current->layout->max_x)
		{
			if (nk_input_is_mouse_pressed(input, NK_BUTTON_LEFT))
			{
				if (hovered->last == NULL)
				{
					selection->pointer = NULL;
				}
				else
				{
					if (point->last_hovered == NULL)
					{
						selection->index = selection->index + 1;
					}
				}
			}
		}

		if (!nk_input_is_mouse_down(input, NK_BUTTON_LEFT))
		{
			point->current = NULL;
		}

		fck_nk_pie_execute(nke, 125.0f);

		nk_end(nk->ctx);

		if (hovered->count != hovered->track)
		{
			selection->index = to_u32(0LLU);
		}

		hovered->count = hovered->track;
		hovered->track = 0;
	}

	if (selection->pointer == NULL)
	{
		selection->rect = nk_rect(0, 0, 0, 0);
		selection->index = to_u32(0LLU);
	}

	point->last_hovered = NULL;
	hovered->last = NULL;
}

static void fck_nk_api_present(fck_nk nke, const struct sht_command_buffer *buffer, fckc_u32 frame_index)
{
	fck_nk_private *nk = (fck_nk_private *)nke.handle;

	const fckc_u64 now = os->chrono->ms();
	nk->ctx->delta_time_seconds = (float)(now - nk->time_last_frame) / 1000;
	nk->time_last_frame = now;

	struct sht_driver *driver = &nk->driver;

	sht_command_buffer_vt *command = driver->vt->command_buffer;

	static const struct nk_draw_vertex_layout_element vertex_layout[] = {
		{NK_VERTEX_POSITION, NK_FORMAT_FLOAT, offsetof(struct fck_nk_vertex, x)},
		{NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, offsetof(struct fck_nk_vertex, u)},
		{NK_VERTEX_COLOR, NK_FORMAT_R32G32B32A32_FLOAT, offsetof(struct fck_nk_vertex, r)},
		{NK_VERTEX_LAYOUT_END}};

	struct nk_convert_config config;

	memset(&config, 0, sizeof(config));
	config.vertex_layout = vertex_layout;
	config.vertex_size = sizeof(fck_nk_vertex);
	config.vertex_alignment = alignof(fck_nk_vertex);
	config.tex_null = nk->null_texture;
	config.circle_segment_count = 36;
	config.curve_segment_count = 36;
	config.arc_segment_count = 36;
	config.global_alpha = 1.0f;
	config.shape_AA = NK_ANTI_ALIASING_ON;
	config.line_AA = NK_ANTI_ALIASING_ON;

	struct nk_buffer vertices, elements;
	nk_buffer_init_default(&vertices);

	const sht_buffer index_buffer = nk->indices[frame_index];
	nk_buffer_init_fixed(&elements, index_buffer.cpu, index_buffer.size);

	const nk_flags result = nk_convert(nk->ctx, &nk->commands, &vertices, &elements, &config);
	if (sht_test(result, NK_CONVERT_COMMAND_BUFFER_FULL))
	{
		os->io->log("Nuklear GUI Command Buffer full!");
	}
	if (sht_test(result, NK_CONVERT_VERTEX_BUFFER_FULL))
	{
		os->io->log("Nuklear GUI Vertex Buffer full!");
	}
	if (sht_test(result, NK_CONVERT_ELEMENT_BUFFER_FULL))
	{
		os->io->log("Nuklear GUI Element Buffer full!");
	}

	const fck_nk_vertex *vertex_transforms = (const fck_nk_vertex *)nk_buffer_memory_const(&vertices);

	const sht_swapchain swapchain = driver->vt->swapchain(*driver);
	const sht_extent extent = swapchain.vt->extent(swapchain);

	{
		const fck_nk_screen screen = {
			.width = (float)extent.width,
			.height = (float)extent.height,
		};
		const sht_buffer_upload_desc screen_upload = {
			.data = &screen,
			.size = sizeof(screen),
			.count = 1,
		};

		const sht_buffer_upload_desc vertices_upload = {
			.data = vertex_transforms,
			.size = sizeof(*vertex_transforms),
			.count = vertices.needed / sizeof(*vertex_transforms),
		};

		fck_gfx_api *gfx = (fck_gfx_api *)apis->find(fck_gfx_api_name);

		sht_bss *bss = gfx->bss(nk->gfx);
		driver->vt->bss->upload_buffer(*bss, 0, &screen_upload);
		driver->vt->bss->upload_buffer(*bss, 1, &vertices_upload);
		// driver->vt->bss->upload_image(*bss, 2, &font_upload);
		command->bss(*buffer, *bss);

		sht_graphics_pipeline *pipeline = gfx->pipeline(nk->gfx);

		command->graphics_pipeline(*buffer, *pipeline);
		// Dynamic geometry updates indices and vertices each frame
		command->index_buffer(*buffer, &nk->indices[frame_index], 0);

		const struct nk_draw_command *cmd;
		fckc_u32 index_offset = 0;

		nk_draw_foreach(cmd, nk->ctx, &nk->commands)
		{
			if (!cmd->elem_count && !cmd->texture.ptr)
				continue;
			fck_assert(cmd->texture.ptr);

			const sht_image_upload_desc font_upload = {
				.samplers = nk->sampler,
				.views = *(sht_image_view *)cmd->texture.ptr,
			};

			driver->vt->bss->upload_image(*bss, 2, &font_upload);
			command->bss(*buffer, *bss);
			// driver->vt->bss->upload(ui->bss, 1, sht_upload_params{.view =
			// ui->font.view, .sampler = ui->sampler});

			sht_scissor scissor;
			scissor.offset.x = to_u32(NK_MAX(cmd->clip_rect.x, 0.f));
			scissor.offset.y = to_u32(NK_MAX(cmd->clip_rect.y, 0.f));
			scissor.extent.width = to_u32(cmd->clip_rect.w);
			scissor.extent.height = to_u32(cmd->clip_rect.h);

			// float x = cmd->clip_rect.x;
			// float y = cmd->clip_rect.y;
			command->scissor(*buffer, &scissor);

			const sht_draw_indexed_desc desc = {
				.first_index = index_offset,
				.index_count = cmd->elem_count,
				.instance_count = 1,
				.first_instance = 0,
				.vertex_offset = 0,
			};
			command->draw_indexed(*buffer, &desc);
			index_offset = index_offset + cmd->elem_count;
		}

		nk_buffer_free(&vertices);
		nk_buffer_free(&elements);
	}
}

static void fck_nk_hamburger_push(fck_nk_hamburger *burger, fck_nk_hamburger_item *menu_item)
{
	fck_assert(menu_item->next == NULL);

	if (burger->items == NULL)
	{
		burger->items = menu_item;
		burger->items_last = menu_item;
	}
	else
	{
		burger->items_last->next = menu_item;
		burger->items_last = menu_item;
	}
}

static fck_nk_hamburger_item *fck_nk_hamburger_api_push(fck_nk nk, fck_nk_hamburger_item *item)
{
	fck_assert(item->next == NULL);
	fck_nk_private *nk_internal = (fck_nk_private *)nk.handle;

	fck_nk_hamburger_push(&nk_internal->os.burger, item);
	return item;
}

static void fck_nk_input_api_begin(fck_nk nk)
{
	fck_nk_private *nk_internal = (fck_nk_private *)nk.handle;
	nk_input_begin(nk_internal->ctx);
}

static void fck_nk_input_api_events(fck_nk nke, const fck_input_event *const events, fckc_size_t count)
{
	fck_nk_private *nk = (fck_nk_private *)nke.handle;
	for (fckc_size_t index = 0; index < count; index++)
	{
		const fck_input_event *const e = events + index;
		if (e->source->type == fck_input_source_keyboard)
		{
			switch (e->description->id)
			{
			case fck_pkey_return:
				nk_input_key(nk->ctx, NK_KEY_ENTER, e->data.scalar > 0.0f);
				break;
			default:
				break;
			}
			continue;
		}

		if (e->source->type == fck_input_source_mouse)
		{
			switch (e->description->id)
			{
			case fck_mouse_left:
				nk_input_button(nk->ctx, NK_BUTTON_LEFT, nk->ctx->input.mouse.pos.x, nk->ctx->input.mouse.pos.y, e->data.scalar > 0.0f);
				break;
			case fck_mouse_middle:
				nk_input_button(nk->ctx, NK_BUTTON_MIDDLE, nk->ctx->input.mouse.pos.x, nk->ctx->input.mouse.pos.y, e->data.scalar > 0.0f);
				break;
			case fck_mouse_right:
				nk_input_button(nk->ctx, NK_BUTTON_RIGHT, nk->ctx->input.mouse.pos.x, nk->ctx->input.mouse.pos.y, e->data.scalar > 0.0f);
				break;
			case fck_mouse_position:
				nk_input_motion(nk->ctx, e->data.floats[0], e->data.floats[1]);
				break;
			case fck_mouse_wheel:
				nk_input_scroll(nk->ctx, nk_vec2(e->data.floats[0], e->data.floats[1]));
				break;
			default:
				break;
			}
			continue;
		}

		if (e->source->type == fck_input_source_text)
		{
			nk_input_unicode(nk->ctx, e->data.unicode);
			continue;
		}
	}
}

static struct nk_color *fck_ui_set_style(struct nk_context *ctx, enum fck_nuklear_theme theme);

static void fck_nk_api_theme(fck_nk nk, fck_nuklear_theme theme)
{
	fck_nk_private *nk_internal = (fck_nk_private *)nk.handle;
	fck_ui_set_style(nk_internal->ctx, theme);
}

static int fck_nk_api_to_screen(fck_nk nk, float *x, float *y)
{
	fck_nk_private *nk_internal = (fck_nk_private *)nk.handle;

	int window_width = 0;
	int window_height = 0;
	os->win->size(nk_internal->os.window, &window_width, &window_height);

	const float ox = (window_width * 0.5f);
	const float oy = (window_height * 0.5f);

	const float px = *x - ox;
	const float py = *y - oy;

	*x = px;
	*y = py;
	return 1;
}

static int fck_nk_api_to_nuklear(fck_nk nk, float *x, float *y)
{
	fck_nk_private *nk_internal = (fck_nk_private *)nk.handle;

	int window_width = 0;
	int window_height = 0;
	os->win->size(nk_internal->os.window, &window_width, &window_height);

	const float ox = (window_width * 0.5f);
	const float oy = (window_height * 0.5f);

	const float px = *x + ox;
	const float py = *y + oy;

	*x = px;
	*y = py;
	return 1;
}

static int fck_nk_api_legacy_control_point(fck_nk nk, const void *pointer, float *x, float *y, float size, fck_nk_colour on, fck_nk_colour off)
{
	// Since this nuklear implementation moves everything around, it is on said implementation to fix it
	fck_nk_private *nk_internal = (fck_nk_private *)nk.handle;

	int window_width = 0;
	int window_height = 0;
	os->win->size(nk_internal->os.window, &window_width, &window_height);

	float px = *x - (size * 0.5f);
	float py = *y - (size * 0.5f);
	fck_nk_api_to_nuklear(nk, &px, &py);

	struct nk_rect r = nk_rect(px, py, size, size);

	const struct nk_input *input = &nk_internal->ctx->input;

	const float scaled_size = size * 2.0f;
	float spx = *x - (scaled_size * 0.5f);
	float spy = *y - (scaled_size * 0.5f);
	fck_nk_api_to_nuklear(nk, &spx, &spy);

	const struct nk_rect sr = nk_rect(spx, spy, scaled_size, scaled_size);

	const fck_nk_colour *select = &off;
	if (nk_input_is_mouse_hovering_rect(input, sr) || NK_INBOX(input->mouse.prev.x, input->mouse.prev.y, sr.x, sr.y, sr.w, sr.h))
	{
		nk_internal->os.control_state.point.last_hovered = pointer;

		if (nk_internal->os.control_state.point.current == NULL)
		{
			r.x = spx;
			r.y = spy;
			r.w = r.h = scaled_size;
		}

		if (nk_input_is_mouse_down(input, NK_BUTTON_LEFT))
		{
			if ((nk_internal->os.control_state.point.current == NULL || nk_internal->os.control_state.point.current == pointer))
			{
				nk_internal->os.control_state.point.current = pointer;

				select = &on;
				*x = input->mouse.pos.x;
				*y = input->mouse.pos.y;
				fck_nk_api_to_screen(nk, x, y);
			}
		}
	}

	const struct nk_color c = nk_rgba(select->r, select->g, select->b, select->a);
	struct nk_command_buffer *canvas = nk_window_get_canvas(nk_internal->ctx);
	if (nk_internal->os.control_state.point.current == NULL || nk_internal->os.control_state.point.current == pointer)
	{
		nk_stroke_rect(canvas, sr, 0.0f, 2.0f, c);
	}
	nk_fill_rect(canvas, r, 0.0f, c);
	return select == &on;
}

static int fck_nk_api_control_point(fck_nk nk, const void *pointer, float *x, float *y, float w, float h)
{
	// Since this nuklear implementation moves everything around, it is on said implementation to fix it
	fck_nk_private *nk_internal = (fck_nk_private *)nk.handle;

	int window_width = 0;
	int window_height = 0;
	os->win->size(nk_internal->os.window, &window_width, &window_height);

	float px = *x;
	float py = *y;
	fck_nk_api_to_nuklear(nk, &px, &py);

	struct nk_rect rect = nk_rect(px - (w * 0.5f), py - (h * 0.5f), w, h);

	const struct nk_input *input = &nk_internal->ctx->input;

	int select = 0;
	if (nk_input_is_mouse_hovering_rect(input, rect) || NK_INBOX(input->mouse.prev.x, input->mouse.prev.y, rect.x, rect.y, rect.w, rect.h))
	{
		nk_internal->os.control_state.point.last_hovered = pointer;

		if (nk_input_is_mouse_down(input, NK_BUTTON_LEFT))
		{
			if (nk_internal->os.control_state.point.current == NULL)
			{
				nk_internal->os.control_state.point.offset.x = (input->mouse.pos.x - px);
				nk_internal->os.control_state.point.offset.y = (input->mouse.pos.y - py);
			}
			if ((nk_internal->os.control_state.point.current == NULL || nk_internal->os.control_state.point.current == pointer))
			{
				nk_internal->os.control_state.point.current = pointer;

				select = 1;
				*x = input->mouse.pos.x - nk_internal->os.control_state.point.offset.x;
				*y = input->mouse.pos.y - nk_internal->os.control_state.point.offset.y;
				fck_nk_api_to_screen(nk, x, y);
			}
		}
	}

	return select == 1;
}

static void fck_nk_api_set_select(fck_nk nk, const void *pointer)
{
	fck_nk_private *nk_internal = (fck_nk_private *)nk.handle;
	nk_internal->os.control_state.selection.pointer = pointer;
	// TODO: Invalidate the rect?
}

static int fck_nk_api_select(fck_nk nk, const void *pointer, float x, float y, float w, float h, fck_nk_colour on)
{
	fck_nk_private *nk_internal = (fck_nk_private *)nk.handle;

	int window_width = 0;
	int window_height = 0;
	os->win->size(nk_internal->os.window, &window_width, &window_height);

	fck_nk_api_to_nuklear(nk, &x, &y);

	const struct nk_input *input = &nk_internal->ctx->input;

	const struct nk_rect rect = nk_rect(x - (w * 0.5f), y - (h * 0.5f), w, h);
	const struct nk_color colour = nk_rgba(on.r, on.g, on.b, on.a);
	struct nk_command_buffer *canvas = nk_window_get_canvas(nk_internal->ctx);

	fck_nk_hovered *hovered = &nk_internal->os.control_state.hovered;
	fck_nk_selection *selection = &nk_internal->os.control_state.selection;
	fck_nk_control_point *point = &nk_internal->os.control_state.point;

	if (nk_input_is_mouse_hovering_rect(input, rect))
	{
		const fckc_u32 self = hovered->track;
		hovered->last = pointer;
		hovered->track = hovered->track + 1;
		if (point->current == NULL)
		{
			if (hovered->count != 0)
			{
				const fckc_u32 selected = selection->index % hovered->count;
				const int is_not_selected = selection->pointer != pointer;
				if (is_not_selected)
				{
					nk_stroke_rect(canvas, rect, 0.0f, 1.0f, colour);

					if (self == selected && nk_input_is_mouse_pressed(input, NK_BUTTON_LEFT))
					{
						selection->pointer = pointer;
						selection->rect = rect;
					}
				}
			}
		}
		else
		{
			if (point->current == pointer)
			{
				selection->pointer = pointer;
				selection->rect = rect;
			}
		}
	}

	if (pointer == selection->pointer)
	{
		float minX = rect.x;
		float maxX = rect.x + rect.w;
		float minY = rect.y;
		float maxY = rect.y + rect.h;
		float offset = 6.0f;
		float dash = 6.0f;
		float fullStep = offset + dash;

		float current = minX;
		float line_thickness = 2.0f;
		while (current < maxX)
		{
			float from = fck_clamp(current, minX, maxX);
			float to = fck_clamp( current + dash, minX, maxX);
			nk_stroke_line(canvas, from, minY, to, minY, line_thickness, colour);
			nk_stroke_line(canvas, from, maxY, to, maxY, line_thickness, colour);
			current += fullStep;
		}
		current = minY;
		while (current < maxY)
		{
			float from = fck_clamp(current, minY, maxY);
			float to = fck_clamp(current + dash, minY, maxY);
			nk_stroke_line(canvas, minX, from, minX, to, line_thickness, colour);
			nk_stroke_line(canvas, maxX, from, maxX, to, line_thickness, colour);
			current += fullStep;
		}

		selection->rect = rect;
		return 1;
	}

	return 0;
}

static void fck_nk_panel_api_begin(fck_nk nk, const char *name, float width)
{
	fck_nk_private *nk_internal = (fck_nk_private *)nk.handle;
	fck_nk_panel_state *state = &nk_internal->os.panel;

	struct nk_context *ctx = nk_internal->ctx;

	const struct nk_vec2 size = nk_window_get_size(ctx);

	// This might benefit from ACTUALLY being a window...
	// But then the API is tough cause we cannot start TWO windows and then END two windows :/

	// const float tab_size = 48.0f;
	// const float widths[] = {/*tab_size,*/ width, size.x - width};
	// widths[fck_nk_os_group_panel] = width;
	// widths[fck_nk_os_group_canvas] = size.x - width;
	nk_layout_row_static(ctx, size.y, width, 1);
	// Lazily Add
	// fck_nk_panel_item *item = fck_nk_panel_state_find(state, name);

	// if (nk_group_begin(ctx, item->name, NK_WINDOW_BORDER))
	//{
	//	nk_layout_row_static(ctx, tab_size, tab_size, 1);
	//	fck_nk_panel_item *current = state->root;
	//	while (current)
	//	{
	//		nk_button_label(ctx, current->name);
	//		current = current->next;
	//	}
	//	nk_group_end(ctx);
	// }
	state->open = nk_group_begin(ctx, name, NK_WINDOW_BORDER);
}

static void fck_nk_panel_api_end(fck_nk nk)
{
	fck_nk_private *nk_internal = (fck_nk_private *)nk.handle;
	struct nk_context *ctx = nk_internal->ctx;
	if (nk_internal->os.panel.open)
	{
		nk_group_end(ctx);
	}
}

static int fck_nk_panel_menu_api_push(fck_nk nk, const char *fmt, ...)
{
	fck_nk_private *nk_internal = (fck_nk_private *)nk.handle;
	struct nk_context *ctx = nk_internal->ctx;

	char buffer[256]; /* Adjust this size if you expect massive names */
	int unique_id;
	va_list args;

	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);

	return nk_tree_push_hashed(ctx, NK_TREE_TAB, buffer, NK_MINIMIZED, NULL, 0, 0);
}

static void fck_nk_panel_menu_api_pop(fck_nk nk)
{
	fck_nk_private *nk_internal = (fck_nk_private *)nk.handle;
	struct nk_context *ctx = nk_internal->ctx;
	nk_tree_pop(ctx);
}

static int fck_nk_elements_api_dropdown(fck_nk nk, int selected, const char *const *items, int count)
{
	fck_nk_private *nk_internal = (fck_nk_private *)nk.handle;
	struct nk_context *ctx = nk_internal->ctx;
	// const struct nk_vec2 position = nk_widget_position(ctx);
	const struct nk_vec2 size = nk_widget_size(ctx);
	const struct nk_vec2 dropdown_size = nk_vec2(size.x, 128.0f);
	return nk_combo(ctx, items, count, selected, 25, dropdown_size);
}

static fckc_f32 fck_nuklear_elements_api_f32(fck_nk nk, const char *name, fckc_f32 min, fckc_f32 val, fckc_f32 max, fckc_f32 step)
{
	fck_nk_private *nk_internal = (fck_nk_private *)nk.handle;
	struct nk_context *ctx = nk_internal->ctx;
	return nk_propertyf(ctx, name, min, val, max, step, 0.5f);
}

static fckc_i32 fck_nuklear_elements_api_i32(fck_nk nk, const char *name, fckc_i32 min, fckc_i32 val, fckc_i32 max, fckc_i32 step)
{
	fck_nk_private *nk_internal = (fck_nk_private *)nk.handle;
	struct nk_context *ctx = nk_internal->ctx;
	return nk_propertyi(ctx, name, min, val, max, step, 0.5f);
}

static int fck_nuklear_elements_api_button(fck_nk nk, const char *title)
{
	fck_nk_private *nk_internal = (fck_nk_private *)nk.handle;
	struct nk_context *ctx = nk_internal->ctx;
	return nk_button_label(ctx, title);
}

static fck_nk_control fck_nk_api_control(fck_nk nk)
{
	fck_nk_private *nk_internal = (fck_nk_private *)nk.handle;
	return nk_internal->os.control;
}

static void fck_nk_input_api_end(fck_nk nk)
{
	fck_nk_private *nk_internal = (fck_nk_private *)nk.handle;

	nk_input_end(nk_internal->ctx);
	nk_clear(nk_internal->ctx);
	nk_buffer_clear(&nk_internal->commands);
}

static fck_nk_pie_item *fck_nk_pie_api_root(fck_nk nk)
{
	fck_nk_private *nk_internal = (fck_nk_private *)nk.handle;
	return &nk_internal->os.pie.root;
}

static void fck_nk_pie_api_add_child(fck_nk_pie_item *item, fck_nk_pie_item *child)
{
	fck_assert(child->next == NULL);

	child->parent = item;

	if (item->items == NULL)
	{
		item->items = child;
		item->items_last = child;
	}
	else
	{
		child->prev = item->items_last;
		item->items_last->next = child;
		item->items_last = child;
	}
}

static fck_nk_pie_item *fck_nk_pie_api_push(fck_nk nk, fck_nk_pie_item *item)
{
	fck_assert(item->next == NULL);
	fck_nk_private *nk_internal = (fck_nk_private *)nk.handle;
	fck_nk_pie *pie = &nk_internal->os.pie;
	fck_nk_pie_item *root = &pie->root;
	fck_nk_pie_api_add_child(root, item);
	return item;
}

static int fck_nk_pie_api_used(fck_nk nk, fck_nk_pie_item *item)
{
	const fck_nk_private *nk_internal = (fck_nk_private *)nk.handle;
	const fck_nk_pie *pie = &nk_internal->os.pie;
	const fck_nk_pie_item *root = &pie->root;

	const fck_nk_pie_item *current = item->parent;
	while (current)
	{
		if (current == root)
		{
			return 1;
		}
		current = current->parent;
	}
	return 0;
}

static void fck_nk_pie_api_remove(fck_nk nk, fck_nk_pie_item *item)
{
	fck_nk_private *nk_internal = (fck_nk_private *)nk.handle;
	fck_nk_pie *pie = &nk_internal->os.pie;

	fck_nk_pie_item *root = &pie->root;
	fck_assert(item->parent);

	if (item->parent)
	{
		// Remove from pie
		if (item->parent->items == item)
		{
			item->parent->items = item->next;
		}
		if (item->parent->items_last == item)
		{
			item->parent->items_last = item->prev;
		}
	}
	if (item->prev != NULL)
	{
		item->prev->next = item->next;
	}
	if (item->next != NULL)
	{
		item->next->prev = item->prev;
	}

	item->parent = NULL;
	item->next = NULL;
	item->prev = NULL;
}

static void fck_nk_pie_api_apply_position(fck_nk nk, float *x, float *y)
{
	fck_nk_private *nk_internal = (fck_nk_private *)nk.handle;
	*x = nk_internal->os.pie.x;
	*y = nk_internal->os.pie.y;

	fck_nk_api_to_screen(nk, x, y);
}

static int fck_nk_pie_api_happened(fck_nk_pie_item *item)
{
	const int value = item->value;
	item->value = 0;
	return value;
}

static fck_nuklear_hamburger_api nuklear_hamburger_api = {
	.push = fck_nk_hamburger_api_push,
};

static fck_nuklear_pie_api nuklear_pie_api = {
	.push = fck_nk_pie_api_add_child,
	.root = fck_nk_pie_api_root,
	.remove = fck_nk_pie_api_remove,
	.used = fck_nk_pie_api_used,
	.apply_position = fck_nk_pie_api_apply_position,
	.happened = fck_nk_pie_api_happened,
};

static fck_nuklear_input_api nuklear_input_api = {
	.begin = fck_nk_input_api_begin,
	.end = fck_nk_input_api_end,
	.events = fck_nk_input_api_events,
};

static fck_nuklear_panel_api nuklear_panel_api = {
	.begin = fck_nk_panel_api_begin,
	.end = fck_nk_panel_api_end,
	.push = fck_nk_panel_menu_api_push,
	.pop = fck_nk_panel_menu_api_pop,
};

static fck_nuklear_elements_api nuklear_property_api = {
	.f32 = fck_nuklear_elements_api_f32,
	.i32 = fck_nuklear_elements_api_i32,
	.button = fck_nuklear_elements_api_button,
	.dropdown = fck_nk_elements_api_dropdown,
};

static fck_nuklear_api nuklear_api = {
	.begin = fck_nk_api_begin,
	.end = fck_nk_api_end,
	.create = fck_nk_api_create,
	.present = fck_nk_api_present,
	.theme = fck_nk_api_theme,
	.legacy_control_point = fck_nk_api_legacy_control_point,
	.control_point = fck_nk_api_control_point,
	.set_selection = fck_nk_api_set_select,
	.select = fck_nk_api_select,
	.to_screen = fck_nk_api_to_screen,
	.control = fck_nk_api_control,
	.input = &nuklear_input_api,
	.hamburger = &nuklear_hamburger_api,
	.pie = &nuklear_pie_api,
	.panel = &nuklear_panel_api,
	.elements = &nuklear_property_api,
};

FCK_EXPORT_API fck_nuklear_api *fck_nuklear_load(fck_api_registry *registry, void *old)
{
	(void)old;
	apis = registry;
	registry->add(fck_nuklear_api_name, &nuklear_api);
	return &nuklear_api;
}

// Let's keep this mess at the bottom :D
static struct nk_color fck_ui_cached_colour_table[NK_COLOR_COUNT];
struct nk_color *fck_ui_set_style(struct nk_context *ctx, enum fck_nuklear_theme theme)
{
	if (theme == fck_nk_theme_white)
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
	else if (theme == fck_nk_theme_ruta)
	{
		const struct nk_color secondary = nk_rgba(75, 140, 0, 255);
		const struct nk_color secondary_highlight = nk_rgba(95, 178, 0, 255);
		const struct nk_color secondary_clicked = nk_rgba(115, 216, 0, 255);

		fck_ui_cached_colour_table[NK_COLOR_TEXT] = nk_rgba(210, 210, 210, 255);
		fck_ui_cached_colour_table[NK_COLOR_WINDOW] = nk_rgba(30, 33, 40, 215);
		fck_ui_cached_colour_table[NK_COLOR_HEADER] = secondary;
		fck_ui_cached_colour_table[NK_COLOR_BORDER] = nk_rgba(51, 55, 67, 255);
		fck_ui_cached_colour_table[NK_COLOR_BUTTON] = secondary;
		fck_ui_cached_colour_table[NK_COLOR_BUTTON_HOVER] = secondary_highlight;
		fck_ui_cached_colour_table[NK_COLOR_BUTTON_ACTIVE] = secondary_clicked;
		fck_ui_cached_colour_table[NK_COLOR_TOGGLE] = nk_rgba(51, 55, 67, 255);
		fck_ui_cached_colour_table[NK_COLOR_TOGGLE_HOVER] = secondary_highlight;
		fck_ui_cached_colour_table[NK_COLOR_TOGGLE_CURSOR] = secondary;
		fck_ui_cached_colour_table[NK_COLOR_SELECT] = nk_rgba(51, 55, 67, 255);
		fck_ui_cached_colour_table[NK_COLOR_SELECT_ACTIVE] = secondary;
		fck_ui_cached_colour_table[NK_COLOR_SLIDER] = nk_rgba(51, 55, 67, 255);
		fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR] = secondary;
		fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR_HOVER] = secondary_highlight;
		fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR_ACTIVE] = secondary_clicked;
		fck_ui_cached_colour_table[NK_COLOR_PROPERTY] = nk_rgba(51, 55, 67, 255);
		fck_ui_cached_colour_table[NK_COLOR_EDIT] = nk_rgba(51, 55, 67, 225);
		fck_ui_cached_colour_table[NK_COLOR_EDIT_CURSOR] = nk_rgba(190, 190, 190, 255);
		fck_ui_cached_colour_table[NK_COLOR_COMBO] = nk_rgba(51, 55, 67, 255);
		fck_ui_cached_colour_table[NK_COLOR_CHART] = nk_rgba(51, 55, 67, 255);
		fck_ui_cached_colour_table[NK_COLOR_CHART_COLOR] = secondary;
		fck_ui_cached_colour_table[NK_COLOR_CHART_COLOR_HIGHLIGHT] = secondary_highlight;
		fck_ui_cached_colour_table[NK_COLOR_SCROLLBAR] = nk_rgba(30, 33, 40, 255);
		fck_ui_cached_colour_table[NK_COLOR_SCROLLBAR_CURSOR] = nk_rgba(64, 84, 95, 255);
		fck_ui_cached_colour_table[NK_COLOR_SCROLLBAR_CURSOR_HOVER] = nk_rgba(70, 90, 100, 255);
		fck_ui_cached_colour_table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE] = nk_rgba(75, 95, 105, 255);
		fck_ui_cached_colour_table[NK_COLOR_TAB_HEADER] = secondary;
		fck_ui_cached_colour_table[NK_COLOR_KNOB] = fck_ui_cached_colour_table[NK_COLOR_SLIDER];
		fck_ui_cached_colour_table[NK_COLOR_KNOB_CURSOR] = fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR];
		fck_ui_cached_colour_table[NK_COLOR_KNOB_CURSOR_HOVER] = fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR_HOVER];
		fck_ui_cached_colour_table[NK_COLOR_KNOB_CURSOR_ACTIVE] = fck_ui_cached_colour_table[NK_COLOR_SLIDER_CURSOR_ACTIVE];
		nk_style_from_table(ctx, fck_ui_cached_colour_table);
	}
	else if (theme == fck_nk_theme_red)
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
	else if (theme == fck_nk_theme_blue)
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
	else if (theme == fck_nk_theme_dark)
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
	else if (theme == fck_nk_theme_dracula)
	{
		const struct nk_color background = nk_rgba(40, 42, 54, 255);
		const struct nk_color currentline = nk_rgba(68, 71, 90, 255);
		const struct nk_color foreground = nk_rgba(248, 248, 242, 255);
		const struct nk_color comment = nk_rgba(98, 114, 164, 255);
		/* struct nk_color cyan = nk_rgba(139, 233, 253, 255); */
		/* struct nk_color green = nk_rgba(80, 250, 123, 255); */
		/* struct nk_color orange = nk_rgba(255, 184, 108, 255); */
		const struct nk_color pink = nk_rgba(255, 121, 198, 255);
		const struct nk_color purple = nk_rgba(189, 147, 249, 255);
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
	else if (theme == fck_nk_theme_latte)
	{
		/*struct nk_color rosewater = nk_rgba(220, 138, 120, 255);*/
		/*struct nk_color flamingo = nk_rgba(221, 120, 120, 255);*/
		const struct nk_color pink = nk_rgba(234, 118, 203, 255);
		const struct nk_color mauve = nk_rgba(136, 57, 239, 255);
		/*struct nk_color red = nk_rgba(210, 15, 57, 255);*/
		/*struct nk_color maroon = nk_rgba(230, 69, 83, 255);*/
		/*struct nk_color peach = nk_rgba(254, 100, 11, 255);*/
		const struct nk_color yellow = nk_rgba(223, 142, 29, 255);
		/*struct nk_color green = nk_rgba(64, 160, 43, 255);*/
		const struct nk_color teal = nk_rgba(23, 146, 153, 255);
		/*struct nk_color sky = nk_rgba(4, 165, 229, 255);*/
		/*struct nk_color sapphire = nk_rgba(32, 159, 181, 255);*/
		/*struct nk_color blue = nk_rgba(30, 102, 245, 255);*/
		/*struct nk_color lavender = nk_rgba(114, 135, 253, 255);*/
		const struct nk_color text = nk_rgba(76, 79, 105, 255);
		/*struct nk_color subtext1 = nk_rgba(92, 95, 119, 255);*/
		/*struct nk_color subtext0 = nk_rgba(108, 111, 133, 255);*/
		const struct nk_color overlay2 = nk_rgba(124, 127, 147, 55);
		/*struct nk_color overlay1 = nk_rgba(140, 143, 161, 255);*/
		const struct nk_color overlay0 = nk_rgba(156, 160, 176, 255);
		const struct nk_color surface2 = nk_rgba(172, 176, 190, 255);
		const struct nk_color surface1 = nk_rgba(188, 192, 204, 255);
		const struct nk_color surface0 = nk_rgba(204, 208, 218, 255);
		const struct nk_color base = nk_rgba(239, 241, 245, 255);
		const struct nk_color mantle = nk_rgba(230, 233, 239, 255);
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
	else if (theme == fck_nk_theme_frappe)
	{
		/*struct nk_color rosewater = nk_rgba(242, 213, 207, 255);*/
		/*struct nk_color flamingo = nk_rgba(238, 190, 190, 255);*/
		const struct nk_color pink = nk_rgba(244, 184, 228, 255);
		/*struct nk_color mauve = nk_rgba(202, 158, 230, 255);*/
		/*struct nk_color red = nk_rgba(231, 130, 132, 255);*/
		/*struct nk_color maroon = nk_rgba(234, 153, 156, 255);*/
		/*struct nk_color peach = nk_rgba(239, 159, 118, 255);*/
		/*struct nk_color yellow = nk_rgba(229, 200, 144, 255);*/
		const struct nk_color green = nk_rgba(166, 209, 137, 255);
		/*struct nk_color teal = nk_rgba(129, 200, 190, 255);*/
		/*struct nk_color sky = nk_rgba(153, 209, 219, 255);*/
		/*struct nk_color sapphire = nk_rgba(133, 193, 220, 255);*/
		/*struct nk_color blue = nk_rgba(140, 170, 238, 255);*/
		const struct nk_color lavender = nk_rgba(186, 187, 241, 255);
		const struct nk_color text = nk_rgba(198, 208, 245, 255);
		/*struct nk_color subtext1 = nk_rgba(181, 191, 226, 255);*/
		/*struct nk_color subtext0 = nk_rgba(165, 173, 206, 255);*/
		const struct nk_color overlay2 = nk_rgba(148, 156, 187, 255);
		const struct nk_color overlay1 = nk_rgba(131, 139, 167, 255);
		const struct nk_color overlay0 = nk_rgba(115, 121, 148, 255);
		const struct nk_color surface2 = nk_rgba(98, 104, 128, 255);
		const struct nk_color surface1 = nk_rgba(81, 87, 109, 255);
		const struct nk_color surface0 = nk_rgba(65, 69, 89, 255);
		const struct nk_color base = nk_rgba(48, 52, 70, 255);
		const struct nk_color mantle = nk_rgba(41, 44, 60, 255);
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
	else if (theme == fck_nk_theme_macchiato)
	{
		/*struct nk_color rosewater = nk_rgba(244, 219, 214, 255);*/
		/*struct nk_color flamingo = nk_rgba(240, 198, 198, 255);*/
		const struct nk_color pink = nk_rgba(245, 189, 230, 255);
		/*struct nk_color mauve = nk_rgba(198, 160, 246, 255);*/
		/*struct nk_color red = nk_rgba(237, 135, 150, 255);*/
		/*struct nk_color maroon = nk_rgba(238, 153, 160, 255);*/
		/*struct nk_color peach = nk_rgba(245, 169, 127, 255);*/
		const struct nk_color yellow = nk_rgba(238, 212, 159, 255);
		const struct nk_color green = nk_rgba(166, 218, 149, 255);
		/*struct nk_color teal = nk_rgba(139, 213, 202, 255);*/
		/*struct nk_color sky = nk_rgba(145, 215, 227, 255);*/
		/*struct nk_color sapphire = nk_rgba(125, 196, 228, 255);*/
		/*struct nk_color blue = nk_rgba(138, 173, 244, 255);*/
		const struct nk_color lavender = nk_rgba(183, 189, 248, 255);
		const struct nk_color text = nk_rgba(202, 211, 245, 255);
		/*struct nk_color subtext1 = nk_rgba(184, 192, 224, 255);*/
		/*struct nk_color subtext0 = nk_rgba(165, 173, 203, 255);*/
		const struct nk_color overlay2 = nk_rgba(147, 154, 183, 255);
		const struct nk_color overlay1 = nk_rgba(128, 135, 162, 255);
		const struct nk_color overlay0 = nk_rgba(110, 115, 141, 255);
		const struct nk_color surface2 = nk_rgba(91, 96, 120, 255);
		const struct nk_color surface1 = nk_rgba(73, 77, 100, 255);
		const struct nk_color surface0 = nk_rgba(54, 58, 79, 255);
		const struct nk_color base = nk_rgba(36, 39, 58, 255);
		const struct nk_color mantle = nk_rgba(30, 32, 48, 255);
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
	else if (theme == fck_nk_theme_mocha)
	{
		/*struct nk_color rosewater = nk_rgba(245, 224, 220, 255);*/
		/*struct nk_color flamingo = nk_rgba(242, 205, 205, 255);*/
		const struct nk_color pink = nk_rgba(245, 194, 231, 255);
		/*struct nk_color mauve = nk_rgba(203, 166, 247, 255);*/
		/*struct nk_color red = nk_rgba(243, 139, 168, 255);*/
		/*struct nk_color maroon = nk_rgba(235, 160, 172, 255);*/
		/*struct nk_color peach = nk_rgba(250, 179, 135, 255);*/
		/*struct nk_color yellow = nk_rgba(249, 226, 175, 255);*/
		const struct nk_color green = nk_rgba(166, 227, 161, 255);
		/*struct nk_color teal = nk_rgba(148, 226, 213, 255);*/
		/*struct nk_color sky = nk_rgba(137, 220, 235, 255);*/
		/*struct nk_color sapphire = nk_rgba(116, 199, 236, 255);*/
		/*struct nk_color blue = nk_rgba(137, 180, 250, 255);*/
		const struct nk_color lavender = nk_rgba(180, 190, 254, 255);
		const struct nk_color text = nk_rgba(205, 214, 244, 255);
		/*struct nk_color subtext1 = nk_rgba(186, 194, 222, 255);*/
		/*struct nk_color subtext0 = nk_rgba(166, 173, 200, 255);*/
		const struct nk_color overlay2 = nk_rgba(147, 153, 178, 255);
		const struct nk_color overlay1 = nk_rgba(127, 132, 156, 255);
		const struct nk_color overlay0 = nk_rgba(108, 112, 134, 255);
		const struct nk_color surface2 = nk_rgba(88, 91, 112, 255);
		const struct nk_color surface1 = nk_rgba(69, 71, 90, 255);
		const struct nk_color surface0 = nk_rgba(49, 50, 68, 255);
		const struct nk_color base = nk_rgba(30, 30, 46, 255);
		const struct nk_color mantle = nk_rgba(24, 24, 37, 255);
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
