#ifndef FCK_SERIALISER_JSON_VT_H_INCLUDED
#define FCK_SERIALISER_JSON_VT_H_INCLUDED

extern struct fck_serialiser_vt *fck_json_writer_vt;
extern struct fck_serialiser_vt *fck_json_reader_vt;

struct fck_serialiser;
struct kll_allocator;

struct fck_serialiser *fck_serialiser_json_writer_alloc(struct fck_serialiser *s, struct kll_allocator *a);

char *fck_serialiser_json_string_alloc(struct fck_serialiser *s);
void fck_serialiser_json_string_free(struct fck_serialiser *s, char *json);

void fck_serialiser_json_writer_free(struct fck_serialiser *);

#endif // FCK_SERIALISER_JSON_VT_H_INCLUDED