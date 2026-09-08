#ifndef FCK_PLUGINS_H_INCLUDED
#define FCK_PLUGINS_H_INCLUDED

#include <fckc_inttypes.h>

#define fck_plugins_api_name "fck-plugins"

#if defined(_WIN32) || defined(_WIN64)
#define fck_plugin_extension ".dll"
#elif defined(__APPLE__) && defined(__MACH__)
#define fck_plugin_extension ".dylib"
#elif defined(__unix__) || defined(__unix) || defined(__linux__)
#define fck_plugin_extension ".so"
#else
#error "Unsupported platform: unknown shared object extension"
#endif

struct fck_shared_object;

typedef struct fck_plugins_api
{
	fckc_u32 (*hotreload)(void);

	// TODO: Nice to list all that shit! :)
	const char *(*loaded)(const char *prev);
	const char *(*unloaded)(const char *prev);

	void (*root)(const char *path);

	void *(*load)(const char *path);
	void  (*unload)(const char *path);
	void  (*shutdown)(void);

	const struct fck_shared_object *(*so)(const char *path);
} fck_plugins_api;

#endif // !FCK_PLUGINS_H_INCLUDED