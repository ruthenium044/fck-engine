#ifndef FCK_APIS_H_IMPLEMENTED
#define FCK_APIS_H_IMPLEMENTED

#include <fckc_inttypes.h>

typedef struct fck_apis
{
	void (*add)(const char *name, void *api);
	void (*remove)(const char *name);
	void *(*find_from_hash)(fckc_u64 hash);
	void *(*find_from_string)(const char *name);
	void *(*next)(void *prev);
} fck_apis;

fck_apis *fck_apis_load(void);
void fck_apis_unload(fck_apis *);

#endif // !FCK_APIS_H_IMPLEMENTED