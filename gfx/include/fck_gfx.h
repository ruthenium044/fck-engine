#ifndef FCK_GFX_H_INCLUDED
#define FCK_GFX_H_INCLUDED

#define fck_gfx_api_name "fck-gfx"

typedef struct fck_gfx
{
	void *handle;
} fck_gfx;

struct sht_driver;

// TODO: We should not need to pass this one around
struct fck_shader_api;

struct sht_bss;
struct sht_graphics_pipeline;

struct kll_allocator;

// Instead of this thing, we want to use fck_shader_asset
typedef struct fck_gfx_shader
{
	const char *name;
	const char *path;
} fck_gfx_shader;

// typedef enum fck_gfx_target_flags
// {
//	fck_gfx_target_colour = 1 << 0,
//	fck_gfx_target_depth = 1 << 1
// } fck_gfx_target_flags;

struct fck_db_asset;

typedef struct fck_gfx_create_info
{
	// This is not so elegant - Instead of boolean configuration always prefer more explicit settings
	// fck_gfx_target_flags flags;
	// const fck_shader_asset *vs;
	// const fck_shader_asset *fs;
	int has_depth;
	const struct fck_db_asset *vertex;
	const struct fck_db_asset *fragment;
} fck_gfx_create_info;

typedef struct fck_gfx_api
{
	struct fck_gfx (*create)(struct kll_allocator *allocator, struct sht_driver *driver, const fck_gfx_create_info *info);
	struct sht_bss *(*bss)(fck_gfx gfx);
	struct sht_graphics_pipeline *(*pipeline)(fck_gfx gfx);
} fck_gfx_api;

#endif // !FCK_GFX_H_INCLUDED
