
#include <fck_apis.h>
#include <fck_hash.h>
#include <fck_input.h>
#include <fck_mouse.h>
#include <fck_os.h>
#include <fck_pkey.h>
#include <fck_plugins.h>
#include <fck_shader.h>
#include <fckc_assert.h>
#include <fckc_inttypes.h>
#include <sht_render.h>

#include "reflection.h"

#include <kll.h>
#include <kll_format.h>
#include <kll_malloc.h>

#include <stdio.h>

#include <ctype.h>
#include <stdlib.h>
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
#include "nuklear.h"
#include "nuklear_overview_demo.inl"

static struct nk_color fck_ui_cached_colour_table[NK_COLOR_COUNT];

typedef enum fck_nuklear_theme
{
	fck_nk_theme_black,
	fck_nk_theme_white,
	fck_nk_theme_ruta,
	fck_nk_theme_red,
	fck_nk_theme_blue,
	fck_nk_theme_dark,
	fck_nk_theme_dracula,
	fck_nk_theme_latte,
	fck_nk_theme_frappe,
	fck_nk_theme_macchiato,
	fck_nk_theme_mocha
} fck_nuklear_theme;

static struct nk_color *fck_ui_set_style(struct nk_context *ctx, enum fck_nuklear_theme theme)
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
		struct nk_color secondary = nk_rgba(75, 140, 0, 255);
		struct nk_color secondary_highlight = nk_rgba(95, 178, 0, 255);
		struct nk_color secondary_clicked = nk_rgba(115, 216, 0, 255);

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
	else if (theme == fck_nk_theme_latte)
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
	else if (theme == fck_nk_theme_frappe)
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
	else if (theme == fck_nk_theme_macchiato)
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
	else if (theme == fck_nk_theme_mocha)
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

static void purge_files(const char *pattern)
{
	char **paths;
	const fckc_size_t count = os->glob->executable(pattern, &paths);
	for (fckc_size_t index = 0; index < count; index++)
	{
		char *path = paths[index];
		os->fs->remove(path);
	}
	os->glob->free(paths);
}

static fck_api_registry *fck_api_registry_load(const char *path)
{
	// This badboy needs to get released
	const fck_shared_object so = os->so->load(path);
	fck_load_func *loader = (fck_load_func *)os->so->symbol(so, "fck_api_load");
	fck_api_registry *registry = (fck_api_registry *)loader(NULL, NULL);
	return registry;
}

static fck_plugins_api *fck_plugins_load(fck_api_registry *registry, const char *path)
{
	// This badboy needs to get released
	const fck_shared_object so = os->so->load(path);
	fck_load_func *loader = (fck_load_func *)os->so->symbol(so, "fck_plugins_load");
	fck_plugins_api *plugins = (fck_plugins_api *)loader(registry, NULL);
	return plugins;
}

static void load_config(int argc, char **argv)
{
	for (int index = 0; index < argc; index++)
	{
		char *value = argv[index];
		os->io->log(value);
	}
}

typedef struct app_screen
{
	float width;
	float height;
} app_screen;

typedef struct app_config
{
	fckc_i32 gradient;
	fckc_i32 is_sdf;
} app_config;

typedef struct app_vertex_transform
{
	float x;
	float y;
	float u;
	float v;
	float r;
	float g;
	float b;
	float a;
} app_vertex_transform;

typedef struct app_quad_transform
{
	float x;
	float y;
	float z;
	float rotation;
	float width;
	float height;
	float scale;
	float unused;
} app_quad_transform;

typedef struct app_line_transform
{
	float sx;
	float sy;
	float ex;
	float ey;
	float z;
	float thickness;
	float scale;
	float unused;
} app_line_transform;

// This will backlash. Try to keep them the same size, else the stride and all that stuff needs to stay opaque! We are lucky for now :D
typedef union app_shape_transform {
	app_vertex_transform vertex;
	app_quad_transform quad;
	app_line_transform line;
} app_shape_transform;

typedef struct app_quads
{
	app_quad_transform transforms[64];
	fckc_u32 count;
} app_quads;

static void app_quads_add(app_quads *quads, float x, float y)
{
	const app_quad_transform init_transform = {
		.x = x,
		.y = y,
		.z = 0.0f,
		.rotation = 0.0f,
		.width = 100.0f,
		.height = 100.0f,
		.scale = 1.0f,
	};

	app_quad_transform *transform = quads->transforms + quads->count;
	*transform = init_transform;
	quads->count = quads->count + 1;
}

typedef struct app_lines
{
	app_line_transform transforms[64];
	fckc_u32 count;
} app_lines;

static void app_lines_add(app_lines *quads, float sx, float sy, float ex, float ey, float thickness)
{
	const app_line_transform init_transform = {
		.sx = sx,
		.sy = sy,
		.ex = ex,
		.ey = ey,
		.z = 0.0f,
		.thickness = thickness,
		.scale = 1.0f,
	};

	app_line_transform *transform = quads->transforms + quads->count;
	*transform = init_transform;
	quads->count = quads->count + 1;
}

typedef enum app_graphics_shape
{
	app_shape_quad,
	app_shape_line,
	app_shape_count,
} app_graphics_shape;

static const char *app_graphics_shape_to_string(app_graphics_shape primitive)
{
	switch (primitive)
	{
	case app_shape_quad:
		return "app_shape_quad";
	case app_shape_line:
		return "app_shape_line";
	case app_shape_count:
		return "app_shape_line";
	}
	return "app_shape_unknown";
}

typedef enum app_graphics_style
{
	app_style_solid,
	app_style_textured,
	app_style_rounded,
	app_style_count,
} app_graphics_style;

static const char *app_graphic_material_to_string(app_graphics_style material)
{
	switch (material)
	{
	case app_style_solid:
		return "app_style_solid";
	case app_style_textured:
		return "app_style_textured";
	case app_style_rounded:
		return "app_style_rounded";
	case app_style_count:
		break;
	}
	return "app_style_unknown";
}

static fckc_size_t app_graphic_pipeline_bindings(app_graphics_shape primitive, app_graphics_style material,
                                                 sht_binding const **out_bindings)
{
	switch (primitive)
	{
	case app_shape_quad:
	case app_shape_line:
		switch (material)
		{
		case app_style_solid:
		case app_style_rounded:
			static const sht_binding bindings[] = {
				{.id = 0, .type = SHT_BINDING_UNIFORM, .stages = SHT_STAGE_VERTEX_SHADER},
				{.id = 1, .type = SHT_BINDING_STORAGE, .stages = SHT_STAGE_VERTEX_SHADER},
				{.id = 2, .type = SHT_BINDING_UNIFORM, .stages = SHT_STAGE_VERTEX_SHADER},
				{.id = 3, .type = SHT_BINDING_READ_ONLY_IMAGE, .stages = SHT_STAGE_FRAGMENT_SHADER},
				{.id = 5, .type = SHT_BINDING_UNIFORM, .stages = SHT_STAGE_VERTEX_SHADER | SHT_STAGE_FRAGMENT_SHADER},
				// TODO: Configuration binding
			};
			*out_bindings = bindings;
			return fck_arraysize(bindings);
		case app_style_textured:
			static const sht_binding textured_bindings[] = {
				{.id = 0, .type = SHT_BINDING_UNIFORM, .stages = SHT_STAGE_VERTEX_SHADER},
				{.id = 1, .type = SHT_BINDING_STORAGE, .stages = SHT_STAGE_VERTEX_SHADER},
				{.id = 2, .type = SHT_BINDING_UNIFORM, .stages = SHT_STAGE_VERTEX_SHADER},
				{.id = 3, .type = SHT_BINDING_READ_ONLY_IMAGE, .stages = SHT_STAGE_FRAGMENT_SHADER},
				{.id = 5, .type = SHT_BINDING_UNIFORM, .stages = SHT_STAGE_VERTEX_SHADER | SHT_STAGE_FRAGMENT_SHADER},
			};
			*out_bindings = textured_bindings;
			return fck_arraysize(textured_bindings);
		case app_style_count:
			break;
		}
		break;
	case app_shape_count:
		break;
	}
	return 0;
}

