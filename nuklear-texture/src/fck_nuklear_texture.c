
#include <fck_apis.h>
#include <fckc_apidef.h>

#include <fck_db.h>
#include <fck_nuklear.h>
#include <fck_texture.h>

static fck_api_registry *apis;

// LMAO, I do not thing I will let this one stay
#define fck_asset_state_machine(asset, resolved, requested)                                                                                \
	switch (*(asset)->state)                                                                                                               \
	{                                                                                                                                      \
	case fck_db_asset_state_resolved:                                                                                                      \
		({resolved});                                                                                                                      \
		break;                                                                                                                             \
	case fck_db_asset_state_imported:                                                                                                      \
		*(asset)->state = fck_db_asset_state_requested;                                                                                    \
	case fck_db_asset_state_requested:                                                                                                     \
		({requested});                                                                                                                     \
		break;                                                                                                                             \
	}

static void fck_nuklear_texture_preview(const fck_nk_asset_preview_args *args)
{
	fck_texture_api *texture = (fck_texture_api *)apis->find(fck_texture_api_name);
	// TODO: we want to do a full stretch! So we should query the currently available size of the panel! :)
	/*switch (*args->asset->state)
	{
	case fck_db_asset_state_resolved: {
	    struct sht_image_view *image_view = texture->asset->gpu(args->asset);
	    args->nk->element->image(args->view, image_view, 200.0f);
	    break;
	}
	case fck_db_asset_state_imported:
	    *args->asset->state = fck_db_asset_state_requested;
	case fck_db_asset_state_requested: {
	    const fck_texture *data = texture->asset->cpu(args->asset);
	    args->nk->element->rect(args->view, 200.f, 200.0f, (fck_nk_colour){255, 0, 255, 255});
	    break;
	}
	}*/

	fck_asset_state_machine(
		args->asset,
		{
			struct sht_image_view *image_view = texture->asset->gpu(args->asset);
			args->nk->element->image(args->view, image_view, 200.0f);
		},
		{
			const fck_texture *data = texture->asset->cpu(args->asset);
			args->nk->element->rect(args->view, 200.f, 200.0f, (fck_nk_colour){255, 0, 255, 255});
		});
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