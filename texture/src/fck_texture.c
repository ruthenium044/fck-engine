#include "fck_texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include "fck_db.h"
#include "fck_os.h"
#include "sht_render.h"
#include "stb_image.h"

#include <fck_apis.h>
#include <fckc_apidef.h>
#include <kll.h>
#include <kll_malloc.h>

#include <fckc_assert.h>
#include <fckc_inttypes.h>

#include <stddef.h>
#include <string.h>

static fck_texture fck_texture_api_load(const char *path)
{
	fck_texture png;
	png.data = stbi_load(path, &png.width, &png.height, &png.channels, 0);
	if (png.data == NULL)
	{
		memset(&png, 0, sizeof(png));
	}
	return png;
}

static int fck_texture_api_is_ok(fck_texture png)
{
	return png.data != NULL;
}

static void fck_texture_api_free(fck_texture png)
{
	stbi_image_free(png.data);
}

typedef struct fck_texture_resolved
{
	fckc_i64 timestamp;
	sht_image image;
	sht_image_view view;
} fck_texture_resolved;

typedef struct fck_texture_asset
{
	fck_db_asset base;
	fck_texture value;
	fck_texture_resolved resolved;
} fck_texture_asset;

static sht_image fck_texture_upload_image_on_gpu(sht_driver driver, sht_image image, const void *pixels, sht_format format, int width,
                                                 int height)
{
	const fckc_size_t size = (fckc_size_t)width * height * 4;
	driver.vt->upload_image(driver, &image, pixels, size);
	return image;
}

static sht_image fck_texture_load_image_on_gpu(sht_driver driver, const void *pixels, sht_format format, int width, int height)
{
	sht_memory *memory = driver.vt->memory(driver);
	const sht_image_configuration config = {
		.format = format,
		.height = to_u32(height),
		.width = to_u32(width),
		.transfer = sht_transfer_target,
		.usage = sht_image_usage_sampled,
	};

	sht_image image = memory->image->create(memory->bump, &config, sht_memory_gpu);
	if (!memory->image->is_ok(&image))
	{
		return image;
	}
	fck_texture_upload_image_on_gpu(driver, image, pixels, format, width, height);
	return image;
}

static sht_image_view *fck_texture_asset_resolve(fck_texture_asset *asset, sht_driver *driver)
{
	sht_memory *memory = driver->vt->memory(*driver);
	if (asset->base.timestamp > asset->resolved.timestamp)
	{
		if (asset->value.data != NULL)
		{
			if (memory->image->is_ok(&asset->resolved.image))
			{
				driver->vt->idle(*driver);
				memory->image->discard(memory->bump, &asset->resolved.view);
				memory->image->destroy(memory->bump, &asset->resolved.image);
			}

			const sht_format format = sht_format_r8g8b8a8_unorm;
			asset->resolved.image =
				fck_texture_load_image_on_gpu(*driver, asset->value.data, format, asset->value.width, asset->value.height);
			asset->resolved.view = memory->image->view(memory->bump, asset->resolved.image, format);
			asset->resolved.timestamp = os->chrono->now();
		}
	}
	return &asset->resolved.view;
}

static fck_db_asset *fck_texture_import(const fck_db_loader_args *args, const char *file)
{
	os->io->log("Load PNG: %s", file);

	fck_texture_asset *asset = (fck_texture_asset *)args->api->asset->get(args->db, args->target);
	if (asset)
	{
		fck_assert(asset->base.size = sizeof(*asset));
		fck_texture_api_free(asset->value);
		asset->value = fck_texture_api_load(file);
		asset->base.timestamp = os->chrono->now();
		return &asset->base;
	}
	{
		const fck_texture value = fck_texture_api_load(file);
		fck_texture_asset *asset = (fck_texture_asset *)kll_malloc(kll->system, sizeof(*asset));
		memset(asset, 0, sizeof(*asset));
		asset->value = value;
		asset->base.timestamp = os->chrono->now();
		asset->base.size = sizeof(*asset);
		return &asset->base;
	}
}

static fckc_size_t fck_texture_supports(const char ***extensions)
{
	static const char *supported[] = {"png", "jpg"};
	*extensions = supported;
	return fck_arraysize(supported);
}

static fck_texture_asset_api png_asset_api = {
	.resolve = fck_texture_asset_resolve,
};

static fck_texture_api png_api = {
	.asset = &png_asset_api,
	.load = fck_texture_api_load,
	.is_ok = fck_texture_api_is_ok,
	.free = fck_texture_api_free,
};

static fck_db_loader_interface png_loader = {
	.type = "texture",
	.name = "png",
	.import = fck_texture_import,
	.supports = fck_texture_supports,
};

FCK_EXPORT_API fck_texture_api *fck_texture_load(fck_api_registry *registry, void *params)
{
	registry->add(fck_db_loader_interface_name, &png_loader);

	registry->add(fck_texture_api_name, &png_api);
	return &png_api;
}