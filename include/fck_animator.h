#ifndef FCK_ANIMATOR_INCLUDED
#define FCK_ANIMATOR_INCLUDED

#include "fck_spritesheet.h"

enum fck_common_animations
{
	FCK_COMMON_ANIMATION_IDLE,
	FCK_COMMON_ANIMATION_CROUCH,
	FCK_COMMON_ANIMATION_JUMP_UP,
	FCK_COMMON_ANIMATION_JUMP_FORWARD,
	FCK_COMMON_ANIMATION_JUMP_BACKWARD,
	FCK_COMMON_ANIMATION_WALK_FORWARD,
	FCK_COMMON_ANIMATION_WALK_BACKWARD,
	FCK_COMMON_ANIMATION_PUNCH_A,
	FCK_COMMON_ANIMATION_PUNCH_B,
	FCK_COMMON_ANIMATION_KICK_A,
	FCK_COMMON_ANIMATION_KICK_B,
	FCK_COMMON_ANIMATION_COUNT,
	FCK_COMMON_ANIMATION_INVALID = FCK_COMMON_ANIMATION_COUNT
};

enum fck_animation_type
{
	FCK_ANIMATION_TYPE_LOOP,
	FCK_ANIMATION_TYPE_ONCE,
};

struct fck_animation
{
	fck_animation_type animation_type;
	fck_rect_list_view rect_view;
	uint64_t frame_time_ms;
	SDL_FPoint offset;
};

struct fck_static_sprite
{
	size_t sprite_index;
	SDL_FPoint offset;
};

struct fck_animator
{
	// fck_spritesheet *spritesheet;
	fck_animation animations[FCK_COMMON_ANIMATION_COUNT];

	fck_common_animations active_oneshot;
	fck_common_animations active_animation;

	uint64_t time_accumulator_ms;
	size_t current_frame;
};

void fck_animator_alloc(fck_animator *animator, fck_spritesheet *spritsheet);

void fck_animator_free(fck_animator *animator);

void fck_animator_insert(fck_animator *animator, fck_spritesheet *spritesheet, fck_common_animations anim,
                         fck_animation_type animation_type, size_t start, size_t count, uint64_t frame_time_ms, float offset_x,
                         float offset_y);

void fck_animator_set(fck_animator *animator, fck_common_animations anim);

void fck_animator_play(fck_animator *animator, fck_common_animations anim);

bool fck_animator_is_playing(fck_animator *animator);

bool fck_animator_update(fck_animator *animator, uint64_t delta_ms);

SDL_FRect const *fck_animator_get_rect(fck_animator *animator, fck_spritesheet *spritesheet);

void fck_animator_apply(fck_animator *animator, SDL_FRect *rect, float scale);

#endif // FCK_ANIMATOR_INCLUDED