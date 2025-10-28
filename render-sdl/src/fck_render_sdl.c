#include <fck_render.h>

#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>

#include <fck_os.h>
#include <fck_apis.h>

int fck_texture_is_valid(fck_texture image)
{
	return image.handle != NULL;
}

int fck_texture_blend_mode_set(fck_texture image, fck_alias(fck_texture_blend_mode, fckc_u32) blend_mode)
{
	return (int)SDL_SetTextureBlendMode((SDL_Texture *)image.handle, blend_mode);
}

fck_texture fck_texture_create(fck_render_o renderer, fckc_u32 access, fckc_u32 blend_mode, fckc_u32 width, fckc_u32 height)
{
	SDL_Texture *texture = SDL_CreateTexture((SDL_Renderer *)renderer.handle, //
	                                         SDL_PIXELFORMAT_ARGB8888,        //
	                                         (SDL_TextureAccess)access,       //
	                                         (int)width,                      //
	                                         (int)height);
	fck_texture image = (fck_texture){.handle = (void *)texture};

	if (!fck_texture_is_valid(image))
	{
		return image;
	}
	fck_texture_blend_mode_set(image, blend_mode);
	return image;
}

int fck_texture_upload(fck_texture image, void const *pixels, fckc_size_t pitch)
{
	int result = SDL_UpdateTexture((SDL_Texture *)image.handle, NULL, pixels, pitch);
	return result;
}

void fck_texture_destroy(fck_texture image)
{
	SDL_DestroyTexture((SDL_Texture *)image.handle);
}

int fck_render_sdl_raw(fck_render_o obj, fck_texture texture, fck_vertex_2d *vertices, fckc_u32 vertex_count, fck_index *indices,
                       fckc_u32 index_count)
{
	int vs = sizeof(struct fck_vertex_2d);
	fckc_size_t vp = offsetof(struct fck_vertex_2d, position);
	fckc_size_t vt = offsetof(struct fck_vertex_2d, uv);
	fckc_size_t vc = offsetof(struct fck_vertex_2d, col);

	SDL_Renderer *renderer = (SDL_Renderer *)obj.handle;
	int result = SDL_RenderGeometryRaw(renderer, (SDL_Texture *)texture.handle,                             //
	                                   (const float *)((const fckc_u8 *)vertices + vp), vs,                 //
	                                   (const struct SDL_FColor *)((const fckc_u8 *)vertices + vc), vs,     //
	                                   (const float *)((const fckc_u8 *)vertices + vt), vs,                 //
	                                   vertex_count, (const void *)indices, index_count, sizeof(*indices)); //
	return result;
}

static fck_texture_api texture_api = {
	.create = fck_texture_create,
	.destroy = fck_texture_destroy,
	.is_valid = fck_texture_is_valid,
	.upload = fck_texture_upload,
};

static fck_render_vt render_sdl_vt = {
	.texture = &texture_api,
	.raw = fck_render_sdl_raw,
};

fck_renderer fck_render_sdl_create(struct fck_window *window)
{
	fck_renderer renderer;
	renderer.vt = &render_sdl_vt;
	renderer.obj.handle = SDL_CreateRenderer((SDL_Window *)window->handle, NULL);
}

void fck_render_sdl_destroy(fck_renderer renderer)
{
	SDL_DestroyRenderer((SDL_Renderer *)renderer.obj.handle);
}

static fck_render_api render_sdl_api = {
	.name = "SDL",
	.create = fck_render_sdl_create,
	.destroy = fck_render_sdl_destroy,
};

FCK_EXPORT_API fck_render_api *fck_main(fck_api_registry *apis, void *params)
{
	apis->add(FCK_RENDER_API, &render_sdl_api);
	return &render_sdl_api;
}