
#ifndef FCK_IMG_H_INCLUDED
#define FCK_IMG_H_INCLUDED

#include <fckc_inttypes.h>

struct kll_allocator;

typedef struct fck_img
{
	struct kll_allocator *allocator;
	void *pixels;
	fckc_size_t width;
	fckc_size_t height;
	fckc_size_t pitch;
} fck_img;

typedef struct fck_img_api
{
	fck_img (*load)(struct kll_allocator *, const char *path);
	fck_img (*copy)(struct kll_allocator *, fck_img);
	int (*is_valid)(fck_img);
	void (*free)(fck_img);
} fck_img_api;

#endif // !FCK_IMG_H_INCLUDED
