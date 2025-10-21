#ifndef FCK_SERIALISER_JSON_VT_H_INCLUDED
#define FCK_SERIALISER_JSON_VT_H_INCLUDED

#include <fckc_inttypes.h>
#include <fckc_apidef.h>

#if defined(FCK_SER_JSON_EXPORT)
#define FCK_SER_JSON_API FCK_EXPORT_API
#else
#define FCK_SER_JSON_API FCK_IMPORT_API
#endif

extern struct fck_serialiser_vt *fck_json_writer_vt;
extern struct fck_serialiser_vt *fck_json_reader_vt;

struct kll_allocator;
struct yyjson_mut_doc;

typedef struct fck_json_serialiser
{
	struct fck_serialiser_vt *vt;
	struct kll_allocator *allocator;

	struct yyjson_mut_doc *doc;

} fck_json_serialiser;

typedef struct fck_json_serialiser_api
{
	struct fck_json_serialiser* (*writer_alloc)(struct fck_json_serialiser* s, struct kll_allocator* a);
	void (*writer_free)(struct fck_json_serialiser* s);

	char* (*string_alloc)(struct fck_json_serialiser* s);
	void (*string_free)(struct fck_json_serialiser* s, char* json);
}fck_json_serialiser_api;

FCK_SER_JSON_API extern fck_json_serialiser_api* fck_ser_json;

#endif // FCK_SERIALISER_JSON_VT_H_INCLUDED