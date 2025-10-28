#ifndef FCK_RENDER_H_INCLUDED
#define FCK_RENDER_H_INCLUDED

#include <fckc_inttypes.h>

#define FCK_RENDER_API "fck-render"

struct fck_renderer;
struct fck_window;

typedef struct fck_vertex_2d
{
	fckc_f32 position[2];
	fckc_f32 col[4];
	fckc_f32 uv[2];
} fck_vertex_2d;

typedef struct fck_index
{
	fckc_u32 value;
} fck_index;

typedef struct fck_texture
{
	void *handle;
} fck_texture;

typedef enum fck_texture_access
{
	FCK_TEXTURE_ACCESS_STATIC,
	FCK_TEXTURE_ACCESS_STREAMING,
	FCK_TEXTURE_ACCESS_TARGET
} fck_texture_access;

typedef enum fck_texture_blend_mode
{
	FCK_TEXTURE_BLEND_MODE_NONE = 0x00000000u,
	FCK_TEXTURE_BLEND_MODE_BLEND = 0x00000001u,
	FCK_TEXTURE_BLEND_MODE_BLEND_PREMULTIPLIED = 0x00000010u,
	FCK_TEXTURE_BLEND_MODE_ADD = 0x00000002u,
	FCK_TEXTURE_BLEND_MODE_ADD_PREMULTIPLIED = 0x00000020u,
	FCK_TEXTURE_BLEND_MODE_MOD = 0x00000004u,
	FCK_TEXTURE_BLEND_MODE_MUL = 0x00000008u,
	FCK_TEXTURE_BLEND_MODE_INVALID = 0x7FFFFFFFu,
} fck_texture_blend_mode;

typedef struct fck_render_o
{
	void *handle;
} fck_render_o;

typedef struct fck_texture_api
{
	fck_texture (*create)(fck_render_o renderer, fck_alias(fck_texture_access, fckc_u32), fck_alias(fck_texture_blend_mode, fckc_u32),
	                      fckc_u32 width, fckc_u32 height);
	int (*upload)(fck_texture texture, void const *pixel, fckc_size_t pitch);
	int (*is_valid)(fck_texture texture);
	void (*destroy)(fck_texture texture);
} fck_texture_api;

typedef struct fck_render_vt
{
	fck_texture_api* texture;
	int (*raw)(fck_render_o, fck_texture, fck_vertex_2d *vertices, fckc_u32 vertex_count, fck_index *indices, fckc_u32 index_count);
} fck_render_vt;

typedef struct fck_renderer
{
	fck_render_vt *vt;
	fck_render_o obj; // Pointer or not to pointer?
} fck_renderer;

typedef struct fck_render_api
{
	const char* name;
	fck_renderer (*create)(struct fck_window *window);
	void (*destroy)(fck_renderer renderer);
} fck_render_api;

#endif // !FCK_RENDER_H_INCLUDED
