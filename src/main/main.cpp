// fck main
// TODO:
// - Graphics
// - Input handling (x)
// - Draw some images
// - Systems and data model
// - Data serialisation
// - Frame independence
// - Networking!! <- implies multiplayer

// SDL core - functionality such as creating a window and getting events
#include <SDL3/SDL.h>

// SDL image - Loads images... Many kinds. We only care about PNG
#include <SDL3_image/SDL_image.h>

// SDL net - networking... More later
#include <SDL3_net/SDL_net.h>

#include "fck_checks.h"
#include "fck_drop_file.h"
#include "fck_ecs.h"
#include "fck_keyboard.h"
#include "fck_mouse.h"

struct fck_font_resource
{
	char texture_path[256];
	int32_t pixel_per_glyph_w;
	int32_t pixel_per_glyph_h;
	int32_t columns;
	int32_t rows;
};

struct fck_font_asset
{
	SDL_Texture *texture;
	int32_t pixel_per_glyph_w;
	int32_t pixel_per_glyph_h;
	int32_t columns;
	int32_t rows;
};

struct fck_font_editor
{
	SDL_Texture *selected_font_texture;
	const char *relative_texture_path;

	float editor_pivot_x;
	float editor_pivot_y;
	float editor_scale;

	int pixel_per_glyph_w;
	int pixel_per_glyph_h;
};

struct fck_engine
{
	SDL_Window *window;
	SDL_Renderer *renderer;

	fck_mouse_state mouse;
	fck_keyboard_state keyboard;

	fck_font_asset default_editor_font;
	fck_font_editor font_editor;
};

struct fck_pig
{
	// :((
};

struct fck_wolf
{
	double itsadouble;
	// :((
};

struct fck_file_memory
{
	Uint8 *data;
	size_t size;
};

enum SDL_UICutMode
{
	SDL_UI_CUT_MODE_LEFT,
	SDL_UI_CUT_MODE_RIGHT,
	SDL_UI_CUT_MODE_TOP,
	SDL_UI_CUT_MODE_BOTTOM,
};

SDL_FRect SDL_FRectCreate(float x, float y, float w, float h)
{
	SDL_FRect rect;
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;
	return rect;
}

SDL_FRect SDL_FRectCutRight(SDL_FRect *target, float cut)
{
	target->w = SDL_max(target->w - cut, 0);
	return SDL_FRectCreate(target->x + target->w, target->y, cut, target->h);
}

SDL_FRect SDL_FRectCutBottom(SDL_FRect *target, float cut)
{
	target->h = SDL_max(target->h - cut, 0);
	return SDL_FRectCreate(target->x, target->y + target->h, target->w, cut);
}

SDL_FRect SDL_FRectCutLeft(SDL_FRect *target, float cut)
{
	const float width = target->w;
	target->w = SDL_max(target->w - cut, 0);
	float x = target->x;
	target->x = target->x + (width - target->w);
	return SDL_FRectCreate(x, target->y, cut, target->h);
}

SDL_FRect SDL_FRectCutTop(SDL_FRect *target, float cut)
{
	const float height = target->h;
	target->h = SDL_max(target->h - cut, 0);
	float y = target->y;
	target->y = target->y + (height - target->h);
	return SDL_FRectCreate(target->x, y, target->w, cut);
}

SDL_FRect SDL_FRectCut(SDL_FRect *target, SDL_UICutMode cutMode, float cut)
{
	switch (cutMode)
	{
	case SDL_UI_CUT_MODE_LEFT:
		return SDL_FRectCutLeft(target, cut);
	case SDL_UI_CUT_MODE_RIGHT:
		return SDL_FRectCutRight(target, cut);
	case SDL_UI_CUT_MODE_TOP:
		return SDL_FRectCutTop(target, cut);
	case SDL_UI_CUT_MODE_BOTTOM:
		return SDL_FRectCutBottom(target, cut);
	}
	SDL_assert(false && "Should never end up here - Invalid Rect cut mode");
	return *target;
}

