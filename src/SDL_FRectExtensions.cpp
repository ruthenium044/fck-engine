#include "SDL_FRectExtensions.h"

SDL_FRect SDL_FRectCreate(float x, float y, float w, float h)
{
	SDL_FRect rect;
	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;
	return rect;
}

SDL_FRect SDL_FRectCutRight(SDL_FRect *target, float cut)
{
	target->w = SDL_max(target->w - cut, 0);
	return SDL_FRectCreate(target->x + target->w, target->y, cut, target->h);
}

SDL_FRect SDL_FRectCutBottom(SDL_FRect *target, float cut)
{
	target->h = SDL_max(target->h - cut, 0);
	return SDL_FRectCreate(target->x, target->y + target->h, target->w, cut);
}

SDL_FRect SDL_FRectCutLeft(SDL_FRect *target, float cut)
{
	const float width = target->w;
	target->w = SDL_max(target->w - cut, 0);
	float x = target->x;
	target->x = target->x + (width - target->w);
	return SDL_FRectCreate(x, target->y, cut, target->h);
}

SDL_FRect SDL_FRectCutTop(SDL_FRect *target, float cut)
{
	const float height = target->h;
	target->h = SDL_max(target->h - cut, 0);
	float y = target->y;
	target->y = target->y + (height - target->h);
	return SDL_FRectCreate(target->x, y, target->w, cut);
}

SDL_FRect SDL_FRectCut(SDL_FRect *target, SDL_UICutMode cutMode, float cut)
{
	switch (cutMode)
	{
	case SDL_UI_CUT_MODE_LEFT:
		return SDL_FRectCutLeft(target, cut);
	case SDL_UI_CUT_MODE_RIGHT:
		return SDL_FRectCutRight(target, cut);
	case SDL_UI_CUT_MODE_TOP:
		return SDL_FRectCutTop(target, cut);
	case SDL_UI_CUT_MODE_BOTTOM:
		return SDL_FRectCutBottom(target, cut);
	}
	SDL_assert(false && "Should never end up here - Invalid Rect cut mode");
	return *target;
}