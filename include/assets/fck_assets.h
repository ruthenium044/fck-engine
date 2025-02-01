#ifndef FCK_ASSETS_INCLUDED
#define FCK_ASSETS_INCLUDED

#include <SDL3/SDL_stdinc.h>
#include "gen/gen_assets.h"

#ifdef GEN_DEFINED_PNG
void fck_assets_load_single(struct SDL_Renderer *renderer, gen_png png);
void fck_assets_load_multi(struct SDL_Renderer *renderer, gen_png const *png, size_t count);
struct SDL_Texture* fck_assets_get(gen_png png);
#endif // GEN_DEFINED_png

#endif // FCK_ASSETS_INCLUDED