bool fck_file_write(const char *path, const char *name, const char *extension, const void *source, size_t size)
{
	SDL_assert(path != nullptr && "NULL is not a path");
	SDL_assert(name != nullptr && "NULL is not a name");
	SDL_assert(extension != nullptr && "NULL is not an extension");
	SDL_assert(source != nullptr && size != 0);

	char path_buffer[512];
	SDL_zero(path_buffer);

	size_t added_length = SDL_strlcat(path_buffer, FCK_RESOURCE_DIRECTORY_PATH, sizeof(path_buffer));
	added_length = added_length + SDL_strlcat(path_buffer + added_length, path, sizeof(path_buffer));
	added_length = added_length + SDL_strlcat(path_buffer + added_length, name, sizeof(path_buffer));
	SDL_strlcat(path_buffer + added_length, extension, sizeof(path_buffer));

	SDL_IOStream *stream = SDL_IOFromFile(path_buffer, "wb");
	CHECK_WARNING(stream == nullptr, SDL_GetError(), return false);

	size_t written_size = SDL_WriteIO(stream, source, size);
	CHECK_WARNING(written_size < size, SDL_GetError());

	SDL_bool result = SDL_CloseIO(stream);
	CHECK_WARNING(!result, SDL_GetError(), return false);

	return true;
}

bool fck_file_read(const char *path, const char *name, const char *extension, fck_file_memory *output)
{
	SDL_assert(path != nullptr && "NULL is not a path");
	SDL_assert(name != nullptr && "NULL is not a name");
	SDL_assert(extension != nullptr && "NULL is not an extension");
	SDL_assert(output != nullptr);

	char path_buffer[512];
	SDL_zero(path_buffer);

	size_t added_length = SDL_strlcat(path_buffer, FCK_RESOURCE_DIRECTORY_PATH, sizeof(path_buffer));
	added_length = added_length + SDL_strlcat(path_buffer + added_length, path, sizeof(path_buffer));
	added_length = added_length + SDL_strlcat(path_buffer + added_length, name, sizeof(path_buffer));
	SDL_strlcat(path_buffer + added_length, extension, sizeof(path_buffer));

	SDL_IOStream *stream = SDL_IOFromFile(path_buffer, "rb");
	CHECK_ERROR(stream == nullptr, SDL_GetError(), return false);

	Sint64 stream_size = SDL_GetIOSize(stream);
	CHECK_ERROR(stream_size < 0, SDL_GetError());

	Uint8 *data = (Uint8 *)SDL_malloc(stream_size);

	size_t read_size = SDL_ReadIO(stream, data, stream_size);
	SDL_bool has_error_in_reading = read_size < stream_size;
	CHECK_ERROR(has_error_in_reading, SDL_GetError());

	SDL_bool result = SDL_CloseIO(stream);
	CHECK_ERROR(!result, SDL_GetError());

	if (has_error_in_reading)
	{
		SDL_free(data);
		output->data = nullptr;
		output->size = 0;
		return false;
	}

	output->data = data;
	output->size = read_size;

	return true;
}

void fck_file_free(fck_file_memory *file_memory)
{
	SDL_assert(file_memory != nullptr);

	SDL_free(file_memory->data);
	file_memory->data = nullptr;
	file_memory->size = 0;
}