typedef struct app_property_string
{
	// If there is a need for a longer string, fuck you
	const char *value;
} app_property_string;

typedef enum app_property_type
{
	// What else do you need for now?
	app_property_float,
	app_property_int,
} app_property_type;

#define app_properties_capacity 16
#define app_property_blocks_capacity 8

typedef struct app_properties
{
	app_property_string keys[app_properties_capacity];
	app_property_type types[app_properties_capacity];
	fckc_size_t offsets[app_properties_capacity];
	// TODO: deal with capacity...
	fckc_u8 buffer[sizeof(float) * app_properties_capacity];
	fckc_size_t size;
} app_properties;

typedef struct app_property_block
{
	app_property_string name;
	app_properties properties;
} app_property_block;

typedef struct app_prroperty_blocks
{
	app_property_block values[app_property_blocks_capacity];
	fckc_size_t count;
} app_prroperty_blocks;

static void *app_property_structure_reserve(app_properties *properties, const char *name, fckc_size_t size)
{
	for (fckc_size_t index = 0; index < app_properties_capacity; index++)
	{
		app_property_string *key = properties->keys + index;
		if (key->value == NULL)
		{
			key->value = name;
		}
		if (strcmp(key->value, name) == 0)
		{
			app_property_type *type = properties->types + index;
			*type = app_property_float;

			// Do not forget alignup later! :)
			fckc_size_t *offset = properties->offsets + index;
			*offset = properties->size;
			properties->size = properties->size + size;

			fckc_u8 *dst = properties->buffer + *offset;
			memset(dst, 0, size);
			return dst;
		}
	}
	return NULL;
}

static void app_property_structure_set_float(app_properties *properties, const char *name, float value)
{
	void *destination = app_property_structure_reserve(properties, name, sizeof(value));
	if (destination)
	{
		memcpy(destination, &value, sizeof(value));
	}
}

static void app_property_structure_set_int(app_properties *properties, const char *name, int value)
{
	void *destination = app_property_structure_reserve(properties, name, sizeof(value));
	if (destination)
	{
		memcpy(destination, &value, sizeof(value));
	}
}

static void app_property_structure_upload(app_properties *properties, sht_driver driver, sht_bss bss)
{
	if (properties->size == 0)
	{
		return;
	}

	const sht_buffer_upload_desc upload = {
		.data = properties->buffer,
		.size = properties->size,
		.count = 1,
	};
	// TODO: Hardcoded binding!!!
	driver.vt->bss->upload_buffer(bss, 5, &upload);
}

typedef struct app_graphic_pipeline
{
	// ... I think with alignment this one is even generic enough to work as an arena or alloctor lol
	kll_arena *strings;

	sht_graphics_pipeline pipeline;
	sht_bss bss;

	// Rework properties
	app_properties properties;

	app_shape_transform *transforms;
	fckc_u32 count;
	fckc_u32 capacity;
} app_graphic_pipeline;

typedef struct app_graphics
{
	fck_shader_api *shader;
	sht_driver driver;
	app_graphic_pipeline values[app_shape_count][app_style_count];
} app_graphics;

static fckc_size_t app_bindings_add(sht_stage_flags stage, const fck_glsl_reflection_variable *var, sht_binding *bindings,
                                    fckc_size_t count, fckc_size_t capacity)
{
	fck_assert(var->binding >= 0);
	const fck_glsl_reflection_type *type = var->type;

	for (fckc_size_t index = 0; index < count; index++)
	{
		sht_binding *binding = bindings + index;
		if (binding->id == var->binding)
		{
			binding->stages = binding->stages | stage;
			return count;
		}
	}

	sht_binding *binding = bindings + count;
	if (sht_test(var->qualifiers, fck_glsl_reflection_declaration_qualifier_uniform))
	{
		fck_assert(count < capacity);
		binding->id = var->binding;
		binding->stages = stage;
		binding->type = SHT_BINDING_UNIFORM;
		if (glsl_reflection->is(type, "sampler2D"))
		{
			binding->type = SHT_BINDING_READ_ONLY_IMAGE;
		}
		count = count + 1;
	}
	if (sht_test(var->qualifiers, fck_glsl_reflection_declaration_qualifier_buffer))
	{
		fck_assert(count < capacity);
		binding->id = var->binding;
		binding->stages = stage;
		binding->type = SHT_BINDING_STORAGE;
		count = count + 1;
	}
	return count;
}

static void app_graphic_pipeline_create(fck_shader_api *shader, app_graphic_pipeline *gfx, sht_driver driver, const char *vertex_name,
                                        const char *vertex, const char *fragment_name, const char *fragment)
{
	fck_shader_compiler compiler = shader->create();

	fck_file vert_file = os->fs->open(vertex, "r");
	fck_shader_desc vert_desc = (fck_shader_desc){FCK_SHADER_VERTEX, vertex_name, "main"};

	fck_file frag_file = os->fs->open(fragment, "r");
	fck_shader_desc frag_desc = (fck_shader_desc){FCK_SHADER_FRAGMENT, fragment_name, "main"};
	fck_glsl_object vert = {0};
	fck_glsl_object frag = {0};
	vert = compiler.create_glsl_from_file(&compiler, &vert_desc, &vert_file);
	frag = compiler.create_glsl_from_file(&compiler, &frag_desc, &frag_file);

	// Setup Reflection
	const fckc_size_t bindings_capacity = 16;
	sht_binding bindings[bindings_capacity];
	fckc_size_t bindings_count = 0;

	{
		struct fck_glsl_reflection *reflection = glsl_reflection->reflect(vert.generic.source, fck_glsl_reflection_global);
		const fck_glsl_reflection_type *global = glsl_reflection->type_of(reflection, fck_glsl_reflection_global);
		const fck_glsl_reflection_variable *current = global->first;
		while (current)
		{
			if (current->binding >= 0)
			{
				bindings_count = app_bindings_add(SHT_STAGE_VERTEX_SHADER, current, bindings, bindings_count, bindings_capacity);
			}
			current = current->next;
		}
		glsl_reflection->free(reflection);
	}
	{
		struct fck_glsl_reflection *reflection = glsl_reflection->reflect(frag.generic.source, fck_glsl_reflection_global);
		const fck_glsl_reflection_type *global = glsl_reflection->type_of(reflection, fck_glsl_reflection_global);
		const fck_glsl_reflection_variable *current = global->first;
		while (current)
		{
			if (current->binding >= 0)
			{
				bindings_count = app_bindings_add(SHT_STAGE_FRAGMENT_SHADER, current, bindings, bindings_count, bindings_capacity);
			}
			current = current->next;
		}
		glsl_reflection->free(reflection);
	}

	// sht_binding const *bindings;
	sht_binding_desc binding_desc = {.bindings = bindings, .count = bindings_count};
	gfx->bss = driver.vt->bss->create(driver, &binding_desc);

	// TODO: Deprecate
	sht_vertex_desc vertex_desc = {
		.stride = 0,
		.bindings = NULL,
		.count = 0,
	};
	const sht_raster_desc raster_desc = {
		.cull_mode = SHT_CULL_MODE_NONE,
		.topology = SHT_TRIANGLE_LIST,
		.color = SHT_FORMAT_B8G8R8A8_UNORM,
		.depth = SHT_FORMAT_UNDEFINED, // SHT_FORMAT_D16_UNORM,
	};
	sht_graphic_desc graphic_desc = {
		.fragment = &frag.generic,
		.vertex = &vert.generic,
		.vertex_desc = &vertex_desc,
		.raster = raster_desc,
	};

	os->fs->close(vert_file);
	os->fs->close(frag_file);

	gfx->pipeline = driver.vt->graphics_pipeline->create(driver, gfx->bss, &graphic_desc);

	const fckc_size_t capacity = 64;
	gfx->transforms = (app_shape_transform *)kll_malloc(kll->system, sizeof(*gfx->transforms) * capacity);
	gfx->capacity = capacity;
	gfx->count = 0;

	compiler.destroy(&compiler, &vert.generic);
	compiler.destroy(&compiler, &frag.generic);
	compiler.shutdown(&compiler);
}

