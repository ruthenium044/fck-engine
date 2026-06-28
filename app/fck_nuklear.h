#ifndef FCK_NUKLEAR_H_INCLUDED
#define FCK_NUKLEAR_H_INCLUDED

#include <fckc_inttypes.h>

// Even if I move away from nuklear, I will keep close to its API
#define fck_nuklear_api_name "fck_nuklear"

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
	struct fck_nk_pie_item *next;
	struct fck_nk_pie_item *parent;

	struct fck_nk_pie_item *child_items;
	struct fck_nk_pie_item *child_items_last;

	const char *name;
	int value;
} fck_nk_pie_item;

typedef struct fck_nk_pie
{
	float x;
	float y;
	fck_nk_pie_item *hovered;
	fck_nk_pie_item *items;
	fck_nk_pie_item *items_last;
	fckc_u32 active;
} fck_nk_pie;

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
} fck_nuklear_hamburger_api;

typedef struct fck_nuklear_pie_api
{
	fck_nk_pie_item *(*push)(fck_nk_pie *pie, fck_nk_pie_item *item);
	void (*add_child)(fck_nk_pie_item *item, fck_nk_pie_item *child);
	const fck_nk_pie_item *(*execute)(fck_nk nk, fck_nk_pie *pie, float radius);
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
struct fck_shader_api;
struct fck_input;
struct sht_command_buffer;

// TODO: fck_nk should have an arena so we can create hamburger and pie items through it
typedef struct fck_nuklear_api
{
	// TODO: HMMM, window from driver is also an option
	fck_nk (*create)(struct kll_allocator *allocator, struct fck_window *window, struct sht_driver *driver, struct fck_shader_api *shader);
	fck_nuklear_hamburger_api *hamburger;
	fck_nuklear_input_api *input;
	fck_nuklear_pie_api *pie;

	int (*begin)(fck_nk nk);
	void (*end)(fck_nk nk);

	void (*theme)(fck_nk nk, fck_nuklear_theme theme);

	fck_nk_control (*control)(fck_nk nk);

	// Hm.. Not sure if hugging the driver and then having a pointer to command buffer is ok
	void (*present)(fck_nk nk, const struct sht_command_buffer *buffer, fckc_u32 frame_index);
} fck_nuklear_api;

extern fck_nuklear_api *nk;

#endif // !FCK_NUKLEAR_H_INCLUDED