bool fck_drop_file_receive_png(fck_drop_file_context const *context, SDL_DropEvent const *drop_event)
{
	SDL_assert(context != nullptr);
	SDL_assert(drop_event != nullptr);

	SDL_IOStream *stream = SDL_IOFromFile(drop_event->data, "r");
	CHECK_ERROR(stream == nullptr, SDL_GetError());
	if (!IMG_isPNG(stream))
	{
		// We only allow pngs for now!
		SDL_CloseIO(stream);
		return false;
	}
	const char resource_path_base[] = FCK_RESOURCE_DIRECTORY_PATH;

	const char *target_file_path = drop_event->data;

	// Spin till the end
	const char *target_file_name = SDL_strrchr(target_file_path, '\\');
	SDL_assert(target_file_name != nullptr && "Potential file name is directory?");
	target_file_name = target_file_name + 1;

	char path_buffer[512];
	SDL_zero(path_buffer);
	size_t added_length = SDL_strlcat(path_buffer, resource_path_base, sizeof(path_buffer));

	// There is actually no possible way the path is longer than 2024... Let's
	// just pray
	SDL_strlcat(path_buffer + added_length, target_file_name, sizeof(path_buffer));

	SDL_bool result = SDL_CopyFile(drop_event->data, path_buffer);
	CHECK_INFO(!result, SDL_GetError());

	SDL_CloseIO(stream);

	return true;
}

bool fck_texture_load(SDL_Renderer *renderer, const char *relative_file_path, SDL_Texture **out_texture)
{
	SDL_assert(renderer != nullptr);
	SDL_assert(relative_file_path != nullptr);
	SDL_assert(out_texture != nullptr);

	// CMake (the thing that sets the project up) generates this path
	// No heap allocation is happening since it exists as a constant r-value
	const char resource_path_base[] = FCK_RESOURCE_DIRECTORY_PATH;

	char path_buffer[512];
	SDL_zero(path_buffer);

	size_t added_length = SDL_strlcat(path_buffer, resource_path_base, sizeof(path_buffer));

	SDL_strlcat(path_buffer + added_length, relative_file_path, sizeof(path_buffer));

	*out_texture = IMG_LoadTexture(renderer, path_buffer);

	return out_texture != nullptr;
}

void fck_font_editor_allocate(fck_font_editor *editor)
{
	SDL_assert(editor != nullptr);

	editor->editor_scale = 1.0f;
	editor->editor_pivot_x = 0.0f;
	editor->editor_pivot_y = 0.0f;

	// Rows/Cols or target glyph size?
	editor->pixel_per_glyph_w = 8;
	editor->pixel_per_glyph_h = 12;
}

void fck_font_editor_free(fck_font_editor *editor)
{
}

bool fck_rect_point_intersection(SDL_FRect const *rect, SDL_FPoint const *point)
{
	SDL_assert(rect != nullptr);
	SDL_assert(point != nullptr);

	return point->x > rect->x && point->x < rect->x + rect->w && point->y > rect->y && point->y < rect->y + rect->h;
}

enum fck_layout_horizontal_alignment : int32_t
{
	FCK_LAYOUT_HORIZONTAL_ALIGNMENT_LEFT,
	FCK_LAYOUT_HORIZONTAL_ALIGNMENT_CENTRE,
	FCK_LAYOUT_HORIZONTAL_ALIGNMENT_RIGHT
};

enum fck_layout_vertical_alignment : int32_t
{
	FCK_LAYOUT_VERTICAL_ALIGNMENT_TOP,
	FCK_LAYOUT_VERTICAL_ALIGNMENT_CENTRE,
	FCK_LAYOUT_VERTICAL_ALIGNMENT_BOTTOM
};

struct fck_layout
{
	int32_t scale;
	fck_layout_horizontal_alignment horizontal_alignment;
	fck_layout_vertical_alignment vertical_alignment;
};

float fck_calculate_vertical_offset(fck_layout_vertical_alignment alignment, SDL_FRect const *dst,
                                    int32_t text_row_count, int32_t glyph_height)
{
	SDL_assert(dst != nullptr);

	switch (alignment)
	{
	case FCK_LAYOUT_VERTICAL_ALIGNMENT_TOP:
		return 0.0f;
	case FCK_LAYOUT_VERTICAL_ALIGNMENT_CENTRE: {
		float height = text_row_count * glyph_height;
		return (dst->h * 0.5f) - (height * 0.5f);
	}
	case FCK_LAYOUT_VERTICAL_ALIGNMENT_BOTTOM: {
		float height = text_row_count * glyph_height;
		return dst->h - height;
	}
	default:
		return 0.0f;
	}
}

