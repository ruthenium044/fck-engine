#ifndef FCK_APIS_H_IMPLEMENTED
#define FCK_APIS_H_IMPLEMENTED

#include <fckc_inttypes.h>
#include <fckc_apidef.h>

#define FCK_ENTRY_POINT "fck_main"

typedef struct fck_apis
{
	void (*add)(const char *name, void *api);
	void *(*get)(fckc_u64 hash);
	void *(*find)(const char *name);
	int (*remove)(const char *name);
	void *(*next)(void *prev);
} fck_apis;

typedef void* (fck_main_func)(fck_apis*, void*);

typedef struct fck_apis_manifest {
	void** api;
	const char* name;
	void* params;
}fck_apis_manifest;

typedef struct fck_apis_init {
	fck_apis_manifest* manifest;
	fckc_size_t count;
}fck_apis_init;

#endif // !FCK_APIS_H_IMPLEMENTED