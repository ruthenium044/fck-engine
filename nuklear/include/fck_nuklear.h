#ifndef FCK_NUKLEAR_H_INCLUDED
#define FCK_NUKLEAR_H_INCLUDED

#include <fckc_inttypes.h>

// Even if I move away from nuklear, I will keep close to its API
#define fck_nuklear_api_name "fck-nuklear"

typedef enum fck_nuklear_theme
{
	fck_nk_theme_white,
	fck_nk_theme_ruta,
	fck_nk_theme_red,
	fck_nk_theme_blue,
	fck_nk_theme_dark,
	fck_nk_theme_dracula,
	fck_nk_theme_latte,
	fck_nk_theme_frappe,
	fck_nk_theme_macchiato,
	fck_nk_theme_mocha,
	fck_nk_theme_count
} fck_nuklear_theme;

// TODO: make them lowercase
typedef enum fck_nk_style_colors
{
	FCK_NK_COLOR_TEXT,
	FCK_NK_COLOR_WINDOW,
	FCK_NK_COLOR_HEADER,
	FCK_NK_COLOR_BORDER,
	FCK_NK_COLOR_BUTTON,
	FCK_NK_COLOR_BUTTON_HOVER,
	FCK_NK_COLOR_BUTTON_ACTIVE,
	FCK_NK_COLOR_TOGGLE,
	FCK_NK_COLOR_TOGGLE_HOVER,
	FCK_NK_COLOR_TOGGLE_CURSOR,
	FCK_NK_COLOR_SELECT,
	FCK_NK_COLOR_SELECT_ACTIVE,
	FCK_NK_COLOR_SLIDER,
	FCK_NK_COLOR_SLIDER_CURSOR,
	FCK_NK_COLOR_SLIDER_CURSOR_HOVER,
	FCK_NK_COLOR_SLIDER_CURSOR_ACTIVE,
	FCK_NK_COLOR_PROPERTY,
	FCK_NK_COLOR_EDIT,
	FCK_NK_COLOR_EDIT_CURSOR,
	FCK_NK_COLOR_COMBO,
	FCK_NK_COLOR_CHART,
	FCK_NK_COLOR_CHART_COLOR,
	FCK_NK_COLOR_CHART_COLOR_HIGHLIGHT,
	FCK_NK_COLOR_SCROLLBAR,
	FCK_NK_COLOR_SCROLLBAR_CURSOR,
	FCK_NK_COLOR_SCROLLBAR_CURSOR_HOVER,
	FCK_NK_COLOR_SCROLLBAR_CURSOR_ACTIVE,
	FCK_NK_COLOR_TAB_HEADER,
	FCK_NK_COLOR_KNOB,
	FCK_NK_COLOR_KNOB_CURSOR,
	FCK_NK_COLOR_KNOB_CURSOR_HOVER,
	FCK_NK_COLOR_KNOB_CURSOR_ACTIVE,
	FCK_NK_COLOR_COUNT
} fck_nk_style_colors;

typedef struct fck_nk_rect
{
	float x;
	float y;
	float w;
	float h;
} fck_nk_rect;

typedef struct fck_nk_colour
{
	fckc_u8 r;
	fckc_u8 g;
	fckc_u8 b;
	fckc_u8 a;
} fck_nk_colour;

typedef struct fck_nk
{
	void *handle;
} fck_nk;

typedef enum fck_nk_hamburger_item_type
{
	fck_nk_hamburger_item_bool,
	fck_nk_hamburger_item_button,
} fck_nk_hamburger_item_type;

struct fck_nk_hamburger_item;
typedef struct fck_nk_hamburger_item
{
	struct fck_nk_hamburger_item *next;
	const char *name;
	fck_nk_hamburger_item_type type;
	int value;
} fck_nk_hamburger_item;

struct fck_nk_pie_item;
typedef struct fck_nk_pie_item
{
	struct fck_nk_pie_item *prev;
	struct fck_nk_pie_item *next;
	struct fck_nk_pie_item *parent;

	struct fck_nk_pie_item *items;
	struct fck_nk_pie_item *items_last;

	// Everything but the name is actually private lol
	const char *name;
	int value;
} fck_nk_pie_item;

typedef struct fck_nk_control
{
	fckc_u32 minimise : 1;
	fckc_u32 close : 1;
	fckc_u32 menu : 1;
	fckc_u32 body : 1;
	fckc_u32 unused : 28;
} fck_nk_control;