float fck_calculate_horizontal_offset(fck_layout_horizontal_alignment alignment, SDL_FRect const *dst,
                                      int32_t text_length, int32_t glyph_width)
{
	SDL_assert(dst != nullptr);

	switch (alignment)
	{
	case FCK_LAYOUT_HORIZONTAL_ALIGNMENT_LEFT:
		return 0.0f;
	case FCK_LAYOUT_HORIZONTAL_ALIGNMENT_CENTRE: {
		float width = text_length * glyph_width;
		return (dst->w * 0.5f) - (width * 0.5f);
	}
	case FCK_LAYOUT_HORIZONTAL_ALIGNMENT_RIGHT: {
		float width = text_length * glyph_width;
		return dst->w - width;
	}
	default:
		return 0.0f;
	}
	return 0.0f;
}

void fck_render_text(fck_font_asset const *font_asset, const char *text, fck_layout const *layout,
                     SDL_FRect const *draw_area)
{
	SDL_assert(font_asset != nullptr);
	SDL_assert(text != nullptr);
	SDL_assert(layout != nullptr);
	SDL_assert(draw_area != nullptr);

	// 64 hard limit. No reason
	// fck_font_editor *font_editor = &engine->font_editor;
	SDL_Texture *texture = font_asset->texture;
	float glyph_w = font_asset->pixel_per_glyph_w;
	float glyph_h = font_asset->pixel_per_glyph_h;
	float scaled_glyph_w = glyph_w * layout->scale;
	float scaled_glyph_h = glyph_h * layout->scale;
	SDL_FRect src_rect = {0, 0, glyph_w, glyph_h};
	SDL_FRect dst_rect = {0, 0, scaled_glyph_w, scaled_glyph_h};

	SDL_Renderer *renderer = SDL_GetRendererFromTexture(texture);
	CHECK_ERROR(renderer == nullptr, SDL_GetError(), return);

	// Let's drive it from SDL_RenderDrawColor
	Uint8 r, g, b, a;
	CHECK_ERROR(!SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a), SDL_GetError());
	CHECK_ERROR(!SDL_SetTextureColorMod(texture, r, g, b), SDL_GetError());

	const int MAX_LENGTH = 64;
	int text_length = SDL_strnlen(text, MAX_LENGTH);

	fck_layout_horizontal_alignment hor_alignment = layout->horizontal_alignment;
	fck_layout_vertical_alignment vert_alignment = layout->vertical_alignment;
	float offset_x = fck_calculate_horizontal_offset(hor_alignment, draw_area, text_length, scaled_glyph_w);
	float offset_y = fck_calculate_vertical_offset(vert_alignment, draw_area, 1, scaled_glyph_h);

	int32_t glyph_cols = font_asset->columns;
	int32_t glyph_rows = font_asset->rows;

	for (size_t index = 0; index < MAX_LENGTH; index++)
	{
		const char c = text[index];
		if (c == 0)
		{
			// maybe break instead, if cleanup is needed
			return;
		}
		int x = c % glyph_cols;
		int y = c / glyph_cols;

		src_rect.x = x * glyph_w;
		src_rect.y = y * glyph_h;

		dst_rect.x = draw_area->x + offset_x + (dst_rect.w * index);
		dst_rect.y = draw_area->y + offset_y;

		SDL_bool render_result = SDL_RenderTexture(renderer, texture, &src_rect, &dst_rect);
		CHECK_ERROR(!render_result, SDL_GetError(), return);
	}
	return;
}

struct fck_ui_margin
{
	float top;
	float bottom;
	float left;
	float right;
};

struct fck_ui_padding
{
	float top;
	float bottom;
	float left;
	float right;
};

struct fck_ui_style
{
	// SDL_Color font_color;
	SDL_Color background_color;
	SDL_Color border_color;
	fck_ui_margin margin;
	fck_ui_padding padding;
	fck_font_asset *font;
};

