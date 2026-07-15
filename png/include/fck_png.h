#ifndef FCK_PNG_H_IMPLEMENTED
#define FCK_PNG_H_IMPLEMENTED

#define fck_png_api_name "fck-png"

// TODO: Do not name it png, it can actually load more than that
typedef struct fck_png
{
	void *data;
	int width;
	int height;
	int channels;
} fck_png;

typedef struct fck_png_api
{
	fck_png (*load)(const char *path);
	int (*is_ok)(fck_png png);
	void (*free)(fck_png png);
} fck_png_api;

#endif // !FCK_PNG_H_IMPLEMENTED
