#ifndef FCK_PLUGINS_H_INCLUDED
#define FCK_PLUGINS_H_INCLUDED

#include <fckc_inttypes.h>

#define fck_plugins_api_name "fck_plugins"

typedef struct fck_plugins_api {
	fckc_u32 (*hotreload)(void);

	// TODO: Nice to list all that shit! :) 
	const char** (*loaded)(void);
	const char** (*unloaded)(void);

	void (*root)(const char* path);

	void* (*load)(const char* path);
	void (*unload)(const char* path);
	void (*shutdown)(void);
}fck_plugins_api;

#endif // !FCK_PLUGINS_H_INCLUDED