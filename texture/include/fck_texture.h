#ifndef fck_texture_H_IMPLEMENTED
#define fck_texture_H_IMPLEMENTED

#define fck_texture_api_name "fck-texture"

struct sht_driver;
struct sht_image_view;

typedef struct fck_texture_asset fck_texture_asset;

typedef struct fck_texture
{
	void *data;
	int width;
	int height;
	int channels;
} fck_texture;

typedef struct fck_texture_asset_api
{
	// TODO: Make it possible to create assets from memory!
	struct sht_image_view *(*resolve)(struct fck_texture_asset *asset, struct sht_driver *driver);
} fck_texture_asset_api;

typedef struct fck_texture_api
{
	fck_texture_asset_api *asset;

	fck_texture (*load)(const char *path);
	int (*is_ok)(fck_texture png);
	void (*free)(fck_texture png);
} fck_texture_api;

#endif // !fck_texture_H_IMPLEMENTED
