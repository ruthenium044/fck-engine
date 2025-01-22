#ifndef FCK_ECS_TIMELINE_INCLUDED
#define FCK_ECS_TIMELINE_INCLUDED

#include "ecs/fck_dense_list.h"

struct fck_ecs_timeline
{
	struct fck_ecs_snapshot *snapshots;
	struct fck_ecs_delta *deltas;

	size_t snapshot_capacity;
	size_t snapshot_head;

	size_t delta_capacity;
	size_t delta_head;

	uint32_t baseline_seq_sent;
	uint32_t baseline_seq_recv;
	uint32_t baseline_seq_ackd;
};

void fck_ecs_timeline_alloc(fck_ecs_timeline *timeline, size_t capacity, size_t delta_capacity);
void fck_ecs_timeline_free(fck_ecs_timeline *timeline);

struct fck_ecs_snapshot *fck_ecs_timeline_capture(fck_ecs_timeline *timeline, fck_ecs *ecs);
// struct fck_ecs_snapshot *fck_ecs_timeline_get(fck_ecs_timeline *timeline, size_t index_from_last_back_in_time);

void fck_ecs_timeline_delta_capture(fck_ecs_timeline *timeline, fck_ecs *ecs, fck_serialiser *external_serialiser);
void fck_ecs_timeline_delta_apply(fck_ecs_timeline *timeline, fck_ecs *ecs, fck_serialiser *external_serialiser);

#endif // !FCK_ECS_TIMELINE_INCLUDED