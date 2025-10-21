#ifndef FCK_APIS_H_IMPLEMENTED
#define FCK_APIS_H_IMPLEMENTED

#include <fckc_inttypes.h>
#include <fckc_apidef.h>

#if defined(FCK_APIS_EXPORT)
#define FCK_APIS_API FCK_EXPORT_API
#else
#define FCK_APIS_API FCK_IMPORT_API
#endif

typedef struct fck_apis
{
	void (*add)(const char *name, void *api);
	void *(*get)(fckc_u64 hash);
	void *(*find)(const char *name);
	int (*remove)(const char *name);
	void *(*next)(void *prev);
} fck_apis;

FCK_APIS_API extern fck_apis* apis;

#endif // !FCK_APIS_H_IMPLEMENTED