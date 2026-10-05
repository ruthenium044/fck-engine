#ifndef FCK_SPRITES_H_INCLUDED
#define FCK_SPRITES_H_INCLUDED

#include <fckc_inttypes.h>

#define fck_sprite_api_name "fck-sprite"

#define fck_batchy_api_name "fck-batchy"

struct fck_sprites;
struct kll_allocator;
struct sht_image_view;
struct sht_driver;
struct sht_command_buffer;
struct fck_db_asset;

struct fck_gfx_args;

struct fck_db;

// TODO! Rewrite this using  :(

typedef struct fck_sprite_transform
{
	fckc_f32 x;
	fckc_f32 y;
	fckc_f32 z;
	fckc_f32 rotation;
	fckc_f32 scale;
	fckc_u32 horizontal_index;
	fckc_u32 vertical_index;
} fck_sprite_transform;

struct fck_batchy_private;
typedef struct fck_batchy
{
	// Still trying to find this perfect one-letter variable name for private state
	struct fck_batchy_private *o;
} fck_batchy;

typedef struct fck_batchy_create_args
{
	struct fck_db     *db;
	struct sht_driver *driver;
} fck_batchy_create_args;

typedef struct fck_batchy_batch
{
	fckc_u64 value;
} fck_batchy_batch;

typedef struct fck_batchy_sprite
{
	fck_batchy_batch batch;
	fckc_u64         value;
} fck_batchy_sprite;

typedef struct fck_batchy_batch_configuration
{
	const struct fck_db_asset *asset;

	float sprite_width;
	float sprite_height;
} fck_batchy_batch_configuration;

// Wait, let's simplify it
// We always reconstruct the batches each frame instead of applying individual changes!
typedef struct fck_batchy_api
{
	fck_batchy (*create)(struct kll_allocator *allocator, const fck_batchy_create_args *args);
	void       (*destroy)(fck_batchy batchy);

	fckc_size_t           (*count)(fck_batchy batchy);
	fck_batchy_batch      (*add)(fck_batchy batchy, const fck_batchy_batch_configuration *config);
	void                  (*remove)(fck_batchy batchy, fck_batchy_batch batch);
	void                  (*clear)(fck_batchy batchy, fck_batchy_batch batch);
	fck_sprite_transform *(*batchup)(fck_batchy batchy, fck_batchy_batch batch);

	//	while(batchy->iterate(world, &batch))
	int         (*iterate)(fck_batchy batchy, fck_batchy_batch **batch);
	//		if(batchy->inspect(batchy, *batch, &config)
	int         (*inspect)(fck_batchy batchy, fck_batchy_batch batch, fck_batchy_batch_configuration *config);
	//			const fckc_size_t result = batchy->batch(world, *batch, *out)
	//			for(fckc_size_t index = 0; index < result; index++)
	fckc_size_t (*batch)(fck_batchy batchy, fck_batchy_batch batch, fck_sprite_transform **transforms);

	// TODO: Having scope part of this is a bit MEEEH
	const struct fck_db_asset *(*save)(fck_batchy batchy, const char *scope, const char *path);

	// TODO:
	int (*present)(void *userdata, const struct fck_gfx_args *args);
} fck_batchy_api;

#endif // !FCK_SPRITES_H_INCLUDED
