
#include <fck_apis.h>
#include <fckc_apidef.h>

#include <fck_db.h>
#include <fck_nuklear.h>
#include <fck_texture.h>

static fck_api_registry *apis;

static void fck_nuklear_texture_preview(const fck_nk_asset_preview_args *args)
{
	fck_texture_api *texture = (fck_texture_api *)apis->find(fck_texture_api_name);
	struct sht_image_view *image_view = texture->asset->gpu(args->asset);
	args->nk->element->image(args->view, image_view, fck_nk_image_fill);
}

static fck_nk_asset_preview_interface nuklear_texture_preview = {
	.category = fck_category_texture,
	.preview = fck_nuklear_texture_preview,
};

FCK_EXPORT_API void *fck_nuklear_texture_load(fck_api_registry *registry, void *old)
{
	(void)old;
	apis = registry;
	registry->add(fck_nuklear_asset_preview_interface, &nuklear_texture_preview);
	return &nuklear_texture_preview;
}