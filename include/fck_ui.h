#ifndef FCK_UI_IMPLEMENTED
#define FCK_UI_IMPLEMENTED

struct fck_ui
{
	struct nk_sdl* sdl;
	struct nk_context* ctx;
};

void fck_ui_handle_grab(struct fck_ecs* ecs);

int fck_ui_handle_event(struct fck_ecs* ecs, union SDL_Event* evt);

void fck_ui_setup(struct fck_ecs* ecs, struct fck_system_once_info*);

void fck_ui_render(struct fck_ecs* ecs, bool use_anti_aliasing);

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

#endif // !FCK_UI_IMPLEMENTED