struct fck_ui_button_style
{
	fck_ui_style normal;
	fck_ui_style hover;
};

SDL_FRect fck_rect_apply_padding(SDL_FRect const *rect, fck_ui_padding const *padding)
{
	SDL_assert(rect != nullptr);
	SDL_assert(padding != nullptr);

	SDL_FRect adjusted_rect;

	adjusted_rect.x = rect->x + padding->left;
	adjusted_rect.y = rect->y + padding->top;
	adjusted_rect.w = rect->w - (padding->left + padding->right);
	adjusted_rect.h = rect->h - (padding->top + padding->bottom);

	return adjusted_rect;
}

SDL_FRect fck_rect_apply_margin(SDL_FRect const *rect, fck_ui_margin const *margin)
{
	SDL_assert(rect != nullptr);
	SDL_assert(margin != nullptr);

	SDL_FRect adjusted_rect;

	adjusted_rect.x = rect->x + margin->left;
	adjusted_rect.y = rect->y + margin->top;
	adjusted_rect.w = rect->w - (margin->left + margin->right);
	adjusted_rect.h = rect->h - (margin->top + margin->bottom);

	return adjusted_rect;
}

void fck_ui_style_set_padding(fck_ui_style *style, float padding)
{
	style->padding.top = padding;
	style->padding.bottom = padding;
	style->padding.left = padding;
	style->padding.right = padding;
}

void fck_ui_style_set_padding(fck_ui_style *style, float top, float bottom, float left, float right)
{
	style->padding.top = top;
	style->padding.bottom = bottom;
	style->padding.left = left;
	style->padding.right = right;
}

void fck_ui_style_set_margin(fck_ui_style *style, float padding)
{
	style->margin.top = padding;
	style->margin.bottom = padding;
	style->margin.left = padding;
	style->margin.right = padding;
}

void fck_ui_style_set_margin(fck_ui_style *style, float top, float bottom, float left, float right)
{
	style->margin.top = top;
	style->margin.bottom = bottom;
	style->margin.left = left;
	style->margin.right = right;
}

fck_ui_button_style fck_ui_button_style_simple()
{
	fck_ui_button_style button_style;
	SDL_zero(button_style);

	button_style.hover.background_color = {0, 0, 0, 255};
	button_style.hover.border_color = {255, 255, 255, 255};
	button_style.normal.background_color = {0, 0, 0, 255};
	button_style.normal.border_color = {125, 125, 125, 255};

	fck_ui_style_set_margin(&button_style.normal, 4.0f);
	fck_ui_style_set_margin(&button_style.hover, 4.0f);
	return button_style;
}

bool fck_ui_button(fck_engine *engine, SDL_FRect const *button_rect, fck_ui_button_style const *button_style,
                   const char *label = nullptr)
{
	SDL_assert(engine != nullptr);
	SDL_assert(button_rect != nullptr);
	SDL_assert(button_style != nullptr);
	SDL_assert(engine->renderer != nullptr);

	SDL_Renderer *renderer = engine->renderer;
	fck_mouse_state *mouse_state = &engine->mouse;

	SDL_FPoint mouse_point = {mouse_state->current.cursor_position_x, mouse_state->current.cursor_position_y};

	fck_ui_style const *style;
	if (fck_rect_point_intersection(button_rect, &mouse_point))
	{
		style = &button_style->hover;
		if (fck_button_just_down(mouse_state, SDL_BUTTON_LEFT))
		{
			// If we do not draw for a frame, we should get a nice little blink effect
			// Let's see lol
			return true;
		}
	}
	else
	{
		style = &button_style->normal;
	}

	SDL_FRect border_rect = fck_rect_apply_margin(button_rect, &style->margin);
	SDL_FRect content_rect = fck_rect_apply_padding(&border_rect, &style->padding);

	SDL_Color background = style->background_color;
	SDL_Color border = style->border_color;
	SDL_SetRenderDrawColor(renderer, background.r, background.g, background.b, background.a);
	SDL_RenderFillRect(renderer, &content_rect);

	SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, border.a);
	SDL_RenderRect(renderer, &border_rect);

	if (label != nullptr)
	{
		fck_layout layout = {2, FCK_LAYOUT_HORIZONTAL_ALIGNMENT_CENTRE, FCK_LAYOUT_VERTICAL_ALIGNMENT_CENTRE};
		fck_font_asset *font = style->font != nullptr ? style->font : &engine->default_editor_font;
		fck_render_text(font, label, &layout, button_rect);
	}

	return false;
}

