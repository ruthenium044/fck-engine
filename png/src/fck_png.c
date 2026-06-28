#include "fck_png.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <fckc_apidef.h>
#include <fck_apis.h>

static fck_png fck_png_api_load(const char *path)
{
	fck_png png;
	png.data = stbi_load(path, &png.width, &png.height, &png.channels, 0);
	if (png.data == NULL)
	{
		memset(&png, 0, sizeof(png));
	}
	return png;
}

static int fck_png_api_is_ok(fck_png png)
{
	return png.data != NULL;
}

static void fck_png_api_free(fck_png png)
{
	stbi_image_free(png.data);
}

static fck_png_api png_api = {
	.load = fck_png_api_load,
	.is_ok = fck_png_api_is_ok,
	.free = fck_png_api_free,
};

FCK_EXPORT_API fck_png_api*fck_png_load(fck_api_registry *registry, void *params)
{
	registry->add(fck_png_api_name, &png_api);
	return &png_api;
}