#include "fck_serialiser_json.h"
#include "fck_serialiser.h"
#include <fckc_inttypes.h>

#include <kll.h>
#include <kll_malloc.h>

#include "yyjson.h"
#include <string.h>

typedef struct fck_json_writer
{
	fck_serialiser base;
	kll_allocator *allocator;
	yyjson_mut_doc *doc;
	yyjson_mut_val *stack[64];
	int stack_top;
	fckc_char *final_buffer;
	fckc_size_t final_len;
} fck_json_writer;

static void fck_json_writer_push(struct fck_serialiser *s, const char *name)
{
	fck_json_writer *w = (fck_json_writer *)s;
	yyjson_mut_val *parent = w->stack[w->stack_top];
	yyjson_mut_val *child = yyjson_mut_obj(w->doc);

	yyjson_mut_obj_add_val(w->doc, parent, name, child);
	w->stack[++w->stack_top] = child;
}

static void fck_json_writer_pop(struct fck_serialiser *s)
{
	fck_json_writer *w = (fck_json_writer *)s;
	if (w->stack_top > 0)
	{
		w->stack_top--;
	}
}

#define DEFINE_FCK_JSON_WRITER_FUNC(TYPE_NAME, T, YY_MUT_CREATOR)                                                                          \
	static void fck_json_writer_##TYPE_NAME(struct fck_serialiser *s, const char *name, T *v, fckc_size_t c)                \
	{                                                                                                                                      \
		fck_json_writer *w = (fck_json_writer *)s;                                                                                         \
		yyjson_mut_val *parent = w->stack[w->stack_top];                                                                                   \
		if (c == 1)                                                                                                                        \
		{                                                                                                                                  \
			yyjson_mut_obj_add_val(w->doc, parent, name, YY_MUT_CREATOR(w->doc, v[0]));                                                 \
		}                                                                                                                                  \
		else                                                                                                                               \
		{                                                                                                                                  \
			yyjson_mut_val *arr = yyjson_mut_arr(w->doc);                                                                                  \
			for (fckc_size_t i = 0; i < c; i++)                                                                                            \
			{                                                                                                                              \
				yyjson_mut_arr_add_val(arr, YY_MUT_CREATOR(w->doc, v[i]));                                                                 \
			}                                                                                                                              \
			yyjson_mut_obj_add_val(w->doc, parent, name, arr);                                                                          \
		}                                                                                                                                  \
	}

DEFINE_FCK_JSON_WRITER_FUNC(i8, fckc_i8, yyjson_mut_sint)
DEFINE_FCK_JSON_WRITER_FUNC(i16, fckc_i16, yyjson_mut_sint)
DEFINE_FCK_JSON_WRITER_FUNC(i32, fckc_i32, yyjson_mut_sint)
DEFINE_FCK_JSON_WRITER_FUNC(i64, fckc_i64, yyjson_mut_sint)
DEFINE_FCK_JSON_WRITER_FUNC(u8, fckc_u8, yyjson_mut_uint)
DEFINE_FCK_JSON_WRITER_FUNC(u16, fckc_u16, yyjson_mut_uint)
DEFINE_FCK_JSON_WRITER_FUNC(u32, fckc_u32, yyjson_mut_uint)
DEFINE_FCK_JSON_WRITER_FUNC(u64, fckc_u64, yyjson_mut_uint)
DEFINE_FCK_JSON_WRITER_FUNC(f32, fckc_f32, yyjson_mut_real)
DEFINE_FCK_JSON_WRITER_FUNC(f64, fckc_f64, yyjson_mut_real)

static void fck_json_writer_string(struct fck_serialiser *s, const char *name, void **v, fckc_size_t c)
{
	fck_json_writer *w = (fck_json_writer *)s;
	yyjson_mut_val *parent = w->stack[w->stack_top];

	if (c == 1)
	{
		yyjson_mut_obj_add_str(w->doc, parent, name, (const char *)v[0]);
	}
	else
	{
		yyjson_mut_val *arr = yyjson_mut_arr(w->doc);
		for (fckc_size_t i = 0; i < c; i++)
		{
			yyjson_mut_arr_add_str(w->doc, arr, (const char *)v[i]);
		}
		yyjson_mut_obj_add_val(w->doc, parent, name, arr);
	}
}

