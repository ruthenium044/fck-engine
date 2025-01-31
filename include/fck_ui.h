#ifndef FCK_UI_INCLUDED
#define FCK_UI_INCLUDED

#include "core/fck_mouse.h"
#include "shared/fck_checks.h"
#include "shared/fck_file.h"

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <SDL3_image/SDL_image.h>

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

struct fck_layout
{
	int32_t scale;
	fck_layout_horizontal_alignment horizontal_alignment;
	fck_layout_vertical_alignment vertical_alignment;
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

float fck_calculate_vertical_offset(fck_layout_vertical_alignment alignment, SDL_FRect const *dst, int32_t text_row_count,
                                    int32_t glyph_height);

float fck_calculate_horizontal_offset(fck_layout_horizontal_alignment alignment, SDL_FRect const *dst, int32_t text_length,
                                      int32_t glyph_width);

void fck_render_text(fck_font_asset const *font_asset, const char *text, fck_layout const *layout, SDL_FRect const *draw_area);

SDL_FRect fck_rect_apply_padding(SDL_FRect const *rect, fck_ui_padding const *padding);

SDL_FRect fck_rect_apply_margin(SDL_FRect const *rect, fck_ui_margin const *margin);

void fck_ui_style_set_padding(fck_ui_style *style, float padding);

void fck_ui_style_set_padding(fck_ui_style *style, float top, float bottom, float left, float right);

void fck_ui_style_set_margin(fck_ui_style *style, float padding);

void fck_ui_style_set_margin(fck_ui_style *style, float top, float bottom, float left, float right);

fck_ui_button_style fck_ui_button_style_engine(fck_font_asset *font);

bool fck_rect_point_intersection(SDL_FRect const *rect, SDL_FPoint const *point);

bool fck_ui_button(SDL_Renderer *renderer, fck_mouse_state const *mouse, SDL_FRect const *button_rect,
                   fck_ui_button_style const *button_style, const char *label = nullptr);

bool fck_texture_load(SDL_Renderer *renderer, const char *relative_file_path, SDL_Texture **out_texture);

bool fck_font_asset_load(SDL_Renderer *renderer, const char *file_name, fck_font_asset *font_asset);
#endif // FCK_UI_INCLUDED