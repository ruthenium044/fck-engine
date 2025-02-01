#include "assets/fck_assets.h"

#include <SDL3/SDL_render.h>
#include <SDL3_image/SDL_image.h>

// TODO: Size of FILE_PATHS is an implementation detail - Get rid of it somehow
static void *resources[SDL_arraysize(GEN_FILE_PATHS)];

#ifdef GEN_DEFINED_PNG
static SDL_Texture *null_texture;

void fck_assets_load_single(SDL_Renderer *renderer, gen_png png)
{
	SDL_assert(renderer != nullptr);

	const char *path = gen_get_path(png);
	SDL_Texture *texture = IMG_LoadTexture(renderer, path);
	SDL_assert(texture != nullptr && "Could not load file from baked file path. Regenerate? Rebuild?");
	resources[(size_t)png] = texture;
}

void fck_assets_load_multi(SDL_Renderer *renderer, gen_png const *png, size_t count)
{
	for (size_t index = 0; index < count; index++)
	{
		fck_assets_load_single(renderer, png[index]);
	}
}

SDL_Texture *fck_assets_get(gen_png png)
{
	return (SDL_Texture *)resources[(size_t)png];
}
#endif // GEN_DEFINED_png
