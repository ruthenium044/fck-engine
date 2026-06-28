#ifndef FCK_GFX_H_INCLUDED
#define FCK_GFX_H_INCLUDED

#define fck_gfx_api_name "fck_gfx"

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

typedef struct fck_gfx_shader
{
	const char *name;
	const char *path;
} fck_gfx_shader;

typedef struct fck_gfx_create_info
{
	const fck_gfx_shader *vertex;
	const fck_gfx_shader *fragment;
} fck_gfx_create_info;

typedef struct fck_gfx_api
{
	// TODO: Destroy
	struct fck_gfx (*create)(struct kll_allocator *allocator, struct sht_driver *driver, struct fck_shader_api *shader,
	                          const fck_gfx_create_info *info);
	struct sht_bss *(*bss)(fck_gfx gfx);
	struct sht_graphics_pipeline *(*pipeline)(fck_gfx gfx);
} fck_gfx_api;

extern fck_gfx_api *gfx;

#endif // !FCK_GFX_H_INCLUDED
