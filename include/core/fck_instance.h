#ifndef FCK_INSTANCE_INCLUDED
#define FCK_INSTANCE_INCLUDED

#include "ecs/fck_ecs.h"
#include <SDL3/SDL_stdinc.h>
#include "ecs/fck_queue.h"
#include <SDL3/SDL_events.h>

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
	uint32_t window_id;
	fck_ecs ecs;

	// I hope this shit will never move
	// Technically it should be View<Engine>(ecs);
	struct fck_engine *engine;

	fck_queue<uint16_t, SDL_Event> event_queue;
};

void fck_instance_alloc(fck_instance *instance, fck_instance_info const *info, fck_instance_setup_function instance_setup);
void fck_instance_free(fck_instance *instance);

#endif // !FCK_INSTANCE_INCLUDED