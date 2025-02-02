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

#endif // !FCK_UI_IMPLEMENTED
