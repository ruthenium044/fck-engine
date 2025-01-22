#ifndef FCK_ECS_DELTA_INCLUDED
#define FCK_ECS_DELTA_INCLUDED

#include "ecs/fck_serialiser.h"

struct fck_ecs_delta
{
	fck_serialiser serialiser;
};
void fck_ecs_delta_alloc(fck_ecs_delta *delta);
void fck_ecs_delta_free(fck_ecs_delta *delta);

#endif // !FCK_ECS_SNAPSHOT_DELTA_INCLUDED