#ifndef FCK_ECS_SNAPSHOT_INCLUDED
#define FCK_ECS_SNAPSHOT_INCLUDED

#include "ecs/fck_serialiser.h"

// TODO: Make the following typed, nice and dandy in ECS
// Baselines
struct fck_ecs_snapshot
{
	fck_serialiser serialiser;
	uint32_t seq;
};

// TODO: Make snapshot and delta function interface

void fck_ecs_snapshot_alloc(fck_ecs_snapshot *snapshot);
void fck_ecs_snapshot_free(fck_ecs_snapshot *snapshot);

#endif // !FCK_ECS_SNAPSHOT_INCLUDED