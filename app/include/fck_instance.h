#ifndef FCK_INSTANCE_H_IMPLEMENTED
#define FCK_INSTANCE_H_IMPLEMENTED

#include <fckc_inttypes.h>

// TODO: Forward declartions HAVE and SHALL always be used as "struct X ..."
typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct fck_ui fck_ui;
typedef fckc_u64 SDL_WindowFlags;
typedef union SDL_Event SDL_Event;
typedef struct fck_ui_window_manager fck_ui_window_manager;
typedef struct fck_apis fck_apis;
typedef struct fck_assembly fck_assembly;

typedef enum fck_instance_result
{
	FCK_INSTANCE_CONTINUE,
	FCK_INSTANCE_SUCCESS,
	FCK_INSTANCE_FAILURE

} fck_instance_result;

// This struct is good enough for now
typedef struct fck_instance
{
	fck_ui *ui;             // User
	SDL_Window *window;     // This one could stay public - Makes sense for multi-instance stuff
	SDL_Renderer *renderer; // User
	fck_ui_window_manager *window_manager;
	fck_assembly *assembly;
} fck_instance;

fck_instance *fck_instance_alloc(const char *title, int with, int height, SDL_WindowFlags window_flags, const char *renderer_name);
void fck_instance_free(fck_instance *instance);
fck_instance_result fck_instance_event(fck_instance *instance, SDL_Event const *event);
fck_instance_result fck_instance_tick(fck_instance *instance);

#endif // !FCK_INSTANCE_H_IMPLEMENTED
