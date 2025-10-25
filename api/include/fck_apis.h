#ifndef FCK_APIS_H_IMPLEMENTED
#define FCK_APIS_H_IMPLEMENTED

#include <fckc_inttypes.h>
#include <fckc_apidef.h>

typedef struct fck_apis
{
	void (*add)(const char *name, void *api);
	void *(*get)(fckc_u64 hash);
	void *(*find)(const char *name);
	int (*remove)(const char *name);
	void *(*next)(void *prev);
} fck_apis;

fck_apis* apis;

#endif // !FCK_APIS_H_IMPLEMENTED