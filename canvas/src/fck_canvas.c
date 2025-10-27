#include <fck_canvas.h>

#include <SDL3/SDL_render.h>
#include <fck_apis.h>
#include <fckc_apidef.h>

int fck_image_is_valid(fck_image image)
{
	return image.handle != NULL;
}

int fck_image_blend_mode_set(fck_image image, fck_alias(fck_image_blend_mode, fckc_u32) blend_mode)
{
	return (int)SDL_SetTextureBlendMode((SDL_Texture *)image.handle, blend_mode);
}

fck_image fck_image_create(fck_canvas canvas, fck_alias(fck_image_access, fckc_u32) access,
                           fck_alias(fck_image_blend_mode, fckc_u32) blend_mode, fckc_u32 width, fckc_u32 height)
{
	SDL_Texture *texture = SDL_CreateTexture((SDL_Renderer *)canvas.handle, SDL_PIXELFORMAT_ARGB8888, access, width, height);
	fck_image image = (fck_image){.handle = (void *)texture};

	if (!fck_image_is_valid(image))
	{
		return image;
	}
	fck_image_blend_mode_set(image, blend_mode);
	// Returns success or failure. No clue what to do on failure lol
	return image;
}

int fck_image_upload(fck_image image, void const *pixels, fckc_size_t pitch)
{
	int result = SDL_UpdateTexture((SDL_Texture *)image.handle, NULL, pixels, pitch);
	return result;
}

void fck_image_destroy(fck_image image)
{
	SDL_DestroyTexture((SDL_Texture *)image.handle);
}

fck_canvas fck_canvas_create(void *window)
{
	SDL_Renderer *renderer = SDL_CreateRenderer((SDL_Window *)window, NULL);
	fck_canvas canvas = (fck_canvas){.handle = (void *)renderer};
	return canvas;
}

int fck_canvas_is_valid(fck_canvas canvas)
{
	return canvas.handle != NULL;
}

void fck_canvas_destroy(fck_canvas canvas)
{
	SDL_DestroyRenderer((SDL_Renderer *)canvas.handle);
}

int fck_canvas_clear(fck_canvas canvas)
{
	return (int)SDL_RenderClear((SDL_Renderer *)canvas.handle);
}

int fck_canvas_set_color(fck_canvas canvas, fckc_u8 r, fckc_u8 g, fckc_u8 b, fckc_u8 a)
{
	return (int)SDL_SetRenderDrawColor((SDL_Renderer *)canvas.handle, r, g, b, a);
}

int fck_canvas_geometry_raw(fck_canvas canvas, fck_image image, const fckc_f32 *xy, fckc_u32 xy_stride, const fck_colour *color,
                            fckc_u32 color_stride, const fckc_f32 *uv, fckc_u32 uv_stride, fckc_u32 num_vertices, const void *indices,
                            fckc_u32 num_indices, fckc_u32 size_indices)
{

	SDL_Renderer *renderer = (SDL_Renderer *)canvas.handle;
	int result = SDL_RenderGeometryRaw(renderer,                                        //
	                                   (SDL_Texture *)image.handle, xy, (int)xy_stride, //
	                                   (SDL_FColor *)color, (int)color_stride,          //
	                                   uv, (int)uv_stride,                              //
	                                   (int)num_vertices,                               //
	                                   indices, (int)num_indices, (int)size_indices);
	return result;
}
int fck_canvas_geometry(fck_canvas canvas, fck_image image, fck_canvas_vertex *vertices, fckc_u32 num_vertices, const void *indices,
                        fckc_u32 num_indices, fckc_u32 size_indices)
{
	int vs = sizeof(struct fck_canvas_vertex);
	fckc_size_t vp = offsetof(struct fck_canvas_vertex, position);
	fckc_size_t vt = offsetof(struct fck_canvas_vertex, uv);
	fckc_size_t vc = offsetof(struct fck_canvas_vertex, col);

	SDL_Renderer *renderer = (SDL_Renderer *)canvas.handle;
	int result = fck_canvas_geometry_raw(
		canvas, image, (const float *)((const fckc_u8 *)vertices + vp), vs, (const fck_colour *)((const fckc_u8 *)vertices + vc), vs,
		(const float *)((const fckc_u8 *)vertices + vt), vs, num_vertices, indices, num_indices, size_indices);
	return result;
}

int fck_canvas_present(fck_canvas canvas)
{
	return (int)SDL_RenderPresent((SDL_Renderer *)canvas.handle);
}

static fck_image_api image_api = {
	.create = fck_image_create,
	.destroy = fck_image_destroy,
	.is_valid = fck_image_is_valid,
	.upload = fck_image_upload,
};

static fck_canvas_api canvas_api = {
	.image = &image_api,
	.create = fck_canvas_create,
	.destroy = fck_canvas_destroy,
	.is_valid = fck_canvas_is_valid,
	.clear = fck_canvas_clear,
	.present = fck_canvas_present,
	.set_color = fck_canvas_set_color,
	.geometry = fck_canvas_geometry,
};

FCK_EXPORT_API fck_canvas_api *fck_main(fck_api_registry *apis, void *params)
{
	printf("%s loaded and initialised\n", __FILE_NAME__);
	apis->add("FCK_CANVAS", &canvas_api);
	return &canvas_api;
}