static void app_graphics_init(app_graphics *graphics, fck_shader_api *shader, sht_driver driver)
{
	graphics->shader = shader;
	graphics->driver = driver;
}

static void app_graphics_create(app_graphics *pipelines, app_graphics_shape primitive, app_graphics_style material, const char *vertex,
                                const char *fragment)
{
	fck_shader_compiler compiler = pipelines->shader->create();
	if (!pipelines->shader->is_ok(compiler))
	{
		return;
	}

	app_graphic_pipeline *gfx = &pipelines->values[primitive][material];

	fck_file vert_file = os->fs->open(vertex, "r");
	fck_shader_desc vert_desc = (fck_shader_desc){FCK_SHADER_VERTEX, app_graphics_shape_to_string(primitive), "main"};

	fck_file frag_file = os->fs->open(fragment, "r");
	fck_shader_desc frag_desc = (fck_shader_desc){FCK_SHADER_FRAGMENT, app_graphic_material_to_string(material), "main"};
	fck_glsl_object vert = {0};
	fck_glsl_object frag = {0};
	vert = compiler.create_glsl_from_file(&compiler, &vert_desc, &vert_file);
	frag = compiler.create_glsl_from_file(&compiler, &frag_desc, &frag_file);

	sht_binding const *bindings;
	const fckc_size_t binding_count = app_graphic_pipeline_bindings(primitive, material, &bindings);

	sht_binding_desc binding_desc = {.bindings = bindings, .count = binding_count};
	gfx->bss = pipelines->driver.vt->bss->create(pipelines->driver, &binding_desc);

	// TODO: Deprecate
	sht_vertex_desc vertex_desc = {
		.stride = 0,
		.bindings = NULL,
		.count = 0,
	};
	const sht_raster_desc raster_desc = {
		.cull_mode = SHT_CULL_MODE_NONE,
		.topology = SHT_TRIANGLE_LIST,
		.color = SHT_FORMAT_B8G8R8A8_UNORM,
		.depth = SHT_FORMAT_UNDEFINED,
	};
	sht_graphic_desc graphic_desc = {
		.fragment = &frag.generic,
		.vertex = &vert.generic,
		.vertex_desc = &vertex_desc,
		.raster = raster_desc,
	};

	os->fs->close(vert_file);
	os->fs->close(frag_file);

	gfx->pipeline = pipelines->driver.vt->graphics_pipeline->create(pipelines->driver, gfx->bss, &graphic_desc);

	const fckc_size_t capacity = 64;
	gfx->transforms = (app_shape_transform *)kll_malloc(kll->system, sizeof(*gfx->transforms) * capacity);
	gfx->capacity = capacity;
	gfx->count = 0;

	compiler.destroy(&compiler, &vert.generic);
	compiler.destroy(&compiler, &frag.generic);
	compiler.shutdown(&compiler);
}

static void app_graphics_add_line(app_graphics *graphics, app_graphics_style material, float sx, float sy, float ex, float ey,
                                  float thickness)
{
	app_graphic_pipeline *g = &graphics->values[app_shape_line][material];
	fck_assert(g->count < g->capacity);

	const app_line_transform init_transform = {
		.sx = sx,
		.sy = sy,
		.ex = ex,
		.ey = ey,
		.z = 0.0f,
		.thickness = thickness,
		.scale = 1.0f,
	};

	app_shape_transform *transform = g->transforms + g->count;
	transform->line = init_transform;
	g->count = g->count + 1;
}

static void app_graphics_add_quad(app_graphics *graphics, app_graphics_style material, float x, float y)
{
	app_graphic_pipeline *g = &graphics->values[app_shape_quad][material];
	fck_assert(g->count < g->capacity);

	const app_quad_transform init_transform = {
		.x = x,
		.y = y,
		.z = 0.0f,
		.rotation = 0.0f,
		.width = 100.0f,
		.height = 100.0f,
		.scale = 1.0f,
	};
	app_shape_transform *transform = g->transforms + g->count;
	transform->quad = init_transform;
	g->count = g->count + 1;
}

typedef struct app_nk
{
	struct nk_font_atlas atlas;
	struct nk_font *default_font;
	struct nk_draw_null_texture null_texture;

	struct nk_buffer commands;
	struct nk_context *ctx;

	sht_buffer indices[4];
	sht_sampler sampler;
	sht_image font_image;
	sht_image_view font_view;
	app_graphic_pipeline gfx;

	fckc_u64 time_last_frame;
} app_nk;

static sht_image app_nk_bake_font(sht_driver driver, const void *pixels, sht_format format, int width, int height)
{
	sht_memory *memory = driver.vt->memory(driver);
	sht_image_configuration config = {
		.format = format,
		.height = to_u32(height),
		.width = to_u32(width),
		.transfer = SHT_TRANSFER_TARGET,
		.usage = SHT_IMAGE_USAGE_SAMPLED,
	};

	sht_image image = memory->image->create(memory->bump, &config, SHT_MEMORY_GPU);
	if (!memory->image->is_ok(&image))
	{
		return image;
	}
	const fckc_size_t size = (fckc_size_t)width * height * 4;
	driver.vt->upload_image(driver, &image, pixels, size);
	return image;
}

