#ifndef FCK_ECS_TIMELINE_INCLUDED
#define FCK_ECS_TIMELINE_INCLUDED

#include <SDL3/SDL_stdinc.h>

// timeline
struct fck_ecs_timeline_protocol
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
};

struct fck_ecs_sc_timeline
{
	fck_ecs_timeline timeline;
	fck_ecs_timeline_protocol protocol;
};

struct fck_ecs_mc_timeline
{
	fck_ecs_timeline timeline;
	fck_ecs_timeline_protocol *protocols;
	uint32_t capacity;
};

// TODO: Make size_t -> uint32_t
// Could become private interface ...
//
// Single Producer - Single Consumer

void fck_ecs_sc_timeline_alloc(fck_ecs_sc_timeline *single, size_t capacity, size_t delta_capacity);
void fck_ecs_sc_timeline_free(fck_ecs_sc_timeline *single);
void fck_ecs_sc_timeline_delta_capture(fck_ecs_sc_timeline *single, struct fck_ecs *ecs, struct fck_serialiser *serialiser);
void fck_ecs_sc_timeline_delta_apply(fck_ecs_sc_timeline *single, struct fck_ecs *ecs, struct fck_serialiser *serialiser);
void fck_ecs_sc_timeline_delta_ack(fck_ecs_sc_timeline *single, uint32_t snapshot_seq);

// Single Producer - Multiple Consumer

void fck_ecs_mc_timeline_alloc(fck_ecs_mc_timeline *multi, size_t capacity, size_t delta_capacity, size_t protocol_capacity);
void fck_ecs_mc_timeline_free(fck_ecs_mc_timeline *multi);
void fck_ecs_mc_timeline_delta_capture(fck_ecs_mc_timeline *multi, uint32_t index, struct fck_ecs *ecs, struct fck_serialiser *serialiser);
void fck_ecs_mc_timeline_delta_apply(fck_ecs_mc_timeline *multi, uint32_t index, struct fck_ecs *ecs, struct fck_serialiser *serialiser);
void fck_ecs_mc_timeline_delta_ack(fck_ecs_mc_timeline *multi, uint32_t index, uint32_t snapshot_seq);

#endif // !FCK_ECS_TIMELINE_INCLUDED