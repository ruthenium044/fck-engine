#include "fck_img.h"
#include <fck_apis.h>
#include <fckc_apidef.h>

#include <kll.h>
#include <kll_malloc.h>

#include <fck_os.h>

#include <SDL3_image/SDL_image.h>

fck_img fck_img_load(struct kll_allocator *allocator, const char *path)
{
	SDL_Surface *surface = IMG_Load(path);
	fck_img img;
	img.allocator = allocator;
	img.pitch = surface->pitch;
	img.width = surface->w;
	img.height = surface->h;
	fckc_size_t channel_count = img.pitch / img.width;
	fckc_size_t size = (fckc_size_t)(img.width * img.height * channel_count);
	img.pixels = kll_malloc(allocator, size);
	os->mem->cpy(img.pixels, surface->pixels, size);
	SDL_DestroySurface(surface);
	return img;
}

fck_img fck_img_copy(struct kll_allocator *allocator, fck_img other)
{
	fck_img img;
	img.allocator = allocator;
	img.pitch = other.pitch;
	img.width = other.width;
	img.height = other.height;
	fckc_size_t channel_count = img.pitch / img.width;
	fckc_size_t size = (fckc_size_t)(img.width * img.height * channel_count);
	img.pixels = kll_malloc(allocator, size);
	os->mem->cpy(img.pixels, other.pixels, size);
	return img;
}

int fck_img_is_valid(fck_img img)
{
	return img.pixels != NULL;
}

void fck_img_free(fck_img img)
{
	kll_free(img.allocator, img.pixels);
}

static fck_img_api img_api = {
	.load = fck_img_load,
	.free = fck_img_free,
	.copy = fck_img_copy,
	.is_valid = fck_img_is_valid,
};

FCK_EXPORT_API void *fck_main(fck_api_registry *reg, void *params)
{
	reg->add("FCK_IMG", &img_api);
	return &img_api;
}