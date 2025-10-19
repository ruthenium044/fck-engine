#ifndef FCK_SERIALISER_JSON_VT_H_INCLUDED
#define FCK_SERIALISER_JSON_VT_H_INCLUDED

#include <fckc_inttypes.h>

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

struct fck_json_serialiser *fck_serialiser_json_writer_alloc(struct fck_json_serialiser *s, struct kll_allocator *a);

char *fck_serialiser_json_string_alloc(struct fck_json_serialiser *s);
void fck_serialiser_json_string_free(struct fck_json_serialiser *s, char *json);

void fck_serialiser_json_writer_free(struct fck_json_serialiser *);

#endif // FCK_SERIALISER_JSON_VT_H_INCLUDED