static void *fck_json_writer_buffer(struct fck_serialiser *s)
{
	fck_json_writer *w = (fck_json_writer *)s;
	if (w->final_buffer)
	{
		free(w->final_buffer);
	}
	w->final_buffer = (fckc_char *)yyjson_mut_write(w->doc, YYJSON_WRITE_PRETTY, (size_t *)&w->final_len);
	return w->final_buffer;
}

static fckc_size_t fck_json_writer_at(struct fck_serialiser *s)
{
	fck_json_writer *w = (fck_json_writer *)s;
	if (!w->final_buffer)
	{
		fck_json_writer_buffer(s);
	}
	return to_size_t(w->final_len);
}

static void fck_json_writer_destroy(struct fck_serialiser *s)
{
	fck_json_writer *w = (fck_json_writer *)s;
	if (w->final_buffer)
	{
		free(w->final_buffer);
	}
	yyjson_mut_doc_free(w->doc);
	kll_free(w->allocator, w);
}

static fck_serialiser *fck_json_writer_create(kll_allocator *allocator)
{
	fck_json_writer *w = (fck_json_writer *)kll_malloc(allocator, sizeof(fck_json_writer));
	if (!w)
	{
		return NULL;
	}

	w->allocator = allocator;
	w->doc = yyjson_mut_doc_new(NULL);
	yyjson_mut_val *root = yyjson_mut_obj(w->doc);
	yyjson_mut_doc_set_root(w->doc, root);

	w->stack[0] = root;
	w->stack_top = 0;
	w->final_buffer = NULL;
	w->final_len = to_size_t(0);

	w->base.buffer = fck_json_writer_buffer;
	w->base.at = fck_json_writer_at;
	w->base.destroy = fck_json_writer_destroy;
	w->base.push = fck_json_writer_push;
	w->base.pop = fck_json_writer_pop;
	w->base.i8 = fck_json_writer_i8;
	w->base.i16 = fck_json_writer_i16;
	w->base.i32 = fck_json_writer_i32;
	w->base.i64 = fck_json_writer_i64;
	w->base.u8 = fck_json_writer_u8;
	w->base.u16 = fck_json_writer_u16;
	w->base.u32 = fck_json_writer_u32;
	w->base.u64 = fck_json_writer_u64;
	w->base.f32 = fck_json_writer_f32;
	w->base.f64 = fck_json_writer_f64;
	w->base.string = fck_json_writer_string;
	w->base.iterator = NULL;
	w->base.query = NULL;

	return (fck_serialiser *)w;
}

/* --- Reader Structure and Structural Flattening System --- */

typedef struct fck_json_reader_node
{
	fck_serialiser_primitive type;
	const char *name;
	fck_serialiser_value *values;
	fckc_size_t count;
} fck_json_reader_node;

typedef struct fck_json_reader
{
	fck_serialiser base;
	kll_allocator *arena;
	yyjson_doc *doc;
	yyjson_val *stack[64];
	int stack_top;

	fck_json_reader_node *nodes;
	fckc_size_t node_count;
	fckc_size_t node_capacity;

	fck_serialiser_element query_element;
} fck_json_reader;

typedef struct fck_json_iterator_internal
{
	fck_serialiser_iterator base;
	fck_json_reader *reader;
	size_t current_index;
} fck_json_iterator_internal;

static fckc_char *fck_json_strdup(kll_allocator *allocator, const fckc_char *src)
{
	if (!src)
	{
		return NULL;
	}
	fckc_size_t len = to_size_t(strlen(src));
	fckc_char *dst = (fckc_char *)kll_malloc(allocator, len + 1);
	if (dst)
	{
		memcpy(dst, src, len + 1);
	}
	return dst;
}

static void fck_json_reader_grow(fck_json_reader *r)
{
	if (r->node_count >= r->node_capacity)
	{
		r->node_capacity = (r->node_capacity == 0) ? to_size_t(64) : r->node_capacity * to_size_t(2);
		r->nodes = (fck_json_reader_node *)kll_realloc(r->arena, r->nodes, sizeof(fck_json_reader_node) * to_size_t(r->node_capacity));
	}
}

