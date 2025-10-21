#define FCK_SER_JSON_EXPORT
#include "fck_serialiser_json_vt.h"
#include "fck_serialiser_vt.h"

#include "yyjson.h"

#include "kll.h"
#include "kll_malloc.h"

#include <assert.h>

// TODO: Fix this shit

static const fckc_size_t FCK_WRITE_BUFFER_STRING_SIZE = 64;
static const fckc_size_t FCK_READ_BUFFER_STRING_SIZE = 64;

void fck_write_json_i8(fck_serialiser *s, fck_serialiser_params *p, fckc_i8 *v, fckc_size_t c)
{
	yyjson_mut_doc *doc = (yyjson_mut_doc *)((fck_json_serialiser *)s)->doc;
	yyjson_mut_val *root = yyjson_mut_doc_get_root(doc);

	assert(doc && root);

	switch (c)
	{
	case 0:
		return;
	case 1:
		yyjson_mut_obj_add_int(doc, root, p->name, (fckc_i64)*v);
		break;
	default: {
		yyjson_mut_val *arr = yyjson_mut_arr_with_sint8(doc, v, c);
		yyjson_mut_obj_add_val(doc, root, p->name, arr);
	}
	break;
	}
}

void fck_write_json_i16(fck_serialiser *s, fck_serialiser_params *p, fckc_i16 *v, fckc_size_t c)
{
	yyjson_mut_doc *doc = (yyjson_mut_doc *)((fck_json_serialiser *)s)->doc;
	yyjson_mut_val *root = yyjson_mut_doc_get_root(doc);

	assert(doc && root);

	switch (c)
	{
	case 0:
		return;
	case 1:
		yyjson_mut_obj_add_int(doc, root, p->name, (fckc_i64)*v);
		break;
	default: {
		yyjson_mut_val *arr = yyjson_mut_arr_with_sint16(doc, v, c);
		yyjson_mut_obj_add_val(doc, root, p->name, arr);
	}
	break;
	}
}

void fck_write_json_i32(fck_serialiser *s, fck_serialiser_params *p, fckc_i32 *v, fckc_size_t c)
{
	yyjson_mut_doc *doc = (yyjson_mut_doc *)((fck_json_serialiser *)s)->doc;
	yyjson_mut_val *root = yyjson_mut_doc_get_root(doc);

	assert(doc && root);

	switch (c)
	{
	case 0:
		return;
	case 1:
		yyjson_mut_obj_add_int(doc, root, p->name, (fckc_i64)*v);
		break;
	default: {
		yyjson_mut_val *arr = yyjson_mut_arr_with_sint32(doc, v, c);
		yyjson_mut_obj_add_val(doc, root, p->name, arr);
	}
	break;
	}
}

void fck_write_json_i64(fck_serialiser *s, fck_serialiser_params *p, fckc_i64 *v, fckc_size_t c)
{
	yyjson_mut_doc *doc = (yyjson_mut_doc *)((fck_json_serialiser *)s)->doc;
	yyjson_mut_val *root = yyjson_mut_doc_get_root(doc);

	assert(doc && root);

	switch (c)
	{
	case 0:
		return;
	case 1:
		yyjson_mut_obj_add_int(doc, root, p->name, (fckc_i64)*v);
		break;
	default: {
		yyjson_mut_val *arr = yyjson_mut_arr_with_sint64(doc, v, c);
		yyjson_mut_obj_add_val(doc, root, p->name, arr);
	}
	break;
	}
}

void fck_write_json_u8(fck_serialiser *s, fck_serialiser_params *p, fckc_u8 *v, fckc_size_t c)
{
	yyjson_mut_doc *doc = (yyjson_mut_doc *)((fck_json_serialiser *)s)->doc;
	yyjson_mut_val *root = yyjson_mut_doc_get_root(doc);

	assert(doc && root);

	switch (c)
	{
	case 0:
		return;
	case 1:
		yyjson_mut_obj_add_uint(doc, root, p->name, (fckc_u64)*v);
		break;
	default: {
		yyjson_mut_val *arr = yyjson_mut_arr_with_uint8(doc, v, c);
		yyjson_mut_obj_add_val(doc, root, p->name, arr);
	}
	break;
	}
}

void fck_write_json_u16(fck_serialiser *s, fck_serialiser_params *p, fckc_u16 *v, fckc_size_t c)
{
	yyjson_mut_doc *doc = (yyjson_mut_doc *)((fck_json_serialiser *)s)->doc;
	yyjson_mut_val *root = yyjson_mut_doc_get_root(doc);

	assert(doc && root);

	switch (c)
	{
	case 0:
		return;
	case 1:
		yyjson_mut_obj_add_uint(doc, root, p->name, (fckc_u64)*v);
		break;
	default: {
		yyjson_mut_val *arr = yyjson_mut_arr_with_uint16(doc, v, c);
		yyjson_mut_obj_add_val(doc, root, p->name, arr);
	}
	break;
	}
}

void fck_write_json_u32(fck_serialiser *s, fck_serialiser_params *p, fckc_u32 *v, fckc_size_t c)
{
	yyjson_mut_doc *doc = (yyjson_mut_doc *)((fck_json_serialiser *)s)->doc;
	yyjson_mut_val *root = yyjson_mut_doc_get_root(doc);

	assert(doc && root);

	switch (c)
	{
	case 0:
		return;
	case 1:
		yyjson_mut_obj_add_uint(doc, root, p->name, (fckc_u64)*v);
		break;
	default: {
		yyjson_mut_val *arr = yyjson_mut_arr_with_uint32(doc, v, c);
		yyjson_mut_obj_add_val(doc, root, p->name, arr);
	}
	break;
	}
}

