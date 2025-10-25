#ifndef FCK_INSTANCE_H_IMPLEMENTED
#define FCK_INSTANCE_H_IMPLEMENTED

#include <fckc_inttypes.h>

// TODO: Forward declartions HAVE and SHALL always be used as "struct X ..."
struct SDL_Window;
struct SDL_Renderer;
struct fck_ui;
struct fck_ui_window_manager;
struct fck_assembly;
union SDL_Event;

typedef enum fck_instance_result
{
	FCK_INSTANCE_CONTINUE,
	FCK_INSTANCE_SUCCESS,
	FCK_INSTANCE_FAILURE

} fck_instance_result;

// This struct is good enough for now
typedef struct fck_instance
{
	struct fck_ui *ui;             // User
	struct SDL_Window *window;     // This one could stay public - Makes sense for multi-instance stuff
	struct SDL_Renderer *renderer; // User
	struct fck_ui_window_manager *window_manager;
	struct fck_assembly *assembly;
} fck_instance;

fck_instance *fck_instance_alloc(int argc, char* argv[]);
void fck_instance_free(fck_instance *instance);
fck_instance_result fck_instance_event(fck_instance *instance, union SDL_Event const *event);
fck_instance_result fck_instance_tick(fck_instance *instance);

#endif // !FCK_INSTANCE_H_IMPLEMENTED
