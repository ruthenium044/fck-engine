#ifndef FCK_TUD_H_INCLUDED
#define FCK_TUD_H_INCLUDED

typedef struct fck_canvas
{
	void* handle;
}fck_canvas;

typedef struct fck_canvas_api
{
	fck_canvas(*create)(void);
} fck_canvas_api;

#endif // FCK_TUD_H_INCLUDED