void fck_write_json_u64(fck_serialiser *s, fck_serialiser_params *p, fckc_u64 *v, fckc_size_t c)
{
	yyjson_mut_doc *doc = (yyjson_mut_doc *)((fck_json_serialiser *)s)->doc;
	yyjson_mut_val *root = yyjson_mut_doc_get_root(doc);

	assert(doc && root);

	switch (c)
	{
	case 0:
		return;
	case 1:
		yyjson_mut_obj_add_uint(doc, root, p->name, (fckc_u64)*v);
		break;
	default: {
		yyjson_mut_val *arr = yyjson_mut_arr_with_uint64(doc, v, c);
		yyjson_mut_obj_add_val(doc, root, p->name, arr);
	}
	break;
	}
}

void fck_write_json_f32(fck_serialiser *s, fck_serialiser_params *p, float *v, fckc_size_t c)
{
	yyjson_mut_doc *doc = (yyjson_mut_doc *)((fck_json_serialiser *)s)->doc;
	yyjson_mut_val *root = yyjson_mut_doc_get_root(doc);

	assert(doc && root);
	switch (c)
	{
	case 0:
		return;
	case 1:
		yyjson_mut_obj_add_float(doc, root, p->name, (float)*v);
		break;
	default: {
		yyjson_mut_val *arr = yyjson_mut_arr_with_float(doc, v, c);
		yyjson_mut_obj_add_val(doc, root, p->name, arr);
	}
	break;
	}
}

void fck_write_json_f64(fck_serialiser *s, fck_serialiser_params *p, double *v, fckc_size_t c)
{
	yyjson_mut_doc *doc = (yyjson_mut_doc *)((fck_json_serialiser *)s)->doc;
	yyjson_mut_val *root = yyjson_mut_doc_get_root(doc);

	assert(doc && root);

	switch (c)
	{
	case 0:
		return;
	case 1:
		yyjson_mut_obj_add_double(doc, root, p->name, (double)*v);
		break;
	default: {
		yyjson_mut_val *arr = yyjson_mut_arr_with_double(doc, v, c);
		yyjson_mut_obj_add_val(doc, root, p->name, arr);
	}
	break;
	}
}

void fck_write_json_string(fck_serialiser *s, fck_serialiser_params *p, fck_lstring *v, fckc_size_t c)
{
	assert(0 && "NOT SUPPORTED FOR NOW");
}

static fck_serialiser_vt fck_serialiser_json_writer_vt = {
	.i8 = fck_write_json_i8,
	.i16 = fck_write_json_i16,
	.i32 = fck_write_json_i32,
	.i64 = fck_write_json_i64,
	.u8 = fck_write_json_u8,
	.u16 = fck_write_json_u16,
	.u32 = fck_write_json_u32,
	.u64 = fck_write_json_u64,
	.f32 = fck_write_json_f32,
	.f64 = fck_write_json_f64,
	.string = fck_write_json_string,
};

fck_serialiser_vt *fck_json_writer_vt = &fck_serialiser_json_writer_vt;

static void *kll_json_malloc(void *ctx, size_t size)
{
	struct kll_allocator *alloacotor = (kll_allocator *)ctx;
	return kll_malloc(alloacotor, size);
}

static void *kll_json_realloc(void *ctx, void *ptr, size_t old_size, size_t size)
{
	struct kll_allocator *alloacotor = (kll_allocator *)ctx;
	return kll_realloc(alloacotor, ptr, size);
}

static void kll_json_free(void *ctx, void *ptr)
{
	struct kll_allocator *alloacotor = (kll_allocator *)ctx;
	kll_free(alloacotor, ptr);
}

struct fck_json_serialiser *fck_serialiser_json_writer_alloc(struct fck_json_serialiser *s, struct kll_allocator *a)
{
	yyjson_alc allocator;
	allocator.ctx = a;
	allocator.malloc = kll_json_malloc;
	allocator.realloc = kll_json_realloc;
	allocator.free = kll_json_free;

	yyjson_mut_doc *doc = yyjson_mut_doc_new(&allocator);
	yyjson_mut_val *root = yyjson_mut_obj(doc);
	yyjson_mut_doc_set_root(doc, root);

	s->vt = &fck_serialiser_json_writer_vt;
	s->allocator = a;
	s->doc = doc;
	return s;
}

char *fck_serialiser_json_string_alloc(struct fck_json_serialiser *s)
{
	yyjson_mut_doc *doc = (yyjson_mut_doc *)s->doc;
	char *json = yyjson_mut_write(doc, YYJSON_WRITE_PRETTY, NULL);
	return json;
}

void fck_serialiser_json_string_free(struct fck_json_serialiser *s, char *json)
{
	kll_json_free(s->allocator, (void *)json);
}

void fck_serialiser_json_writer_free(struct fck_json_serialiser *s)
{
	yyjson_doc_free(s->doc);
}

static fck_json_serialiser_api fck_json_ser_api = {
	.writer_alloc = fck_serialiser_json_writer_alloc,
	.writer_free = fck_serialiser_json_writer_free,
	.string_alloc = fck_serialiser_json_string_alloc,
	.string_free = fck_serialiser_json_string_free,
};

fck_json_serialiser_api* fck_ser_json = &fck_json_ser_api;