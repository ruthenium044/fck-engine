#ifndef FCK_SERIALISER_JSON_VT_H_INCLUDED
#define FCK_SERIALISER_JSON_VT_H_INCLUDED

#include "fckc_inttypes.h"

extern struct fck_serialiser_vt *fck_json_writer_vt;
extern struct fck_serialiser_vt *fck_json_reader_vt;

struct fck_serialiser;
struct kll_allocator;

struct fck_serialiser *fck_serliaser_json_writer_alloc(struct fck_serialiser* s, struct kll_allocator* a);

const char *fck_serliaser_json_string_alloc(struct fck_serialiser* s);
const char* fck_serliaser_json_string_free(struct fck_serialiser* s, const char* json);

struct fck_serialiser *fck_serliaser_json_writer_free(struct fck_serialiser *);

#endif // FCK_SERIALISER_JSON_VT_H_INCLUDED