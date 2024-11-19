#ifndef FCK_INSTANCE_INCLUDED
#define FCK_INSTANCE_INCLUDED

#include "ecs/fck_ecs.h"
#include <SDL3/SDL_stdinc.h>

typedef void (*fck_instance_setup_function)(fck_ecs *);

struct fck_instance_info
{
	char const *title;

	char const *ip;
	uint16_t source_port;
	uint16_t destination_port;
};

struct fck_instance
{
	fck_ecs ecs;
	struct fck_engine *engine;
	fck_instance_info *info;
};

void fck_instance_alloc(fck_instance *instance, fck_instance_info const *info, fck_instance_setup_function instance_setup);
void fck_instance_free(fck_instance *instance);

#endif // !FCK_INSTANCE_INCLUDED