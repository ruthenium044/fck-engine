#ifndef FCK_SDL_FRECT_EXTENSIONS_INCLUDED
#define FCK_SDL_FRECT_EXTENSIONS_INCLUDED

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_rect.h>

enum SDL_UICutMode
{
	SDL_UI_CUT_MODE_LEFT,
	SDL_UI_CUT_MODE_RIGHT,
	SDL_UI_CUT_MODE_TOP,
	SDL_UI_CUT_MODE_BOTTOM,
};

SDL_FRect SDL_FRectCreate(float x, float y, float w, float h);

SDL_FRect SDL_FRectCutRight(SDL_FRect *target, float cut);

SDL_FRect SDL_FRectCutBottom(SDL_FRect *target, float cut);

SDL_FRect SDL_FRectCutLeft(SDL_FRect *target, float cut);

SDL_FRect SDL_FRectCutTop(SDL_FRect *target, float cut);

SDL_FRect SDL_FRectCut(SDL_FRect *target, SDL_UICutMode cutMode, float cut);

#endif // !FCK_SDL_FRECT_EXTENSIONS_INCLUDED