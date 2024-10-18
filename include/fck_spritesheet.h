#include <fck_file.h>

#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_stdinc.h>

#ifndef FCK_SPRITESHEET_INCLUDED
#define FCK_SPRITESHEET_INCLUDED

struct fck_point_list
{
	SDL_Point *points;
	size_t count;
	size_t capacity;
};

struct fck_rect_list
{
	SDL_FRect *data;
	size_t count;
	size_t capacity;
};

struct fck_rect_list_view
{
	size_t begin;
	size_t count;
};

struct fck_spritesheet
{
	SDL_Texture *texture;
	fck_rect_list rect_list;
};

void fck_spritesheet_free(fck_spritesheet *sprites);
bool fck_spritesheet_load(SDL_Renderer *renderer, const char *file_name, fck_spritesheet *out_sprites, bool force_rebuild = false);

void fck_rect_list_view_create(fck_rect_list *list, size_t at, size_t count, fck_rect_list_view *view);
SDL_FRect const *fck_rect_list_view_get(fck_rect_list *rect_source, fck_rect_list_view const *view, size_t at);
#endif // !FCK_SPRITESHEET_INCLUDED