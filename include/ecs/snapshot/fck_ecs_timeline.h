#ifndef FCK_ECS_TIMELINE_INCLUDED
#define FCK_ECS_TIMELINE_INCLUDED

#include "ecs/fck_dense_list.h"

// timeline
struct fck_ecs_tl_protocol
{
	// Out: (ackd, send)
	uint32_t ackd;
	uint32_t send;
};

struct fck_ecs_timeline
{
	// Centralised snapshots for multiple users to reference
	struct fck_ecs_snapshot *snapshots;
	// Multiple deltas - Technically not useful... Mostly useful if
	// multiple concurrent users exist at the same time
	struct fck_ecs_delta *deltas;

	size_t snapshot_capacity;
	size_t snapshot_head;

	size_t delta_capacity;
	size_t delta_head;

	fck_ecs_tl_protocol protocol;
};

struct fck_ecs_timeline_single
{
	fck_ecs_timeline timeline;
	fck_ecs_tl_protocol protocol;
};

struct fck_ecs_timeline_multi
{
	fck_ecs_timeline timeline;
	fck_ecs_tl_protocol* protocol;
	uint32_t count;
	uint32_t capacity;
};


void fck_ecs_timeline_alloc(fck_ecs_timeline *timeline, size_t capacity, size_t delta_capacity);
void fck_ecs_timeline_free(fck_ecs_timeline *timeline);

// struct fck_ecs_snapshot *fck_ecs_timeline_capture(fck_ecs_timeline *timeline, struct fck_ecs *ecs);
// struct fck_ecs_snapshot *fck_ecs_timeline_get(fck_ecs_timeline *timeline, size_t index_from_last_back_in_time);

void fck_ecs_timeline_delta_capture(fck_ecs_timeline *timeline, struct fck_ecs *ecs, struct fck_serialiser *external_serialiser);
void fck_ecs_timeline_delta_apply(fck_ecs_timeline *timeline, struct fck_ecs *ecs, struct fck_serialiser *external_serialiser);
void fck_ecs_timeline_delta_ack(fck_ecs_timeline *timeline, uint32_t snapshot_index);

#endif // !FCK_ECS_TIMELINE_INCLUDED