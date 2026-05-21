#ifndef FCK_APIS_H_IMPLEMENTED
#define FCK_APIS_H_IMPLEMENTED

#include <fckc_inttypes.h>

typedef struct fck_api_registry
{
	int (*add)(const char *name, void *api);
	fckc_size_t (*implementations)(const char *name, void ***apis);
	void *(*find)(const char *name);
	int (*remove)(const char *name, void *api);
	const char* (*nameof)(void* api);
} fck_api_registry;

// Both arguments are allowed to be NULL!
typedef void *(fck_main_func)(fck_api_registry * registry, void *old_implementation);

#endif // !FCK_APIS_H_IMPLEMENTED