#ifndef FCK_INSTANCE_H_IMPLEMENTED
#define FCK_INSTANCE_H_IMPLEMENTED

#include <fckc_inttypes.h>

// TODO: Need to somehow get rid of SDL_Event... SOMEHOOOOW
union SDL_Event;

typedef enum fck_instance_result
{
	FCK_INSTANCE_CONTINUE,
	FCK_INSTANCE_SUCCESS,
	FCK_INSTANCE_FAILURE

} fck_instance_result;

struct fck_instance;

struct fck_instance *fck_instance_alloc(int argc, char *argv[]);
void fck_instance_free(struct fck_instance *instance);
fck_instance_result fck_instance_event(struct fck_instance *instance, union SDL_Event const *event);
fck_instance_result fck_instance_tick(struct fck_instance *instance);

#endif // !FCK_INSTANCE_H_IMPLEMENTED
