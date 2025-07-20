
#ifndef FCK_UI_H_IMPLEMENTED
#define FCK_UI_H_IMPLEMENTED

struct nk_sdl;
struct nk_context;
struct SDL_Window;
struct SDL_Renderer;
union SDL_Event;

typedef struct fck_ui
{
	struct nk_sdl* sdl;
	struct nk_context* ctx;
} fck_ui;


void fck_ui_handle_grab(struct fck_ui* ui, struct SDL_Window* window);

int fck_ui_handle_event(struct fck_ui* ui, union SDL_Event* evt);

void fck_ui_setup(struct fck_ui* ui, struct SDL_Renderer* renderer);

void fck_ui_render(struct fck_ui* ui, struct SDL_Renderer* renderer);

// Odd declaration, but the source file requires it to be like this!
// This way the user only needs to #include<fck_ui.h>
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#include "nuklear.h"

#endif // !FCK_UI_H_IMPLEMENTED