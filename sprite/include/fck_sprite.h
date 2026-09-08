#ifndef FCK_SPRITES_H_INCLUDED
#define FCK_SPRITES_H_INCLUDED

#include <fckc_inttypes.h>

#define fck_sprite_api_name "fck-sprite"

struct fck_sprites;
struct kll_allocator;
struct sht_image_view;
struct sht_driver;
struct sht_command_buffer;
struct fck_db_asset;

struct fck_db;

typedef struct fck_sprite_transform
{
	float x;
	float y;
	float z;
	float rotation;
	float scale;
	int horizontal_index;
	int vertical_index;
} fck_sprite_transform;

typedef struct fck_sprite_batch_id
{
	fckc_u32 value;
} fck_sprite_batch_id;

typedef struct fck_sprite_entry_id
{
	fckc_u32 value;
} fck_sprite_entry_id;

typedef struct fck_sprite_id
{
	fck_sprite_batch_id batch;
	fck_sprite_entry_id entry;
} fck_sprite_id;

typedef struct fck_sprites
{
	fckc_u8 opaque[256];
} fck_sprites;

// ... Something like this... we gotta see
typedef struct fck_sprite_batch_api
{
	fck_sprite_batch_id (*add)(fck_sprites *sprites, const char *name, const struct fck_db_asset *asset, float sw, float sh);
	int (*remove)(fck_sprites *sprites, fck_sprite_batch_id index);

	struct sht_image_view *(*image_view)(struct fck_sprites *sprites, fck_sprite_batch_id index);
	const struct fck_db_asset *(*asset)(struct fck_sprites *sprites, fck_sprite_batch_id index);
	int (*dimensions)(fck_sprites *sprites, fck_sprite_batch_id index, float *sprite_width, float *sprite_height);

	const char *(*nameof)(fck_sprites *sprites, fck_sprite_batch_id index);

	fck_sprite_batch_id (*find_by_name)(fck_sprites *sprites, const char *name);

	fck_sprite_batch_id (*index)(fck_sprites *sprites, fckc_u32 index);
	int (*is_ok)(fck_sprites *sprites, fck_sprite_batch_id index);

	fckc_u32 (*count)(fck_sprites *sprites);
	fckc_u32 (*names)(fck_sprites *sprites, const char ***out_names);
} fck_sprite_batch_api;

typedef struct fck_sprite_create_args
{
	struct fck_db *db;
	struct sht_driver *driver;
} fck_sprite_create_args;

typedef struct fck_sprite_api
{
	fck_sprite_batch_api *batches;

	struct fck_sprites (*create)(struct kll_allocator *allocator, const fck_sprite_create_args *args);
	void (*destroy)(fck_sprites *sprites);

	fckc_u32 (*transforms)(fck_sprites *sprites, fck_sprite_batch_id index, fck_sprite_transform **out_transforms);

	fck_sprite_transform *(*get)(fck_sprites *sprites, fck_sprite_id index);
	fck_sprite_transform *(*set)(fck_sprites *sprites, fck_sprite_id index);
	fck_sprite_transform *(*add)(fck_sprites *sprites, fck_sprite_batch_id index);

	// Not a fan of this one...
	int (*remove)(fck_sprites *sprites, fck_sprite_id index);

	void (*present)(fck_sprites *sprites, const struct sht_command_buffer *buffer, fckc_u32 frame_index);

	fck_sprite_id (*indexof)(fck_sprites *sprites, fck_sprite_batch_id index, const fck_sprite_transform *transform);

	fck_sprite_id (*invalid)(void);

	int (*is_ok)(fck_sprites *sprites, fck_sprite_id index);
} fck_sprite_api;

#endif // !FCK_SPRITES_H_INCLUDED
