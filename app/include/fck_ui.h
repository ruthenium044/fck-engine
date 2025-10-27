
#ifndef FCK_UI_H_IMPLEMENTED
#define FCK_UI_H_IMPLEMENTED

struct nk_context;
struct SDL_Window;
struct SDL_Renderer;
union SDL_Event;
struct fck_canvas;

enum fck_theme
{
	THEME_BLACK,
	THEME_WHITE,
	THEME_RED,
	THEME_BLUE,
	THEME_DARK,
	THEME_DRACULA,
	THEME_CATPPUCCIN_LATTE,
	THEME_CATPPUCCIN_FRAPPE,
	THEME_CATPPUCCIN_MACCHIATO,
	THEME_CATPPUCCIN_MOCHA
};

// Consistency!
typedef struct nk_context fck_ui_ctx;

typedef struct fck_ui fck_ui;

fck_ui *fck_ui_alloc(struct fck_canvas *canvas);
void fck_ui_free(fck_ui *ui);

void fck_ui_render(fck_ui *ui, struct fck_canvas *canvas);
void fck_ui_enqueue_event(fck_ui *ui, union SDL_Event const *event);

fck_ui_ctx *fck_ui_context(fck_ui *ui);

struct nk_color *fck_ui_set_style(struct nk_context *ctx, enum fck_theme theme);

// Odd declaration, but the source file requires it to be like this!
// This way the user only needs to #include<fck_ui.h>
// This convenience include is justified since fck_ui is HEAVILY tied to nuklear
// TODO: nuklear.h -> nuklear.inl?
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_KEYSTATE_BASED_INPUT
#define NK_UINT_DRAW_INDEX
#include "nuklear.h"

#endif // !FCK_UI_H_IMPLEMENTED