static void fck_json_reader_flatten(fck_json_reader *r, yyjson_val *val, const char *name)
{
	if (!val)
		return;

	fck_json_reader_grow(r);
	fck_json_reader_node *node = &r->nodes[to_size_t(r->node_count)];

	// Assign the name here. Since this comes from yyjson_get_str(key),
	// it points to the document memory, which is stable/persistent.
	node->name = name;

	if (yyjson_is_obj(val))
	{
		node->type = fck_serialiser_push;
		node->values = NULL;
		node->count = to_size_t(0);
		r->node_count++;

		size_t idx, max;
		yyjson_val *key, *child;
		yyjson_obj_foreach(val, idx, max, key, child)
		{
			// Pass the key string directly to the recursion
			fck_json_reader_flatten(r, child, yyjson_get_str(key));
		}

		fck_json_reader_grow(r);
		fck_json_reader_node *pop_node = &r->nodes[to_size_t(r->node_count)];
		pop_node->name = NULL; // Optional: No name for pop structural node
		pop_node->type = fck_serialiser_pop;
		pop_node->values = NULL;
		pop_node->count = to_size_t(0);
		r->node_count++;
		return;
	}

	// Logic for Arrays and Primitives
	fckc_size_t length = to_size_t(1);
	yyjson_val *first_item = val;
	int is_arr = yyjson_is_arr(val);

	if (is_arr)
	{
		length = to_size_t(yyjson_arr_size(val));
		if (length == 0)
			return;
		first_item = yyjson_arr_get(val, 0);
	}

	node->count = length;
	node->values = (fck_serialiser_value *)kll_malloc(r->arena, sizeof(void *) * to_size_t(length));

	// Handle types
	if (yyjson_is_sint(first_item))
	{
		node->type = fck_serialiser_i64;
		for (fckc_size_t i = 0; i < length; i++)
		{
			yyjson_val *item = is_arr ? yyjson_arr_get(val, to_size_t(i)) : val;
			int64_t *ptr = (int64_t *)kll_malloc(r->arena, sizeof(int64_t));
			*ptr = yyjson_get_sint(item);
			node->values[to_size_t(i)].as_i64 = *ptr;
		}
	}
	else if (yyjson_is_uint(first_item))
	{
		node->type = fck_serialiser_u64;
		for (fckc_size_t i = 0; i < length; i++)
		{
			yyjson_val *item = is_arr ? yyjson_arr_get(val, to_size_t(i)) : val;
			uint64_t *ptr = (uint64_t *)kll_malloc(r->arena, sizeof(uint64_t));
			*ptr = yyjson_get_uint(item);
			node->values[to_size_t(i)].as_u64 = *ptr;
		}
	}
	else if (yyjson_is_real(first_item))
	{
		node->type = fck_serialiser_f64;
		for (fckc_size_t i = 0; i < length; i++)
		{
			yyjson_val *item = is_arr ? yyjson_arr_get(val, to_size_t(i)) : val;
			double *ptr = (double *)kll_malloc(r->arena, sizeof(double));
			*ptr = yyjson_get_real(item);
			node->values[to_size_t(i)].as_f64 = *ptr;
		}
	}
	else if (yyjson_is_str(first_item))
	{
		node->type = fck_serialiser_string;
		for (fckc_size_t i = 0; i < length; i++)
		{
			yyjson_val *item = is_arr ? yyjson_arr_get(val, to_size_t(i)) : val;
			const fckc_char *src = (const fckc_char *)yyjson_get_str(item);
			node->values[to_size_t(i)].as_string = fck_json_strdup(r->arena, src);
		}
	}
	else
	{
		node->type = fck_serialiser_i8;
		for (fckc_size_t i = 0; i < length; i++)
		{
			node->values[to_size_t(i)].as_i8 = 0;
		}
	}

	r->node_count++;
}