void fck_font_editor_update(fck_engine *engine)
{
	fck_font_editor *font_editor = &engine->font_editor;
	SDL_Renderer *renderer = engine->renderer;
	fck_mouse_state *mouse_state = &engine->mouse;

	float scale = font_editor->editor_scale;

	scale = scale + mouse_state->current.scroll_delta_y * 0.25f;
	scale = SDL_clamp(scale, 0.1f, 8.0f);

	font_editor->editor_scale = scale;

	if (font_editor->selected_font_texture != nullptr)
	{
		float w;
		float h;
		CHECK_ERROR(!SDL_GetTextureSize(font_editor->selected_font_texture, &w, &h), SDL_GetError());

		float x = font_editor->editor_pivot_x;
		float y = font_editor->editor_pivot_y;
		float half_w = w * 0.5f;
		float half_h = h * 0.5f;

		float scaled_w = w * scale;
		float scaled_h = h * scale;
		float scaled_half_w = scaled_w * 0.5f;
		float scaled_half_h = scaled_h * 0.5f;
		float offset_x = half_w - scaled_half_w;
		float offset_y = half_h - scaled_half_h;
		float dst_x = x + offset_x;
		float dst_y = y + offset_y;
		SDL_FRect texture_rect_src = {0, 0, w, h};
		SDL_FRect texture_rect_dst = {dst_x, dst_y, scaled_w, scaled_h};

		SDL_FPoint mouse_point = {mouse_state->current.cursor_position_x, mouse_state->current.cursor_position_y};

		if (fck_button_down(mouse_state, SDL_BUTTON_LEFT))
		{
			font_editor->editor_pivot_x -= mouse_point.x - mouse_state->previous.cursor_position_x;
			font_editor->editor_pivot_y -= mouse_point.y - mouse_state->previous.cursor_position_y;
		}
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		SDL_SetTextureColorMod(font_editor->selected_font_texture, 255, 255, 255);
		SDL_RenderTexture(renderer, font_editor->selected_font_texture, &texture_rect_src, &texture_rect_dst);
		SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
		SDL_RenderRect(renderer, &texture_rect_dst);

		SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
		float scaled_glyph_w = font_editor->pixel_per_glyph_w * scale;
		float scaled_glyph_h = font_editor->pixel_per_glyph_h * scale;

		int glyph_cols = scaled_w / scaled_glyph_w;
		int glyph_rows = scaled_h / scaled_glyph_h;

		for (int glyph_index_y = 0; glyph_index_y < glyph_rows; glyph_index_y++)
		{
			for (int glyph_index_x = 0; glyph_index_x < glyph_cols; glyph_index_x++)
			{
				float glyph_x = dst_x + (scaled_glyph_w * glyph_index_x);
				float glyph_y = dst_y + (scaled_glyph_h * glyph_index_y);
				SDL_FRect glyph_rect = {glyph_x, glyph_y, scaled_glyph_w, scaled_glyph_h};
				SDL_RenderRect(renderer, &glyph_rect);
			}
		}

		int window_width;
		int window_height;
		if (SDL_GetWindowSize(engine->window, &window_width, &window_height))
		{
			const float save_button_padding = 4.0f;
			const float save_button_x = 0.0f + save_button_padding;
			const float save_button_width = window_width - (save_button_padding * 2.0f);
			const float save_button_height = 64.0f - (save_button_padding * 2.0f);
			const float save_button_y = window_height - save_button_height - save_button_padding;
			SDL_FRect save_button_rect = {save_button_x, save_button_y, save_button_width, save_button_height};

			if (fck_rect_point_intersection(&save_button_rect, &mouse_point))
			{
				SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
				if (fck_button_just_down(mouse_state, SDL_BUTTON_LEFT))
				{
					if (font_editor->relative_texture_path != nullptr)
					{
						fck_font_resource config;
						SDL_zero(config);
						SDL_strlcpy(config.texture_path, font_editor->relative_texture_path,
						            sizeof(config.texture_path));
						config.pixel_per_glyph_w = font_editor->pixel_per_glyph_w;
						config.pixel_per_glyph_h = font_editor->pixel_per_glyph_h;
						config.rows = glyph_rows;
						config.columns = glyph_cols;

						bool write_result = fck_file_write("", "special", ".font", &config, sizeof(config));
					}
				}
			}
			else
			{
				SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
			}

			fck_layout layout = {2, FCK_LAYOUT_HORIZONTAL_ALIGNMENT_CENTRE, FCK_LAYOUT_VERTICAL_ALIGNMENT_CENTRE};
			fck_render_text(&engine->default_editor_font, "SAVE TO FILE", &layout, &save_button_rect);

			SDL_RenderRect(renderer, &save_button_rect);
		}
	}
}

