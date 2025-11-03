#ifndef FCK_APP_LOADABLE_H_INCLUDED
#define FCK_APP_LOADABLE_H_INCLUDED

struct fck_api_registry;
struct fck_app;
struct fck_event;

typedef struct fck_app_api
{
	struct fck_app *(*init)(int argc, char *argv[]);
	void (*quit)(struct fck_app *instance);
	int (*on_event)(struct fck_app* instance, struct fck_event const* event);
	int (*tick)(struct fck_app *instance);
} fck_app_api;

typedef struct fck_app
{
	// Shared read-write - IT does not really matter who fills this one out, the caller of init is assumed to overwrite it
	struct fck_app_api *api;
	const char *name;
} fck_app;

#define FCK_APP_LOAD "fck_load"

typedef fck_app_api *(fck_load_func)(void);

#endif // !FCK_APP_LOADABLE_H_INCLUDED
