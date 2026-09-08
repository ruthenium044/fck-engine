#ifndef FCK_GFX_H_INCLUDED
#define FCK_GFX_H_INCLUDED

#define fck_gfx_api_name "fck-gfx"

#include <fckc_inttypes.h>

typedef struct fck_gfx
{
	void *handle;
} fck_gfx;

struct sht_driver;
struct sht_swapchain_state;

// TODO: We should not need to pass this one around
struct fck_shader_api;

struct sht_bss;
struct sht_graphics_pipeline;
struct sht_command_buffer;
struct sht_render_desc;

struct kll_allocator;
struct fck_db_asset;

typedef struct fck_gfx_args
{
	const struct sht_command_buffer  *commands;
	const struct sht_render_desc     *suggestion;
	const struct sht_driver          *driver;
	const struct sht_swapchain_state *swapchain;
} fck_gfx_args;

typedef int(fck_gfx_draw)(void *userdata, fck_gfx_args *args);

typedef enum fck_gfx_target_flags
{
	fck_gfx_target_default    = 0,
	fck_gfx_target_no_colour  = 1 << 0,
	fck_gfx_target_depth      = 1 << 1,
	fck_gfx_target_depth_only = fck_gfx_target_no_colour | fck_gfx_target_depth,
	fck_gfx_target_all        = fck_gfx_target_depth,
} fck_gfx_target_flags;

typedef struct fck_gfx_create_info
{
	fck_alias(fck_gfx_target_flags, fckc_u32) flags;
	const struct fck_db_asset *vertex;
	const struct fck_db_asset *fragment;
} fck_gfx_create_info;

typedef struct fck_gfx_api
{
	struct fck_gfx                (*create)(struct kll_allocator *allocator, struct sht_driver *driver, const fck_gfx_create_info *info);
	struct sht_bss               *(*bss)(fck_gfx gfx);
	struct sht_graphics_pipeline *(*pipeline)(fck_gfx gfx);
} fck_gfx_api;

#endif // !FCK_GFX_H_INCLUDED