static fck_serialiser_element *fck_json_reader_query(struct fck_serialiser *s, const char *path)
{
	fck_json_reader *r = (fck_json_reader *)s;
	if (!path || !r->doc)
	{
		return NULL;
	}

	yyjson_val *target = yyjson_doc_ptr_get(r->doc, path);
	if (!target)
	{
		return NULL;
	}

	const int is_arr = yyjson_is_arr(target);
	const fckc_size_t count = is_arr ? to_size_t(yyjson_arr_size(target)) : to_size_t(1);

	if (count == 0)
	{
		return NULL; // Cannot deduce type for an empty array
	}
	yyjson_val *first_item = is_arr ? yyjson_arr_get(target, 0) : target;

	// Reset values for this query
	r->query_element.count = count;
	r->query_element.values = (fck_serialiser_value *)kll_malloc(r->arena, sizeof(void *) * count);
	r->query_element.name = path;

	if (yyjson_is_str(first_item))
	{
		r->query_element.type = fck_serialiser_string;
		for (fckc_size_t i = 0; i < count; i++)
		{
			yyjson_val *item = is_arr ? yyjson_arr_get(target, (size_t)i) : target;
			const char *str = yyjson_get_str(item);
			r->query_element.values[i].as_string = fck_json_strdup(r->arena, (const fckc_char *)str);
		}
	}
	else if (yyjson_is_sint(first_item))
	{
		r->query_element.type = fck_serialiser_i64;
		for (fckc_size_t i = 0; i < count; i++)
		{
			yyjson_val *item = is_arr ? yyjson_arr_get(target, (size_t)i) : target;
			int64_t *val = (int64_t *)kll_malloc(r->arena, sizeof(int64_t));
			*val = yyjson_get_sint(item);
			r->query_element.values[i].as_i64 = *val;
		}
	}
	else if (yyjson_is_uint(first_item))
	{
		r->query_element.type = fck_serialiser_u64;
		for (fckc_size_t i = 0; i < count; i++)
		{
			yyjson_val *item = is_arr ? yyjson_arr_get(target, (size_t)i) : target;
			uint64_t *val = (uint64_t *)kll_malloc(r->arena, sizeof(uint64_t));
			*val = yyjson_get_uint(item);
			r->query_element.values[i].as_u64 = *val;
		}
	}
	else if (yyjson_is_real(first_item))
	{
		r->query_element.type = fck_serialiser_f64;
		for (fckc_size_t i = 0; i < count; i++)
		{
			yyjson_val *item = is_arr ? yyjson_arr_get(target, (size_t)i) : target;
			double *val = (double *)kll_malloc(r->arena, sizeof(double));
			*val = yyjson_get_real(item);
			r->query_element.values[i].as_f64 = *val;
		}
	}
	else
	{
		// Target is an Object, Null, or unsupported type
		kll_free(r->arena, r->query_element.values);
		r->query_element.values = NULL;
		return NULL;
	}

	return &r->query_element;
}

static fck_serialiser_element *fck_json_iterator_next(fck_serialiser_iterator *it, fck_serialiser_element *element)
{
	fck_json_iterator_internal *i = (fck_json_iterator_internal *)it;

	if (i->current_index >= i->reader->node_count)
	{
		return NULL;
	}

	fck_json_reader_node *node = &i->reader->nodes[i->current_index];
	element->type = node->type;
	element->name = node->name;
	element->values = node->values;
	element->count = node->count;

	// Advance index
	i->current_index++;

	return element;
}

static void fck_json_iterator_destroy(fck_serialiser_iterator *it)
{
	fck_json_iterator_internal *i = (fck_json_iterator_internal *)it;
	kll_free(i->reader->arena, i);
}

static fck_serialiser_iterator *fck_json_iterator_create(fck_serialiser *s)
{
	fck_json_reader *r = (fck_json_reader *)s;
	fck_json_iterator_internal *it = (fck_json_iterator_internal *)kll_malloc(r->arena, sizeof(fck_json_iterator_internal));

	it->base.next = fck_json_iterator_next;
	it->base.destroy = fck_json_iterator_destroy;
	it->reader = r;
	it->current_index = 0;

	return (fck_serialiser_iterator *)it;
}

/* --- Classical Reader Implementation Functions --- */

static void fck_json_reader_push(struct fck_serialiser *s, const char *name)
{
	fck_json_reader *r = (fck_json_reader *)s;
	yyjson_val *parent = r->stack[r->stack_top];
	yyjson_val *child = yyjson_obj_get(parent, name);

	r->stack[++r->stack_top] = child;
}

