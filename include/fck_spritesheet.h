#include <fck_file.h>

#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_stdinc.h>

#ifndef FCK_SPRITESHEET_INCLUDED
#define FCK_SPRITESHEET_INCLUDED

struct fck_rect_list
{
	SDL_FRect *rects;
	size_t count;
	size_t capacity;
};

struct fck_spritesheet
{
	SDL_Texture *texture;
	fck_rect_list rect_list;
};

void fck_spritesheet_free(fck_spritesheet *sprites);
bool fck_spritesheet_load(SDL_Renderer *renderer, const char *file_name, fck_spritesheet *out_sprites);

#endif // !FCK_SPRITESHEET_INCLUDED