static void app_nk_init(app_nk *nk, sht_driver driver, fck_shader_api *shader)
{
	sht_memory *memory = driver.vt->memory(driver);

	sht_buffer_configuration config = sht_buffer_retained(SHT_BUFFER_USAGE_INDEX, fck_megabytes(1));
	nk->indices[0] = memory->malloc(memory->bump, &config, SHT_MEMORY_CPU);
	nk->indices[1] = memory->malloc(memory->bump, &config, SHT_MEMORY_CPU);
	nk->indices[2] = memory->malloc(memory->bump, &config, SHT_MEMORY_CPU);
	nk->indices[3] = memory->malloc(memory->bump, &config, SHT_MEMORY_CPU);

	nk->sampler = driver.vt->create_sampler(driver, sht_filter_linear);

	nk->ctx = (struct nk_context *)kll_malloc(kll->system, sizeof(*nk->ctx));
	nk_init_default(nk->ctx, &nk->default_font->handle);
	nk->ctx->clip.userdata = nk_handle_ptr(0);

	nk_font_atlas_init_default(&nk->atlas);
	nk_font_atlas_begin(&nk->atlas);

	const struct nk_font_config font_config = nk_font_config(0);
	nk->default_font = nk_font_atlas_add_default(&nk->atlas, 13, &font_config);

	int default_font_width, default_font_height;

	const void *default_font_pixels = nk_font_atlas_bake(&nk->atlas, &default_font_width, &default_font_height, NK_FONT_ATLAS_RGBA32);

	const sht_format font_format = SHT_FORMAT_R8G8B8A8_UNORM;
	nk->font_image = app_nk_bake_font(driver, default_font_pixels, font_format, default_font_width, default_font_height);
	nk->font_view = memory->image->view(memory->bump, nk->font_image, font_format);

	nk_font_atlas_end(&nk->atlas, nk_handle_ptr(&nk->font_view), &nk->null_texture);

	nk_style_set_font(nk->ctx, &nk->default_font->handle);

	nk_buffer_init_default(&nk->commands);

	app_graphic_pipeline_create(shader, &nk->gfx, driver, "nuklear-vertex", fck_resource_path "vertex.vert", "nuklear-fragment",
	                            fck_resource_path "fragment.frag");
}

static void nuklear_example(struct nk_context *ctx)
{
	/* init gui state */

	enum
	{
		EASY,
		HARD
	};
	static int op = EASY;
	static float value = 0.6f;
	static int i = 20;

	if (nk_begin(ctx, "Show", nk_rect(0, 0, 220, 220), NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_CLOSABLE))
	{
		/* fixed widget pixel width */
		nk_layout_row_static(ctx, 30, 80, 1);
		if (nk_button_label(ctx, "button"))
		{
			os->io->log("Press");
			/* event handling */
		}
		nk_window_set_size(ctx, "Show", nk_vec2(200 + (int)(value * 600.0f), 220 + (int)(value * 600.0f)));

		/* fixed widget window ratio width */
		nk_layout_row_dynamic(ctx, 30, 2);
		if (nk_option_label(ctx, "easy", op == EASY))
			op = EASY;
		if (nk_option_label(ctx, "hard", op == HARD))
			op = HARD;

		/* custom widget pixel width */
		nk_layout_row_begin(ctx, NK_STATIC, 30, 2);
		{
			nk_layout_row_push(ctx, 50);
			nk_label(ctx, "Volume:", NK_TEXT_LEFT);
			nk_layout_row_push(ctx, 110);
			nk_slider_float(ctx, 0, &value, 1.0f, 0.1f);
		}
		nk_layout_row_end(ctx);
	}
	nk_end(ctx);
}