static void fck_json_reader_pop(struct fck_serialiser *s)
{
	fck_json_reader *r = (fck_json_reader *)s;
	if (r->stack_top > 0)
	{
		r->stack_top--;
	}
}

#define DEFINE_FCK_JSON_READER_FUNC(TYPE_NAME, T, CAST_MACRO, YY_GETTER)                                                                   \
	static void fck_json_reader_##TYPE_NAME(struct fck_serialiser *s, const char *name, T *v, fckc_size_t c)                \
	{                                                                                                                                      \
		fck_json_reader *r = (fck_json_reader *)s;                                                                                         \
		yyjson_val *parent = r->stack[r->stack_top];                                                                                       \
		yyjson_val *val = yyjson_obj_get(parent, name);                                                                                 \
		if (!val)                                                                                                                          \
		{                                                                                                                                  \
			memset(v, 0, sizeof(T) * to_size_t(c));                                                                                        \
			return;                                                                                                                        \
		}                                                                                                                                  \
		if (c == 1)                                                                                                                        \
		{                                                                                                                                  \
			if (yyjson_is_arr(val))                                                                                                        \
			{                                                                                                                              \
				v[0] = CAST_MACRO(YY_GETTER(yyjson_arr_get(val, 0)));                                                                      \
			}                                                                                                                              \
			else                                                                                                                           \
			{                                                                                                                              \
				v[0] = CAST_MACRO(YY_GETTER(val));                                                                                         \
			}                                                                                                                              \
		}                                                                                                                                  \
		else                                                                                                                               \
		{                                                                                                                                  \
			if (yyjson_is_arr(val))                                                                                                        \
			{                                                                                                                              \
				fckc_size_t arr_len = to_size_t(yyjson_arr_size(val));                                                                     \
				for (fckc_size_t i = 0; i < c; i++)                                                                                        \
				{                                                                                                                          \
					v[i] = (i < arr_len) ? CAST_MACRO(YY_GETTER(yyjson_arr_get(val, i))) : CAST_MACRO(0);                                  \
				}                                                                                                                          \
			}                                                                                                                              \
			else                                                                                                                           \
			{                                                                                                                              \
				v[0] = CAST_MACRO(YY_GETTER(val));                                                                                         \
				memset(&v[1], 0, sizeof(T) * to_size_t(c - 1));                                                                            \
			}                                                                                                                              \
		}                                                                                                                                  \
	}

DEFINE_FCK_JSON_READER_FUNC(i8, fckc_i8, to_i8, yyjson_get_sint)
DEFINE_FCK_JSON_READER_FUNC(i16, fckc_i16, to_i16, yyjson_get_sint)
DEFINE_FCK_JSON_READER_FUNC(i32, fckc_i32, to_i32, yyjson_get_sint)
DEFINE_FCK_JSON_READER_FUNC(i64, fckc_i64, to_i64, yyjson_get_sint)
DEFINE_FCK_JSON_READER_FUNC(u8, fckc_u8, to_u8, yyjson_get_uint)
DEFINE_FCK_JSON_READER_FUNC(u16, fckc_u16, to_u16, yyjson_get_uint)
DEFINE_FCK_JSON_READER_FUNC(u32, fckc_u32, to_u32, yyjson_get_uint)
DEFINE_FCK_JSON_READER_FUNC(u64, fckc_u64, to_u64, yyjson_get_uint)
DEFINE_FCK_JSON_READER_FUNC(f32, fckc_f32, to_f32, yyjson_get_real)
DEFINE_FCK_JSON_READER_FUNC(f64, fckc_f64, to_f64, yyjson_get_real)

