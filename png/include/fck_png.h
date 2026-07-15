#ifndef FCK_PNG_H_IMPLEMENTED
#define FCK_PNG_H_IMPLEMENTED

#define fck_png_api_name "fck-png"

struct sht_driver;
struct sht_image_view;

typedef struct fck_png_asset fck_png_asset;

// TODO: Do not name it png, it can actually load more than that
typedef struct fck_png
{
	void *data;
	int width;
	int height;
	int channels;
} fck_png;

typedef struct fck_png_asset_api
{
	// TODO: Make it possible to create assets from memory! 
	struct sht_image_view *(*resolve)(struct fck_png_asset *asset, struct sht_driver *driver);
} fck_png_asset_api;

typedef struct fck_png_api
{
	fck_png_asset_api *asset;

	fck_png (*load)(const char *path);
	int (*is_ok)(fck_png png);
	void (*free)(fck_png png);
} fck_png_api;

#endif // !FCK_PNG_H_IMPLEMENTED