static void app_nk_draw(app_nk *nk, sht_driver driver, sht_command_buffer buffer, fckc_u32 frame_index)
{
	fckc_u64 now = os->chrono->ms();
	nk->ctx->delta_time_seconds = (float)(now - nk->time_last_frame) / 1000;
	nk->time_last_frame = now;

	sht_command_buffer_vt *command = driver.vt->command_buffer;

	static const struct nk_draw_vertex_layout_element vertex_layout[] = {
		{NK_VERTEX_POSITION, NK_FORMAT_FLOAT, offsetof(struct app_vertex_transform, x)},
		{NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, offsetof(struct app_vertex_transform, u)},
		{NK_VERTEX_COLOR, NK_FORMAT_R32G32B32A32_FLOAT, offsetof(struct app_vertex_transform, r)},
		{NK_VERTEX_LAYOUT_END}};

	struct nk_convert_config config;

	NK_MEMSET(&config, 0, sizeof(config));
	config.vertex_layout = vertex_layout;
	config.vertex_size = sizeof(app_vertex_transform);
	config.vertex_alignment = alignof(app_vertex_transform);
	config.tex_null = nk->null_texture;
	config.circle_segment_count = 22;
	config.curve_segment_count = 22;
	config.arc_segment_count = 22;
	config.global_alpha = 1.0f;
	config.shape_AA = NK_ANTI_ALIASING_OFF;
	config.line_AA = NK_ANTI_ALIASING_OFF;

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

	const app_vertex_transform *vertex_transforms = (const app_vertex_transform *)nk_buffer_memory_const(&vertices);

	const sht_swapchain swapchain = driver.vt->swapchain(driver);
	const sht_extent extent = swapchain.vt->extent(swapchain);

	{
		const app_screen screen = {
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

		const sht_image_upload_desc font_upload = {
			.samplers = nk->sampler,
			.views = nk->font_view,
		};

		driver.vt->bss->upload_buffer(nk->gfx.bss, 0, &screen_upload);
		driver.vt->bss->upload_buffer(nk->gfx.bss, 1, &vertices_upload);
		driver.vt->bss->upload_image(nk->gfx.bss, 2, &font_upload);
		command->bss(buffer, nk->gfx.bss);

		command->graphics_pipeline(buffer, nk->gfx.pipeline);
		// Dynamic geometry updates indices and vertices each frame
		command->index_buffer(buffer, &nk->indices[frame_index], 0);

		const struct nk_draw_command *cmd;
		fckc_u32 index_offset = 0;
		nk_draw_foreach(cmd, nk->ctx, &nk->commands)
		{
			if (!cmd->elem_count && !cmd->texture.ptr)
				continue;

			// driver->vt->bss->upload(ui->bss, 1, sht_upload_params{.view = ui->font.view, .sampler = ui->sampler});

			sht_scissor scissor;
			scissor.offset.x = to_u32(NK_MAX(cmd->clip_rect.x, 0.f));
			scissor.offset.y = to_u32(NK_MAX(cmd->clip_rect.y, 0.f));
			scissor.extent.width = to_u32(cmd->clip_rect.w);
			scissor.extent.height = to_u32(cmd->clip_rect.h);

			// float x = cmd->clip_rect.x;
			// float y = cmd->clip_rect.y;
			command->scissor(buffer, &scissor);
			command->draw_indexed(buffer, &(sht_draw_indexed_desc){
											  .first_index = index_offset,
											  .index_count = cmd->elem_count,
											  .instance_count = 1,
											  .first_instance = 0,
											  .vertex_offset = 0,
										  });
			index_offset = index_offset + cmd->elem_count;
		}
		nk_buffer_free(&vertices);
		nk_buffer_free(&elements);
	}
}

typedef enum app_nk_menu_item_type
{
	app_nk_menu_item_bool,
	app_nk_menu_item_button,
} app_nk_menu_item_type;

struct app_nk_menu_item;
typedef struct app_nk_menu_item
{
	struct app_nk_menu_item *next;
	const char *name;
	app_nk_menu_item_type type;
	int value;
} app_nk_menu_item;

struct app_nk_pie_item;
typedef struct app_nk_pie_item
{
	struct app_nk_pie_item *next;
	struct app_nk_pie_item *parent;

	struct app_nk_pie_item *child_items;
	struct app_nk_pie_item *child_items_last;

	const char *name;
	int value;
} app_nk_pie_item;

static int app_nk_menu_item_happend(app_nk_menu_item *item)
{
	int value = item->value;
	if (item->type == app_nk_menu_item_button)
	{
		item->value = 0;
	}
	return value;
}

static int app_nk_pie_item_happend(app_nk_pie_item *item)
{
	int value = item->value;
	item->value = 0;
	return value;
}

typedef struct app_nk_os_window_state
{
	fckc_u32 minimise : 1;
	fckc_u32 close : 1;
	fckc_u32 menu : 1;
	fckc_u32 body : 1;
	fckc_u32 pie : 1;
} app_nk_os_window_state;

typedef struct app_nk_os_window
{
	fck_window window;
	struct nk_context *ctx;

	app_nk_menu_item *menu_items;
	app_nk_menu_item *menu_items_last;

	struct nk_vec2 pie_position;
	app_nk_pie_item *last_hovered;
	app_nk_pie_item *pie_items;
	app_nk_pie_item *pie_items_last;

	app_nk_os_window_state state;
} app_nk_os_window;

static app_nk_os_window app_nk_os_window_create(fck_window window, struct nk_context *ctx)
{
	app_nk_os_window os_window = {.window = window, .ctx = ctx};
	return os_window;
}

static void app_nk_os_window_add_menu_item(app_nk_os_window *window, app_nk_menu_item *menu_item)
{
	fck_assert(menu_item->next == NULL);

	if (window->menu_items == NULL)
	{
		window->menu_items = menu_item;
		window->menu_items_last = menu_item;
	}
	else
	{
		window->menu_items_last->next = menu_item;
		window->menu_items_last = menu_item;
	}
}

static void app_nk_os_window_add_pie_item(app_nk_os_window *window, app_nk_pie_item *pie_item)
{
	fck_assert(pie_item->next == NULL);

	if (window->pie_items == NULL)
	{
		window->pie_items = pie_item;
		window->pie_items_last = pie_item;
	}
	else
	{
		window->pie_items_last->next = pie_item;
		window->pie_items_last = pie_item;
	}
}

static void app_nk_pie_item_add_child(app_nk_pie_item *pie_item, app_nk_pie_item *child_item)
{
	fck_assert(child_item->next == NULL);

	child_item->parent = pie_item;

	if (pie_item->child_items == NULL)
	{
		pie_item->child_items = child_item;
		pie_item->child_items_last = child_item;
	}
	else
	{
		pie_item->child_items_last->next = child_item;
		pie_item->child_items_last = child_item;
	}
}

static app_nk_pie_item *app_nk_pie_item_is_part_of(app_nk_pie_item *item, app_nk_pie_item *target)
{
	app_nk_pie_item *current = item;
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

static app_nk_pie_item *app_nk_os_window_pie_fan(app_nk_os_window *os_window, app_nk_pie_item *active, app_nk_pie_item *pie_items,
                                                 struct nk_vec2 center, float offset_angle, float angle_step, float radius_offset,
                                                 float radius)
{
	int count = 0;
	app_nk_pie_item *current = pie_items;
	while (current)
	{
		count = count + 1;
		current = current->next;
	}

	struct nk_context *ctx = os_window->ctx;
	struct nk_input *input = &ctx->input;
	const struct nk_vec2 mouse_pos = input->mouse.pos;

	const float dx = mouse_pos.x - center.x;
	const float dy = mouse_pos.y - center.y;

	const float dist = sqrt(dx * dx + dy * dy);
	int hovered = -1;

	app_nk_pie_item *current_hovered = NULL;

	angle_step = angle_step / (float)count;
	if (dist > radius_offset && dist < radius)
	{
		float mouse_angle = nk_atan2(dy, dx);
		if (mouse_angle < 0.0f)
		{
			mouse_angle += 2.0f * NK_PI;
		}
		mouse_angle = mouse_angle - offset_angle;
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
			if (app_nk_pie_item_is_part_of(os_window->last_hovered, current))
			{
				struct app_nk_pie_item *children = current->child_items;
				if (children)
				{
					struct app_nk_pie_item *result = app_nk_os_window_pie_fan(os_window, os_window->last_hovered, children, center,
					                                                          start_angle, angle_step, radius, radius * 1.25f);
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
			struct app_nk_pie_item *children = current->child_items;
			if (children)
			{
				// Preview Children
				app_nk_os_window_pie_fan(os_window, active, children, center, start_angle, angle_step, radius, radius * 1.25f);
			}
		}

		struct nk_command_buffer *canvas = nk_window_get_canvas(os_window->ctx);
		nk_fill_arc(canvas, center.x, center.y, radius, start_angle, start_angle + angle_step, slice_color);
		nk_stroke_arc(canvas, center.x, center.y, radius, start_angle, start_angle + angle_step, 1.5f, nk_rgba(100, 100, 100, 255));

		float text_angle = start_angle + (angle_step / 2.0f);
		const struct nk_user_font *font = ctx->style.font;
		struct nk_vec2 text_pos;
		text_pos.x = center.x + (((radius_offset * 0.35) + (radius * 0.65f)) * nk_cos(text_angle));
		text_pos.y = center.y + (((radius_offset * 0.35) + (radius * 0.65f)) * nk_sin(text_angle));

		float text_width = font->width(font->userdata, font->height, current->name, nk_strlen(current->name));
		text_pos.x -= text_width / 2.0f;
		text_pos.y -= font->height / 2.0f;

		nk_draw_text(canvas, nk_rect(text_pos.x, text_pos.y, text_width, font->height), current->name, nk_strlen(current->name), font,
		             nk_rgba(0, 0, 0, 0), nk_rgba(255, 255, 255, 255));

		index = index + 1;
		current = current->next;
	}

	return current_hovered;
}

static app_nk_pie_item *app_nk_os_window_pie(app_nk_os_window *os_window, float radius)
{
	struct nk_context *ctx = os_window->ctx;

	struct nk_command_buffer *canvas = nk_window_get_canvas(ctx);
	struct nk_input *input = &ctx->input;

	if (!nk_input_is_mouse_released(input, NK_BUTTON_RIGHT))
	{
		if (!nk_input_is_mouse_down(input, NK_BUTTON_RIGHT))
		{
			app_nk_pie_item* selected = os_window->last_hovered;
			os_window->last_hovered = NULL;
			os_window->state.pie = 0;
			return selected;
		}
	}

	if (os_window->state.pie == 0)
	{
		os_window->pie_position = input->mouse.pos;
	}
	os_window->state.pie = 1;

	const struct nk_vec2 center = os_window->pie_position;
	app_nk_pie_item *pie_items = os_window->pie_items;
	if (pie_items == NULL)
	{
		return NULL;
	}

	int count = 0;
	app_nk_pie_item *current = pie_items;
	while (current)
	{
		count = count + 1;
		current = current->next;
	}

	const float angle_step = (2.0f * NK_PI) / (float)count;
	const struct nk_vec2 mouse_pos = input->mouse.pos;

	const float dx = mouse_pos.x - center.x;
	const float dy = mouse_pos.y - center.y;
	const float dist = sqrt(dx * dx + dy * dy);

	int hovered = -1;
	if (dist > 15.0f && dist < radius)
	{
		float mouse_angle = nk_atan2(dy, dx);
		if (mouse_angle < 0.0f)
		{
			mouse_angle += 2.0f * NK_PI;
		}
		hovered = (int)((mouse_angle + (angle_step / 2.0f)) / angle_step) % count;
	}

	app_nk_pie_item *selected = NULL;
	app_nk_pie_item *current_hovered = NULL;
	current = pie_items;
	int index = 0;
	while (current)
	{
		float start_angle = ((float)index * angle_step) - (angle_step / 2.0f);
		if (hovered == -1)
		{
			if (app_nk_pie_item_is_part_of(os_window->last_hovered, current))
			{
				struct app_nk_pie_item *children = current->child_items;
				if (children)
				{
					struct app_nk_pie_item *result = app_nk_os_window_pie_fan(os_window, os_window->last_hovered, children, center,
					                                                          start_angle, angle_step, radius, radius * 1.5f);
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

			/*if (nk_input_is_mouse_released(input, NK_BUTTON_RIGHT))
			{
				selected = current;
			}*/

			struct app_nk_pie_item *children = current->child_items;
			if (children)
			{
				// Preview Children
				app_nk_os_window_pie_fan(os_window, current, children, center, start_angle, angle_step, radius, radius * 1.5f);
			}
			current_hovered = current;
		}

		{

			float text_angle = start_angle + (angle_step / 2.0f);
			const struct nk_user_font *font = ctx->style.font;
			struct nk_vec2 text_pos;
			text_pos.x = center.x + (radius * 0.65f) * nk_cos(text_angle);
			text_pos.y = center.y + (radius * 0.65f) * nk_sin(text_angle);

			nk_fill_arc(canvas, center.x, center.y, radius, start_angle, start_angle + angle_step, slice_color);
			nk_stroke_arc(canvas, center.x, center.y, radius, start_angle, start_angle + angle_step, 1.5f, nk_rgba(100, 100, 100, 255));

			float text_width = font->width(font->userdata, font->height, current->name, nk_strlen(current->name));

			text_pos.x -= text_width / 2.0f;
			text_pos.y -= font->height / 2.0f;

			nk_draw_text(canvas, nk_rect(text_pos.x, text_pos.y, text_width, font->height), current->name, nk_strlen(current->name), font,
			             nk_rgba(0, 0, 0, 0), nk_rgba(255, 255, 255, 255));
		}
		index = index + 1;
		current = current->next;
	}

	os_window->last_hovered = current_hovered;

	nk_fill_circle(canvas, nk_rect(center.x - 15.0f, center.y - 15.0f, 30.0f, 30.0f), nk_rgba(30, 30, 30, 255));
	nk_stroke_circle(canvas, nk_rect(center.x - 15.0f, center.y - 15.0f, 30.0f, 30.0f), 1.5f, nk_rgba(100, 100, 100, 255));
	return selected;
}

static int app_nk_os_window_begin(app_nk_os_window *os_window)
{
	const fck_window window = os_window->window;
	struct nk_context *ctx = os_window->ctx;
	app_nk_menu_item *menu_items = os_window->menu_items;

	const char *title = os->win->title(window, NULL);

	int window_width, window_height;
	os->win->size(window, &window_width, &window_height);

	const fck_window_configuration *configuration = os->win->configuration(window, NULL);

	nk_style_push_style_item(ctx, &ctx->style.window.fixed_background, ctx->style.button.normal);
	nk_style_push_vec2(ctx, &ctx->style.window.padding, nk_vec2(0, 0));
	nk_style_push_vec2(ctx, &ctx->style.window.group_padding, nk_vec2(0, 0));
	nk_style_push_vec2(ctx, &ctx->style.window.spacing, nk_vec2(0, 0));

	os_window->state.menu = 0;
	os_window->state.close = 0;
	os_window->state.minimise = 0;

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
			app_nk_menu_item *current = menu_items;
			while (current)
			{
				switch (current->type)
				{
				case app_nk_menu_item_bool: {

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
				case app_nk_menu_item_button:
					current->value = to_int(nk_menu_item_label(ctx, current->name, NK_TEXT_LEFT));
					break;
				}
				current = current->next;
			}
			os_window->state.menu = 1;
			nk_menu_end(ctx);
		}
		nk_style_pop_vec2(ctx);

		nk_style_push_vec2(ctx, &ctx->style.button.padding, nk_vec2(12.5f, 12.5f));
		const enum nk_symbol_type symbol = os_window->state.body ? NK_SYMBOL_TRIANGLE_DOWN : NK_SYMBOL_TRIANGLE_UP_OUTLINE;
		if (nk_button_symbol(ctx, symbol))
		{
			os_window->state.body = ~os_window->state.body;
		}
		nk_style_pop_vec2(ctx);

		ctx->style.menu_button = menu_button_style;

		nk_label(ctx, title, NK_TEXT_ALIGN_CENTERED | NK_TEXT_ALIGN_MIDDLE);

		if (nk_button_symbol(ctx, NK_SYMBOL_MINUS))
		{
			os_window->state.minimise = 1;
		}

		if (nk_button_symbol(ctx, NK_SYMBOL_X))
		{
			os_window->state.close = 1;
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
	if (os_window->state.body)
	{
		if (os_window->state.menu)
		{
			// NOTE: This resets the input so it does not interfere with the body's nk_window
			nk_input_begin(os_window->ctx);
			nk_input_end(os_window->ctx);
		}
		if (nk_begin(ctx, "Window Body", nk_rect(0, configuration->title_bar_height, window_width, height), NK_WINDOW_BACKGROUND))
		{
			return os_window->state.body;
		}
		// We should not end up here
	}
	return os_window->state.body;
}

static void app_nk_os_window_end(app_nk_os_window *os_window)
{
	if (os_window->state.body)
	{
		nk_end(os_window->ctx);
	}
}

int main(int argc, char **argv)
{
	load_config(argc, argv);

	purge_files("temp-*.dll");

	fck_api_registry *registry = fck_api_registry_load("fck-api.dll");
	fck_plugins_api *plugins = fck_plugins_load(registry, "fck-plugins.dll");

	plugins->root(os->fs->executable());

	const char *current = NULL;
	while ((current = plugins->unloaded(current)))
	{
		plugins->load(current);
	}

	current = NULL;
	while ((current = plugins->loaded(current)))
	{
		plugins->load(current);
	}

	fck_input *input = (fck_input *)registry->find(fck_input_api_name);
	sht_render_api *render = (sht_render_api *)registry->find(sht_render_api_name);
	fck_shader_api *shader = (fck_shader_api *)registry->find(fck_shader_api_name);
	fck_input_source *mouse = NULL;
	{
		fck_input_source **sources;
		const fckc_size_t count = input->sources(&sources);
		for (fckc_size_t index = 0; index < count; index++)
		{
			fck_input_source *source = sources[index];
			if (source->type == fck_input_source_mouse)
			{
				mouse = source;
				break;
			}
		}
	}
	fck_assert(mouse);

	fck_window window = os->win->create("Vulkan Test Application", 1280, 720);
	int window_width, window_height;
	os->win->size(window, &window_width, &window_height);

	const fck_window_configuration config = {
		.title_bar_height = 35.0f,
		.resize_line_width = 8.0f,
		.menu_area_width = 35.0f * 2.0f,
		.button_area_width = 35.0f * 2.0f,
	};
	os->win->configuration(window, &config);

	// We have to do this a bit smarter... Maybe not now
	// os->win->text_input_start(window);

	const sht_instance instance = render->load(sht_header_version);
	if (!render->is_ok(instance))
	{
		return 0;
	}

	sht_driver driver = instance.vt->start(instance, &window);
	if (!instance.vt->is_ok(driver))
	{
		return 0;
	}
	sht_memory *memory = driver.vt->memory(driver);
	sht_swapchain swapchain = driver.vt->swapchain(driver);
	sht_command_buffer_vt *command = driver.vt->command_buffer;

	app_nk nk;
	app_nk_init(&nk, driver, shader);
	fck_ui_set_style(nk.ctx, fck_nk_theme_ruta);

	app_nk_menu_item help_menu_item = {
		.type = app_nk_menu_item_button,
		.name = "Help",
	};
	app_nk_menu_item about_menu_item = {
		.type = app_nk_menu_item_button,
		.name = "About",
	};
	app_nk_menu_item setting_menu_item = {
		.type = app_nk_menu_item_bool,
		.name = "Setting",
	};

	app_nk_pie_item copy_pie_item = {
		.name = "Copy",
	};
	app_nk_pie_item paste_pie_item = {
		.name = "Paste",
	};
	app_nk_pie_item duplicate_pie_item = {
		.name = "Duplicate",
	};
	app_nk_pie_item delete_pie_item = {
		.name = "Delete",
	};
	app_nk_pie_item properties_pie_item = {
		.name = "Properties",
	};

	app_nk_pie_item properties_child0_pie_item = {
		.name = "Extra",
	};

	app_nk_pie_item properties_child_child_pie_item = {
		.name = "X",
	};

	app_nk_pie_item properties_child_child_child_pie_item = {
		.name = "X",
	};

	app_nk_pie_item properties_child1_pie_item = {
		.name = "Extra",
	};

	app_nk_os_window nk_os_window = app_nk_os_window_create(window, nk.ctx);
	app_nk_os_window_add_menu_item(&nk_os_window, &help_menu_item);
	app_nk_os_window_add_menu_item(&nk_os_window, &about_menu_item);
	app_nk_os_window_add_menu_item(&nk_os_window, &setting_menu_item);

	app_nk_os_window_add_pie_item(&nk_os_window, &copy_pie_item);
	app_nk_os_window_add_pie_item(&nk_os_window, &paste_pie_item);
	app_nk_os_window_add_pie_item(&nk_os_window, &duplicate_pie_item);
	app_nk_os_window_add_pie_item(&nk_os_window, &delete_pie_item);
	app_nk_os_window_add_pie_item(&nk_os_window, &properties_pie_item);

	app_nk_pie_item_add_child(&properties_pie_item, &properties_child0_pie_item);
	app_nk_pie_item_add_child(&properties_pie_item, &properties_child1_pie_item);

	app_nk_pie_item_add_child(&properties_child0_pie_item, &properties_child_child_pie_item);
	app_nk_pie_item_add_child(&properties_child_child_pie_item, &properties_child_child_child_pie_item);

	sht_graphics_pipeline graphic_pipelines;

	sht_elements indices = {0};

	sht_sampler sampler = {0};
	sht_image texture_image = {0};
	sht_image_view texture_view = {0};

	{
		sampler = driver.vt->create_sampler(driver, sht_filter_linear);
		texture_image = memory->image->create(memory->bump,
		                                      &(sht_image_configuration){
												  .format = SHT_FORMAT_R8G8B8A8_UNORM,
												  .width = 4,
												  .height = 1,
												  .transfer = SHT_TRANSFER_TARGET,
												  .usage = SHT_IMAGE_USAGE_SAMPLED,
											  },
		                                      SHT_MEMORY_GPU);
		texture_view = memory->image->view(memory->bump, texture_image, SHT_FORMAT_R8G8B8A8_UNORM);

		fckc_u32 pixels[] = {0xFF0000FF, 0xFF00FF00, 0xFFFF0000, 0xFFFFFFFF};
		driver.vt->upload_image(driver, &texture_image, pixels, sizeof(pixels));
	}

	// sht_image depth_image = {0};
	// sht_image_view depth_view = {0};
	//{
	//	sht_extent extent = swapchain.vt->extent(swapchain);
	//	sht_image_configuration config = (sht_image_configuration){
	//		.format = SHT_FORMAT_D16_UNORM,
	//		.width = extent.width,
	//		.height = extent.height,
	//		.transfer = SHT_TRANSFER_RETAINED,
	//		.usage = SHT_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT,
	//	};

	//	depth_image = memory->image->create(memory->bump, &config, SHT_MEMORY_GPU);
	//	depth_view = memory->image->view(memory->bump, depth_image, SHT_FORMAT_UNDEFINED);
	//}

	{
		fckc_u32 index_data[] = {0, 1, 2, 1, 3, 2};
		indices.count = fck_arraysize(index_data);
		indices.buffer = memory->malloc(memory->bump, &sht_buffer_target(SHT_BUFFER_USAGE_INDEX, sizeof(index_data)), SHT_MEMORY_GPU);
		driver.vt->upload_buffer(driver, &indices.buffer, index_data, sizeof(index_data));
	}

	app_graphics graphics = {0};
	app_graphics_init(&graphics, shader, driver);
	app_graphics_create(&graphics, app_shape_quad, app_style_solid, fck_resource_path "quad.vert", fck_resource_path "solid.frag");
	app_graphics_create(&graphics, app_shape_quad, app_style_textured, fck_resource_path "quad.vert", fck_resource_path "textured.frag");
	app_graphics_create(&graphics, app_shape_quad, app_style_rounded, fck_resource_path "quad.vert", fck_resource_path "round.frag");
	app_graphics_create(&graphics, app_shape_line, app_style_solid, fck_resource_path "line.vert", fck_resource_path "solid.frag");
	app_graphics_create(&graphics, app_shape_line, app_style_textured, fck_resource_path "line.vert", fck_resource_path "textured.frag");
	app_graphics_create(&graphics, app_shape_line, app_style_rounded, fck_resource_path "line.vert", fck_resource_path "round.frag");

	app_graphics_add_quad(&graphics, app_style_textured, 0.0f, 0.0f);
	app_graphics_add_quad(&graphics, app_style_solid, 300.0f, 0.0f);
	app_graphics_add_quad(&graphics, app_style_rounded, 500.0f, 0.0f);

	app_graphics_add_line(&graphics, app_style_solid, -50.0f, -50.0f, 50.0f, 50.0f, 16.0f);
	app_graphics_add_line(&graphics, app_style_solid, 50.0f, -50.0f, -50.0f, 50.0f, 16.0f);

	app_graphics_add_line(&graphics, app_style_textured, -300.0f, 0.0f, -350.0f, 100.0f, 24.0f);
	app_graphics_add_line(&graphics, app_style_rounded, -500.0f, 0.0f, -550.0f, 100.0f, 28.0f);

	// How do I scale this to vertex and fragment shader stuff...
	// HMMMMMMM
	for (fckc_size_t index = 0; index < app_shape_count; index++)
	{
		app_properties *properties = &graphics.values[index][app_style_rounded].properties;
		app_property_structure_set_float(properties, "roundness", 0.5f);
	}

	int is_running = 1;
	while (is_running)
	{
		// TODO: Make render-vk hotreloadable :)
		// How hard can it be?
		plugins->hotreload();

		nk_input_begin(nk.ctx);

		fck_input_event events[32] = {0};
		const fckc_size_t result = input->events(events, fck_arraysize(events));
		for (fckc_size_t index = 0; index < result; index++)
		{
			fck_input_event *e = events + index;
			if (e->source->type == fck_input_source_keyboard)
			{
				switch (e->description->id)
				{
				case fck_pkey_escape:
					is_running = 0;
					break;
				case fck_pkey_return:
					nk_input_key(nk.ctx, NK_KEY_ENTER, e->data.scalar > 0.0f);
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
				case fck_mouse_wheel:
					nk_input_scroll(nk.ctx, nk_vec2(e->data.floats[0], e->data.floats[1]));
					break;
				default:
					break;
				}
				continue;
			}

			if (e->source->type == fck_input_source_text)
			{
				nk_input_unicode(nk.ctx, e->data.unicode);
				continue;
			}
		}

		{
			fckc_u32 ids[] = {fck_mouse_left, fck_mouse_middle, fck_mouse_right, fck_mouse_position, fck_mouse_wheel};
			fck_input_data states[fck_arraysize(ids)];
			const fckc_size_t result = mouse->states(0, ids, states, fck_arraysize(ids));
			fck_input_data *left = states + 0;
			fck_input_data *middle = states + 1;
			fck_input_data *right = states + 2;
			fck_input_data *position = states + 3;
			fck_input_data *wheel = states + 4;
			nk_input_button(nk.ctx, NK_BUTTON_LEFT, nk.ctx->input.mouse.pos.x, nk.ctx->input.mouse.pos.y, left->scalar > 0.0f);
			nk_input_button(nk.ctx, NK_BUTTON_MIDDLE, position->floats[0], position->floats[1], middle->scalar > 0.0f);
			nk_input_button(nk.ctx, NK_BUTTON_RIGHT, position->floats[0], position->floats[1], right->scalar > 0.0f);
			nk_input_button(nk.ctx, NK_BUTTON_RIGHT, position->floats[0], position->floats[1], right->scalar > 0.0f);
			nk_input_motion(nk.ctx, position->floats[0], position->floats[1]);
		}
		nk_input_end(nk.ctx);

		nk_clear(nk.ctx);
		nk_buffer_clear(&nk.commands);

		{
			if (app_nk_os_window_begin(&nk_os_window))
			{
				app_nk_pie_item* item = app_nk_os_window_pie(&nk_os_window, 160.0f);
				if (item)
				{
					os->io->log("Pressed: %s", item->name);
					item->value = 1;
				}
				// nk_demo_overview(nk.ctx);
			}
			app_nk_os_window_end(&nk_os_window);

			if (app_nk_pie_item_happend(&copy_pie_item))
			{
				os->io->log("Copy!");
			}
			if (app_nk_menu_item_happend(&setting_menu_item))
			{
				os->io->log("On");
			}

			if (app_nk_menu_item_happend(&about_menu_item))
			{
				os->io->log("Made by David");
			}
			if (app_nk_menu_item_happend(&setting_menu_item))
			{
				os->io->log("On");
			}

			if (nk_os_window.state.close)
			{
				is_running = 0;
			}
		}

		memory->reset(memory->temp);

		fckc_u32 frame_index;
		const sht_image_view color_target = swapchain.vt->wait_and_acquire(swapchain, &frame_index);
		if (swapchain.vt->is_ok(swapchain, frame_index))
		{
			const sht_extent extent = swapchain.vt->extent(swapchain);

			const sht_command_buffer command_buffer = command->acquire(driver, frame_index);
			if (command->is_ok(command_buffer))
			{
				sht_render_desc desc = {
					.colour = {.view = color_target, .load_op = SHT_CLEAR, .store_op = SHT_STORE, .clear_value = {0.0f, 0.0f, 0.2f, 1.0f}},
					//.depth = {.view = depth_view, .load_op = SHT_CLEAR, .store_op = SHT_DONT_CARE, .clear_value = 0.0f},
				};

				app_screen screen = {
					.width = (float)extent.width,
					.height = (float)extent.height,
				};

				const sht_render_pass render_pass = command->render_pass->begin(command_buffer, &desc);

				sht_viewport viewport;
				viewport.offset.x = 0.0f;
				viewport.offset.y = 0.0f;
				viewport.depth.min = (float)0.0f;
				viewport.depth.max = (float)1.0f;

				sht_scissor scissor;
				scissor.offset.x = 0;
				scissor.offset.y = 0;
				scissor.extent = viewport.extent = swapchain.vt->extent(swapchain);

				if (command->render_pass->is_ok(render_pass))
				{
					command->viewport(command_buffer, &viewport);
					command->scissor(command_buffer, &scissor);

					command->index_buffer(command_buffer, &indices.buffer, 0);

					for (fckc_size_t shape_index = 0; shape_index < app_shape_count; shape_index++)
					{
						for (fckc_size_t style_index = 0; style_index < app_style_count; style_index++)
						{
							app_graphic_pipeline *graphic = &graphics.values[shape_index][style_index];
							if (driver.vt->graphics_pipeline->is_ok(graphic->pipeline))
							{
								if (graphic->count > 0)
								{
									const sht_buffer_upload_desc screen_upload = {
										.data = &screen,
										.size = sizeof(screen),
										.count = 1,
									};
									const sht_buffer_upload_desc quads_upload = {
										.data = graphic->transforms,
										.size = sizeof(*graphic->transforms),
										.count = graphic->count,
									};

									app_config config = {.gradient = 0, .is_sdf = style_index == app_style_rounded};

									const sht_buffer_upload_desc config_upload = {
										.data = &config,
										.size = sizeof(config),
										.count = 1,
									};
									const sht_image_upload_desc image_upload = {
										.views = texture_view,
										.samplers = sampler,
									};

									driver.vt->bss->upload_buffer(graphic->bss, 0, &screen_upload);
									driver.vt->bss->upload_buffer(graphic->bss, 1, &quads_upload);
									driver.vt->bss->upload_buffer(graphic->bss, 2, &config_upload);
									driver.vt->bss->upload_image(graphic->bss, 3, &image_upload);

									app_property_structure_upload(&graphic->properties, driver, graphic->bss);

									command->bss(command_buffer, graphic->bss);

									command->graphics_pipeline(command_buffer, graphic->pipeline);

									sht_draw_indexed_desc indexed = {
										.first_index = 0,
										.first_instance = 0,
										.index_count = to_u32(indices.count),
										.instance_count = graphic->count,
										.vertex_offset = 0,
									};
									command->draw_indexed(command_buffer, &indexed);
								}
							}
						}
					}

					app_nk_draw(&nk, driver, command_buffer, frame_index);

					command->render_pass->end(command_buffer);
				}
				command->submit(command_buffer, SHT_QUEUE_GRAPHIC);
			}
		}
		// driver.vt->idle(driver);
	}

	plugins->shutdown();
	purge_files("temp-*.dll");

	os->win->destroy(window);

	return 0;
}