static void fck_json_reader_string(struct fck_serialiser *s, const char *name, void **v, fckc_size_t c)
{
	fck_json_reader *r = (fck_json_reader *)s;
	yyjson_val *parent = r->stack[r->stack_top];
	yyjson_val *val = yyjson_obj_get(parent, name);

	if (!val)
	{
		for (fckc_size_t i = 0; i < c; i++)
		{
			v[i] = NULL;
		}
		return;
	}

	if (c == 1)
	{
		if (yyjson_is_arr(val))
		{
			const fckc_char *str = (const fckc_char *)yyjson_get_str(yyjson_arr_get(val, 0));
			v[0] = fck_json_strdup(r->arena, str);
		}
		else
		{
			v[0] = fck_json_strdup(r->arena, (const fckc_char *)yyjson_get_str(val));
		}
	}
	else
	{
		if (yyjson_is_arr(val))
		{
			const fckc_size_t arr_len = to_size_t(yyjson_arr_size(val));
			for (fckc_size_t i = 0; i < c; i++)
			{
				if (i < arr_len)
				{
					const fckc_char *str = (const fckc_char *)yyjson_get_str(yyjson_arr_get(val, i));
					v[i] = fck_json_strdup(r->arena, str);
				}
				else
				{
					v[i] = NULL;
				}
			}
		}
		else
		{
			v[0] = fck_json_strdup(r->arena, (const fckc_char *)yyjson_get_str(val));
			for (fckc_size_t i = 1; i < c; i++)
			{
				v[i] = NULL;
			}
		}
	}
}

static void *fck_json_reader_buffer(struct fck_serialiser *s)
{
	return NULL;
}

static fckc_size_t fck_json_reader_at(struct fck_serialiser *s)
{
	return to_size_t(0);
}

static void fck_json_reader_destroy(struct fck_serialiser *s)
{
	fck_json_reader *r = (fck_json_reader *)s;

	for (fckc_size_t i = 0; i < r->node_count; i++)
	{
		if (r->nodes[to_size_t(i)].values)
		{
			for (fckc_size_t j = 0; j < r->nodes[to_size_t(i)].count; j++)
			{
				if (r->nodes[to_size_t(i)].type == fck_serialiser_string)
				{
					if (r->nodes[to_size_t(i)].values[to_size_t(j)].as_string)
					{
						kll_free(r->arena, r->nodes[to_size_t(i)].values[to_size_t(j)].as_string);
					}
				}
			}
			kll_free(r->arena, r->nodes[to_size_t(i)].values);
		}
	}

	if (r->nodes)
	{
		kll_free(r->arena, r->nodes);
	}

	yyjson_doc_free(r->doc);
	kll_free(r->arena, r);
}

static fck_serialiser *fck_json_reader_create(kll_allocator *allocator, const fckc_char *source, fckc_size_t length)
{
	fck_json_reader *r = (fck_json_reader *)kll_malloc(allocator, sizeof(fck_json_reader));
	if (!r)
	{
		return NULL;
	}

	r->arena = allocator;
	r->doc = yyjson_read((const char *)source, to_size_t(length), 0);
	r->stack[0] = r->doc ? yyjson_doc_get_root(r->doc) : NULL;
	r->stack_top = 0;

	r->nodes = NULL;
	r->node_count = to_size_t(0);
	r->node_capacity = to_size_t(0);

	r->base.buffer = fck_json_reader_buffer;
	r->base.at = fck_json_reader_at;
	r->base.destroy = fck_json_reader_destroy;
	r->base.push = fck_json_reader_push;
	r->base.pop = fck_json_reader_pop;
	r->base.i8 = fck_json_reader_i8;
	r->base.i16 = fck_json_reader_i16;
	r->base.i32 = fck_json_reader_i32;
	r->base.i64 = fck_json_reader_i64;
	r->base.u8 = fck_json_reader_u8;
	r->base.u16 = fck_json_reader_u16;
	r->base.u32 = fck_json_reader_u32;
	r->base.u64 = fck_json_reader_u64;
	r->base.f32 = fck_json_reader_f32;
	r->base.f64 = fck_json_reader_f64;
	r->base.string = fck_json_reader_string;
	r->base.query = fck_json_reader_query;
	r->base.iterator = fck_json_iterator_create;

	if (r->stack[0])
	{
		fck_json_reader_flatten(r, r->stack[0], "");
	}

	return (fck_serialiser *)r;
}

static fck_serialiser_json_api serialiser_json_api = {
	.writer = fck_json_writer_create,
	.reader = fck_json_reader_create,
};

fck_serialiser_json_api *serialiser_json = &serialiser_json_api;