typedef struct fck_nuklear_hamburger_api
{
	fck_nk_hamburger_item *(*push)(fck_nk nk, fck_nk_hamburger_item *item);
	int (*used)(fck_nk nk, fck_nk_hamburger_item *item);
} fck_nuklear_hamburger_api;

typedef struct fck_nuklear_pie_api
{
	// if we have root then we could streamline push_child and push to just become push
	fck_nk_pie_item *(*root)(fck_nk nk);
	void (*push)(fck_nk_pie_item *item, fck_nk_pie_item *child);
	void (*remove)(fck_nk nk, fck_nk_pie_item *item);

	int (*used)(fck_nk nk, fck_nk_pie_item *item);

	void (*apply_position)(fck_nk nk, float *x, float *y);
	int (*happened)(fck_nk_pie_item *item);
} fck_nuklear_pie_api;

struct fck_input_event;
typedef struct fck_nuklear_input_api
{
	void (*begin)(fck_nk nk);
	void (*events)(fck_nk nk, const struct fck_input_event *const events, fckc_size_t count);
	void (*end)(fck_nk nk);
} fck_nuklear_input_api;

// The API dependencies HAVE to disappear!
struct kll_allocator;
struct fck_window;
struct sht_driver;
struct sht_command_buffer;
struct sht_image_view;

// TODO: Debug loggin on interaction setting!
typedef struct fck_nuklear_elements_api
{
	fckc_f32 (*f32)(fck_nk nk, const char *name, fckc_f32 min, fckc_f32 val, fckc_f32 max, fckc_f32 step);
	fckc_i32 (*i32)(fck_nk nk, const char *name, fckc_i32 min, fckc_i32 val, fckc_i32 max, fckc_i32 step);

	int (*dropdown)(fck_nk nk, int selected, const char *const *items, int count);
	int (*button)(fck_nk nk, const char *title);

	// Image drawing works like this, let's clean it all up!
	int (*button_image)(fck_nk nk, struct sht_image_view *image);

	void (*label)(fck_nk nk, const char *fmt, ...);
} fck_nuklear_elements_api;

typedef struct fck_nuklear_panel_api
{
	int (*begin_label)(fck_nk nk, const char *name, float width);
	int (*begin_icon)(fck_nk nk, const char *name, struct sht_image_view *image_view, const fck_nk_rect *region, float width);

	void (*end)(fck_nk nk);

	// Maybe fmt?
	int (*push)(fck_nk nk, const char *fmt, ...);
	void (*pop)(fck_nk nk);
} fck_nuklear_panel_api;

// TODO: fck_nk should have an arena so we can create hamburger and pie items through it
typedef struct fck_nuklear_api
{
	// TODO: HMMM, window from driver is also an option
	fck_nk (*create)(struct kll_allocator *allocator, struct fck_window *window, struct sht_driver *driver);
	fck_nuklear_hamburger_api *hamburger;
	fck_nuklear_input_api *input;
	fck_nuklear_pie_api *pie;
	fck_nuklear_panel_api *panel;
	fck_nuklear_elements_api *elements;

	int (*begin)(fck_nk nk);
	void (*end)(fck_nk nk);

	void (*set_theme)(fck_nk nk, fck_nuklear_theme theme);
	fck_nuklear_theme (*get_theme)(fck_nk nk);
	// TODO: Fix this up, maybe provide our own style colour?
	fck_nk_colour (*get_style_colour)(fck_nk_style_colors style);

	int (*control_point)(fck_nk nk, const void *pointer, float *x, float *y, float size, fck_nk_colour on, fck_nk_colour off);
	int (*translation)(fck_nk nk, const void *pointer, float *x, float *y, float w, float h);

	void (*set_selection)(fck_nk nk, const void *pointer);
	int (*select)(fck_nk nk, const void *pointer, float x, float y, float w, float h, fck_nk_colour on);

	// TODO: Maybe remove
	int (*to_screen)(fck_nk nk, float *x, float *y);

	fck_nk_control (*control)(fck_nk nk);

	// Hm.. Not sure if hugging the driver and then having a pointer to command buffer is ok
	void (*present)(fck_nk nk, const struct sht_command_buffer *buffer, fckc_u32 frame_index);
} fck_nuklear_api;

#endif // !FCK_NUKLEAR_H_INCLUDED
