#include "fck_animator.h"
#include <SDL3/SDL_assert.h>

void fck_animator_alloc(fck_animator *animator, fck_spritesheet *spritsheet)
{
	SDL_assert(spritsheet != nullptr);
	SDL_zerop(animator);

	// What a hard dependency
	animator->spritesheet = spritsheet;
}

void fck_animator_free(fck_animator *animator)
{
	SDL_assert(animator != nullptr);
	animator->spritesheet = nullptr;
}

void fck_animator_insert(fck_animator *animator, fck_common_animations anim, fck_animation_type animation_type, size_t start, size_t count,
                         uint64_t frame_time_ms, float offset_x, float offset_y)
{
	SDL_assert(animator != nullptr);
	SDL_assert(animator->spritesheet != nullptr && "Resource data for animator not set!");

	fck_animation *animation = &animator->animations[anim];
	SDL_assert(animation->rect_view.rect_list == nullptr && "Overwriting animation");

	fck_rect_list_view_create(&animator->spritesheet->rect_list, start, count, &animation->rect_view);
	animation->animation_type = animation_type;
	animation->frame_time_ms = frame_time_ms;
	animation->offset.x = offset_x;
	animation->offset.y = offset_y;
}

void fck_animator_set(fck_animator *animator, fck_common_animations anim)
{
	SDL_assert(animator != nullptr);

	fck_animation *next_animation = &animator->animations[anim];
	if (next_animation != animator->active_animation)
	{
		animator->active_animation = next_animation;
		animator->current_frame = 0;
		animator->time_accumulator_ms = 0;
	}
}

void fck_animator_play(fck_animator *animator, fck_common_animations anim)
{
	SDL_assert(animator != nullptr);

	if (animator->active_oneshot == nullptr)
	{
		fck_animation *next_animation = &animator->animations[anim];
		animator->active_oneshot = next_animation;
		animator->current_frame = 0;
		animator->time_accumulator_ms = 0;
	}
}

bool fck_animator_is_playing(fck_animator *animator)
{
	SDL_assert(animator != nullptr);

	return animator->active_oneshot != nullptr;
}

bool fck_animator_update(fck_animator *animator, uint64_t delta_ms)
{
	SDL_assert(animator != nullptr);

	if (animator->active_animation == nullptr)
	{
		return false;
	}

	if (animator->active_animation->rect_view.rect_list == nullptr)
	{
		return false;
	}

	animator->time_accumulator_ms += delta_ms;

	fck_animation *animation = animator->active_animation;

	bool is_oneshot_animation = animator->active_oneshot != nullptr;
	if (is_oneshot_animation)
	{
		animation = animator->active_oneshot;
	}

	while (animator->time_accumulator_ms > animation->frame_time_ms)
	{
		animator->time_accumulator_ms = animator->time_accumulator_ms - animation->frame_time_ms;
		animator->current_frame = animator->current_frame + 1;

		if (animator->current_frame >= animation->rect_view.count)
		{
			if (is_oneshot_animation)
			{
				animator->active_oneshot = nullptr;
				animator->current_frame = 0;
				return true;
			}

			switch (animation->animation_type)
			{
			case FCK_ANIMATION_TYPE_LOOP:
				animator->current_frame = 0;
				break;

			case FCK_ANIMATION_TYPE_ONCE:
				animator->current_frame = animation->rect_view.count - 1;
				break;
			}
		}
	}
	return true;
}

SDL_FRect const *fck_animator_get_rect(fck_animator *animator)
{
	SDL_assert(animator != nullptr);
	SDL_assert(animator->active_animation != nullptr);

	fck_animation *animation = animator->active_animation;
	if (animator->active_oneshot != nullptr)
	{
		animation = animator->active_oneshot;
	}
	fck_rect_list_view const *view = &animation->rect_view;
	return fck_rect_list_view_get(view, animator->current_frame);
}

void fck_animator_apply(fck_animator *animator, SDL_FRect *rect, float scale)
{
	SDL_assert(animator != nullptr);
	SDL_assert(animator->active_animation != nullptr);

	fck_animation *animation = animator->active_animation;
	if (animator->active_oneshot != nullptr)
	{
		animation = animator->active_oneshot;
	}
	rect->x = rect->x + (animation->offset.x * scale);
	rect->y = rect->y + (animation->offset.y * scale);
}