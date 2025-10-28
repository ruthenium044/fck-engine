#ifndef FCK_TUD_H_INCLUDED
#define FCK_TUD_H_INCLUDED

#include <fckc_inttypes.h>

// TODO: ...  Is that smart? I am unsure if I should share fck_color...
typedef struct fck_colour
{
	fckc_f32 r;
	fckc_f32 g;
	fckc_f32 b;
	fckc_f32 a;
} fck_colour;

typedef struct fck_canvas
{
	void *handle;
} fck_canvas;

typedef struct fck_image
{
	void *handle;
} fck_image;

typedef enum fck_image_access
{
	FCK_IMAGE_ACCESS_STATIC,
	FCK_IMAGE_ACCESS_STREAMING,
	FCK_IMAGE_ACCESS_TARGET
} fck_image_access;

typedef enum fck_image_blend_mode
{
	FCK_IMAGE_BLEND_MODE_NONE = 0x00000000u,
	FCK_IMAGE_BLEND_MODE_BLEND = 0x00000001u,
	FCK_IMAGE_BLEND_MODE_BLEND_PREMULTIPLIED = 0x00000010u,
	FCK_IMAGE_BLEND_MODE_ADD = 0x00000002u,
	FCK_IMAGE_BLEND_MODE_ADD_PREMULTIPLIED = 0x00000020u,
	FCK_IMAGE_BLEND_MODE_MOD = 0x00000004u,
	FCK_IMAGE_BLEND_MODE_MUL = 0x00000008u,
	FCK_IMAGE_BLEND_MODE_INVALID = 0x7FFFFFFFu,
} fck_image_blend_mode;

typedef struct fck_image_api
{
	fck_image (*create)(fck_canvas canvas, fck_alias(fck_image_access, fckc_u32) access, fck_alias(fck_image_blend_mode, fckc_u32),
	                    fckc_u32 width, fckc_u32 height);
	// size is pitch now... We should make it size and calcualte the pitch and see that everything aligns? Maybe?
	int (*upload)(fck_image image, void const *pixel, fckc_size_t size);
	int (*is_valid)(fck_image image);
	void (*destroy)(fck_image image);
} fck_image_api;

typedef struct fck_canvas_vertex
{
	float position[2];
	float col[4];
	float uv[2];
} fck_canvas_vertex;

typedef struct fck_canvas_api
{
	fck_image_api *image;

	// This is a bit tied to the window... Man, idk
	fck_canvas (*create)(void *window);
	void (*destroy)(fck_canvas);
	int (*is_valid)(fck_canvas);
	int (*clear)(fck_canvas);
	int (*set_color)(fck_canvas, fckc_u8 r, fckc_u8 g, fckc_u8 b, fckc_u8 a);

	// TODO: Fuck me, fix these parameters
	int (*geometry)(fck_canvas canvas, fck_image image, fck_canvas_vertex *vertices, fckc_u32 num_vertices, const void *indices,
	                fckc_u32 num_indices, fckc_u32 size_indices);
	// void (*get_color)(fck_canvas, fckc_u8* r, fckc_u8* g, fckc_u8* b, fckc_u8* a);
	int (*present)(fck_canvas);
	// void (*rendeer)(fck_canvas canvas, fck_image image);
} fck_canvas_api;

#endif // FCK_TUD_H_INCLUDED