SDL_bool fck_load_font_asset(SDL_Renderer *renderer, const char *file_name, fck_font_asset *font_asset)
{
	fck_file_memory file_memory;
	if (!fck_file_read("", file_name, ".font", &file_memory))
	{
		return false;
	}

	fck_font_resource *font_resource = (fck_font_resource *)file_memory.data;

	font_asset->pixel_per_glyph_h = font_resource->pixel_per_glyph_h;
	font_asset->pixel_per_glyph_w = font_resource->pixel_per_glyph_w;
	font_asset->columns = font_resource->columns;
	font_asset->rows = font_resource->rows;
	SDL_bool load_result = fck_texture_load(renderer, font_resource->texture_path, &font_asset->texture);

	fck_file_free(&file_memory);

	CHECK_CRITICAL(!load_result, SDL_GetError(), return false);

	return true;
}

int fck_print_directory(void *userdata, const char *dirname, const char *fname)
{
	const char *extension = SDL_strrchr(fname, '.');

	SDL_Log("%s - %s - %s", dirname, fname, extension);

	return 1;
}

int main(int c, char **str)
{
	fck_engine engine;
	SDL_zero(engine);

	// Init Systems
	CHECK_CRITICAL(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS), SDL_GetError());

	CHECK_CRITICAL(!IMG_Init(IMG_INIT_PNG), SDL_GetError());

	CHECK_CRITICAL(SDLNet_Init() == -1, SDL_GetError())

	const int window_width = 640;
	const int window_height = 640;
	engine.window = SDL_CreateWindow("fck - engine", window_width, window_height, 0);
	CHECK_CRITICAL(engine.window == nullptr, SDL_GetError());

	engine.renderer = SDL_CreateRenderer(engine.window, nullptr);
	CHECK_CRITICAL(engine.renderer == nullptr, SDL_GetError());

	CHECK_WARNING(!SDL_SetRenderVSync(engine.renderer, true), SDL_GetError());

	// Init Application
	fck_ecs ecs;
	SDL_zero(ecs);

	fck_drop_file_context drop_file_context;
	SDL_zero(drop_file_context);

	fck_load_font_asset(engine.renderer, "special", &engine.default_editor_font);

	fck_drop_file_context_allocate(&drop_file_context, 16);
	fck_drop_file_context_push(&drop_file_context, fck_drop_file_receive_png);

	// Init data blob
	// Unused - Just test
	const int INITIAL_ENTITY_COUNT = 128;
	const int INITIAL_COMPONENT_COUNT = 32;

	fck_ecs_allocate_configuration allocate_configuration;
	allocate_configuration.initial_entities_count = INITIAL_ENTITY_COUNT;
	allocate_configuration.initial_components_count = INITIAL_COMPONENT_COUNT;

	fck_ecs_allocate(&ecs, &allocate_configuration);

	fck_font_editor_allocate(&engine.font_editor);

	// Register types
	fck_component_handle pig_handle = fck_component_register(&ecs, 0, sizeof(fck_pig));
	fck_component_handle wolf_handle = fck_component_register(&ecs, 1, sizeof(fck_wolf));

	// Reserve a player slot
	fck_entity player_entity = fck_entity_create(&ecs);

	// Set and get data
	fck_wolf wolf = {420.0};
	fck_component_set(&ecs, &player_entity, &wolf_handle, &wolf);
	uint8_t *raw_wolf_data = fck_component_get(&ecs, &player_entity, &wolf_handle);
	fck_wolf *wolf_data = (fck_wolf *)raw_wolf_data;
	// !Unused - Just test

	engine.font_editor.selected_font_texture = engine.default_editor_font.texture;
	float w, h;
	CHECK_ERROR(!SDL_GetTextureSize(engine.default_editor_font.texture, &w, &h), SDL_GetError());
	int offset_x = (window_width * 0.5f) - (w * 0.5f);
	int offset_y = (window_height * 0.5f) - (h * 0.5f);
	engine.font_editor.editor_pivot_x = offset_x;
	engine.font_editor.editor_pivot_y = offset_y;

	// engine.font_editor.relative_texture_path = font_config->texture_path;

	SDL_EnumerateDirectory(FCK_RESOURCE_DIRECTORY_PATH, fck_print_directory, nullptr);

	bool is_font_editor_open = false;

	bool is_running = true;
	while (is_running)
	{
		// Event processing - Input, window, etc.
		SDL_Event ev;

		float scroll_delta_x = 0.0f;
		float scroll_delta_y = 0.0f;
		while (SDL_PollEvent(&ev))
		{
			switch (ev.type)
			{
			case SDL_EventType::SDL_EVENT_QUIT:
				is_running = false;
				break;
			case SDL_EventType::SDL_EVENT_DROP_FILE:
				fck_drop_file_context_notify(&drop_file_context, &ev.drop);
				break;
			case SDL_EventType::SDL_EVENT_MOUSE_WHEEL:
				scroll_delta_x = scroll_delta_x + ev.wheel.x;
				scroll_delta_y = scroll_delta_y + ev.wheel.y;
				break;
			default:
				break;
			}
		}
		fck_keyboard_state_update(&engine.keyboard);
		fck_mouse_state_update(&engine.mouse, scroll_delta_x, scroll_delta_y);

		SDL_SetRenderDrawColor(engine.renderer, 0, 0, 0, 255);
		SDL_RenderClear(engine.renderer);

		fck_layout layout = {2, FCK_LAYOUT_HORIZONTAL_ALIGNMENT_CENTRE, FCK_LAYOUT_VERTICAL_ALIGNMENT_CENTRE};

		// Render processing
		if (is_font_editor_open)
		{
			fck_font_editor_update(&engine);
		}
		else
		{
			// SDL_FRect button_rect = {0.0f, 32.0f, window_width, 64.0f};
		}

		SDL_FRect button_rect = {0.0f, 32.0f, window_width, 64.0f};
		fck_ui_button_style button_style = fck_ui_button_style_simple();
		if (fck_ui_button(&engine, &button_rect, &button_style, "FONT EDITOR"))
		{
			is_font_editor_open = !is_font_editor_open;
		}

		SDL_RenderPresent(engine.renderer);
	}

	fck_font_editor_free(&engine.font_editor);

	fck_drop_file_context_free(&drop_file_context);

	fck_ecs_free(&ecs);

	SDL_DestroyRenderer(engine.renderer);
	SDL_DestroyWindow(engine.window);

	SDLNet_Quit();

	IMG_Quit();

	SDL_Quit();
	return 0;
}