#ifndef FCK_APIS_H_IMPLEMENTED
#define FCK_APIS_H_IMPLEMENTED

#include <fckc_inttypes.h>

typedef fckc_u32 fck_api_bool;

typedef struct fck_apis
{
	void (*add)(const char *name, void *api);
	void *(*get)(fckc_u64 hash);
	void *(*find)(const char *name);
	fck_api_bool (*remove)(const char *name);
	void *(*next)(void *prev);
} fck_apis;

fck_apis *fck_apis_load(void);
void fck_apis_unload(fck_apis *apis);

#endif // !FCK_APIS_H_IMPLEMENTED