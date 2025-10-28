#ifndef FCK_TUD_H_INCLUDED
#define FCK_TUD_H_INCLUDED

#include <fckc_inttypes.h>

struct fck_renderer;
struct fck_texture;

typedef struct fck_rect_src
{
	fckc_f32 min_x;
	fckc_f32 min_y;
	fckc_f32 max_x;
	fckc_f32 max_y;
} fck_rect_src;

typedef struct fck_rect_dst
{
	fckc_f32 cx;
	fckc_f32 cy;
	fckc_f32 w;
	fckc_f32 h;
} fck_rect_dst;

typedef struct fck_canvas_api
{
	// This is a bit tied to the window... Man, idk
	int (*sprite)(struct fck_renderer *, struct fck_texture const *, fck_rect_src const *, fck_rect_dst const *);
	int (*rect)(struct fck_renderer *, fck_rect_dst const *);

	// TODO: rect, circle, and so on ...
} fck_canvas_api;

#endif // FCK_TUD_H_INCLUDED