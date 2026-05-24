#ifndef FCK_PLUGINS_H_INCLUDED
#define FCK_PLUGINS_H_INCLUDED

typedef struct fck_plugin_api {
	void* (*load)(const char* path);
	void (*unload)(const char* path);
}fck_plugin_api;

#endif // !FCK_PLUGINS_H_INCLUDED