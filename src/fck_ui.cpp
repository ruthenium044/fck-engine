#include "fck_ui.h"

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
	CHECK_ERROR(renderer, SDL_GetError(), return);

	// Let's drive it from SDL_RenderDrawColor
	Uint8 r, g, b, a;
	CHECK_ERROR(SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a), SDL_GetError());
	CHECK_ERROR(SDL_SetTextureColorMod(texture, r, g, b), SDL_GetError());

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
		CHECK_ERROR(render_result, SDL_GetError(), return);
	}
}

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

fck_ui_button_style fck_ui_button_style_engine(fck_font_asset *font)
{
	fck_ui_button_style button_style;
	SDL_zero(button_style);

	button_style.hover.background_color = {0, 0, 0, 255};
	button_style.hover.border_color = {255, 255, 255, 255};
	button_style.normal.background_color = {0, 0, 0, 255};
	button_style.normal.border_color = {125, 125, 125, 255};

	button_style.hover.font = font;
	button_style.normal.font = font;

	fck_ui_style_set_margin(&button_style.normal, 4.0f);
	fck_ui_style_set_margin(&button_style.hover, 4.0f);
	return button_style;
}

bool fck_rect_point_intersection(SDL_FRect const *rect, SDL_FPoint const *point)
{
	SDL_assert(rect != nullptr);
	SDL_assert(point != nullptr);

	return point->x > rect->x && point->x < rect->x + rect->w && point->y > rect->y && point->y < rect->y + rect->h;
}

bool fck_ui_button(SDL_Renderer *renderer, fck_mouse_state const *mouse, SDL_FRect const *button_rect,
                   fck_ui_button_style const *button_style, const char *label)
{
	SDL_assert(mouse != nullptr);
	SDL_assert(button_rect != nullptr);
	SDL_assert(button_style != nullptr);
	SDL_assert(renderer != nullptr);

	SDL_FPoint mouse_point = {mouse->current.cursor_position_x, mouse->current.cursor_position_y};

	fck_ui_style const *style;

	bool is_clicked = false;

	if (fck_rect_point_intersection(button_rect, &mouse_point))
	{
		style = &button_style->hover;
		if (fck_button_just_down(mouse, SDL_BUTTON_LEFT))
		{
			// If we do not draw for a frame, we should get a nice little blink effect
			// Let's see lol
			is_clicked = true;
		}
	}
	else
	{
		style = &button_style->normal;
	}

	// Default colour for a click
	const SDL_Color some_shitty_green = {160, 210, 160, 255};

	SDL_FRect border_rect = fck_rect_apply_margin(button_rect, &style->margin);
	SDL_FRect content_rect = fck_rect_apply_padding(&border_rect, &style->padding);

	SDL_Color background = is_clicked ? some_shitty_green : style->background_color;
	SDL_Color border = is_clicked ? some_shitty_green : style->border_color;
	SDL_SetRenderDrawColor(renderer, background.r, background.g, background.b, background.a);
	SDL_RenderFillRect(renderer, &content_rect);

	SDL_SetRenderDrawColor(renderer, border.r, border.g, border.b, border.a);
	SDL_RenderRect(renderer, &border_rect);

	if (label != nullptr)
	{
		fck_layout layout = {2, FCK_LAYOUT_HORIZONTAL_ALIGNMENT_CENTRE, FCK_LAYOUT_VERTICAL_ALIGNMENT_CENTRE};
		CHECK_WARNING(style->font != nullptr, "No font available in style!", return is_clicked);
		fck_render_text(style->font, label, &layout, button_rect);
	}

	return is_clicked;
}

bool fck_texture_load(SDL_Renderer *renderer, const char *relative_file_path, SDL_Texture **out_texture)
{
	SDL_assert(renderer != nullptr);
	SDL_assert(relative_file_path != nullptr);
	SDL_assert(out_texture != nullptr);

	fck_file_memory file_memory;
	if (!fck_file_read("", relative_file_path, "", &file_memory))
	{
		return false;
	}

	SDL_IOStream *stream = SDL_IOFromMem(file_memory.data, file_memory.size);
	CHECK_ERROR(stream, SDL_GetError(), return false);

	// IMG_Load_IO frees stream!
	SDL_Surface *surface = IMG_Load_IO(stream, true);
	CHECK_ERROR(surface, SDL_GetError(), return false);

	const SDL_Palette *palette = SDL_GetSurfacePalette(surface);
	const SDL_PixelFormatDetails *details = SDL_GetPixelFormatDetails(surface->format);
	CHECK_ERROR(details, SDL_GetError(), return false);

	// Maybe generalise bitmap transparency
	bool set_color_key_result = SDL_SetSurfaceColorKey(surface, true, SDL_MapRGB(details, palette, 248, 0, 248));
	CHECK_ERROR(set_color_key_result, SDL_GetError(), return false);

	*out_texture = SDL_CreateTextureFromSurface(renderer, surface);

	SDL_DestroySurface(surface);

	CHECK_ERROR(*out_texture, SDL_GetError(), return false);

	return true;
}

SDL_bool fck_font_asset_load(SDL_Renderer *renderer, const char *file_name, fck_font_asset *font_asset)
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

	CHECK_ERROR(load_result, SDL_GetError(), return false);

	return true;
}