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
	fckc_i64       timestamp;
	sht_image      image;
	sht_image_view view;
} fck_texture_resolved;

typedef struct fck_texture_asset
{
	fck_texture          textue;
	fck_texture_resolved resolved;
} fck_texture_asset;

static sht_image fck_texture_upload_image_on_gpu(sht_driver driver, sht_image image, const void *pixels, int width, int height)
{
	const fckc_size_t size = (fckc_size_t)width * height * 4;
	driver.vt->upload_image(driver, &image, pixels, size);
	return image;
}

static sht_image fck_texture_load_image_on_gpu(sht_driver driver, const void *pixels, sht_format format, int width, int height)
{
	sht_memory                   *memory = driver.vt->memory(driver);
	const sht_image_configuration config = {
		.format   = format,
		.height   = to_u32(height),
		.width    = to_u32(width),
		.transfer = sht_transfer_target,
		.usage    = sht_image_usage_sampled,
	};

	sht_image image = memory->image->create(memory->bump, &config, sht_memory_gpu);
	if (!memory->image->is_ok(&image))
	{
		return image;
	}
	fck_texture_upload_image_on_gpu(driver, image, pixels, width, height);
	return image;
}

static struct sht_image_view *fck_texture_asset_resolve(const fck_db_asset *asset, sht_driver *driver)
{
	if (strcmp(asset->category, fck_category_texture) != 0)
	{
		// Uuhhh... Maybe a return value would
		return NULL;
	}

	fck_texture_asset *value = (fck_texture_asset *)asset->userdata;
	if (asset->timestamp >= value->resolved.timestamp)
	{
		sht_memory *memory = driver->vt->memory(*driver);
		if (memory->image->is_ok(&value->resolved.image))
		{
			driver->vt->idle(*driver);
			memory->image->discard(memory->bump, &value->resolved.view);
			memory->image->destroy(memory->bump, &value->resolved.image);
		}

		const sht_format format = sht_format_r8g8b8a8_unorm;
		const int        width  = value->textue.width;
		const int        height = value->textue.height;

		value->resolved.image     = fck_texture_load_image_on_gpu(*driver, value->textue.data, format, width, height);
		value->resolved.view      = memory->image->view(memory->bump, value->resolved.image, format);
		value->resolved.timestamp = os->chrono->now();
		*asset->state             = fck_db_asset_state_resolved;
	}
	return &value->resolved.view;
	;
	// return &value->resolved.view;
}

static void *fck_texture_import(const fck_db_loader_args *args, const char *file)
{
	os->io->log("Load PNG: %s", file);
	// TODO: We should hand around uuid, maybe...
	{
		const fck_db_accessor accessor = args->api->object->edit(args->db, args->target);

		fck_texture_asset *asset = (fck_texture_asset *)accessor.read->userdata(accessor, fck_category_texture);
		if (asset == NULL)
		{
			fck_texture_asset value = {.resolved.timestamp = os->chrono->now()};

			asset = (fck_texture_asset *)accessor.edit->userdata(accessor, fck_category_texture, &value, sizeof(value));
		}

		const fck_texture previous = asset->textue;
		const fck_texture texture  = fck_texture_api_load(file);
		asset->textue              = texture;
		accessor.edit->commit(accessor, fck_db_no_undo);

		if (fck_texture_api_is_ok(previous))
		{
			fck_texture_api_free(previous);
		}
		return asset;
	}
}

static fckc_size_t fck_texture_supports(const char ***extensions)
{
	static const char *supported[] = {"png", "jpg"};
	*extensions                    = supported;
	return fck_arraysize(supported);
}

static const fck_texture *fck_texture_data(const fck_db_asset *asset)
{
	fck_texture_asset *value = (fck_texture_asset *)asset->userdata;
	return &value->textue;
}

static sht_image_view *fck_texture_gpu(const fck_db_asset *asset)
{
	fck_texture_asset *value = (fck_texture_asset *)asset->userdata;

	switch (*asset->state)
	{
	case fck_db_asset_state_requested:
	case fck_db_asset_state_resolved:
		break;
	case fck_db_asset_state_imported:
		*asset->state = fck_db_asset_state_requested;
		break;
	}

	return &value->resolved.view;
}

static fck_texture_asset_api png_asset_api = {
	.resolve = fck_texture_asset_resolve,
	.cpu     = fck_texture_data,
	.gpu     = fck_texture_gpu,
};

static fck_texture_api png_api = {
	.asset = &png_asset_api,
	.load  = fck_texture_api_load,
	.is_ok = fck_texture_api_is_ok,
	.free  = fck_texture_api_free,
};

static fck_db_loader_interface png_loader = {
	.category = fck_category_texture,
	.import   = fck_texture_import,
	.supports = fck_texture_supports,
};

FCK_EXPORT_API fck_texture_api *fck_texture_load(fck_api_registry *registry, void *old)
{
	(void)old;
	registry->add(fck_db_loader_interface_name, &png_loader);
	registry->add(fck_texture_api_name, &png_api);
	return &png_api;
}