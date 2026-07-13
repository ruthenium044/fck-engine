
#include "fck_serialiser_text.h"
#include "fck_serialiser.h"
#include <fckc_inttypes.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <kll.h>
#include <kll_malloc.h>

typedef struct fck_buffer_writer
{
	fck_serialiser base;

	kll_allocator *allocator;
	char *buffer;
	fckc_size_t capacity;
	fckc_size_t offset;
	int indent_level;
} fck_buffer_writer;

static void fck_buffer_writer_printf(fck_buffer_writer *w, const fckc_char *format, ...)
{
	if (w->offset >= w->capacity)
		return;

	va_list args;
	va_start(args, format);
	const int written = vsnprintf(w->buffer + w->offset, w->capacity - w->offset, format, args);
	va_end(args);

	if (written > 0)
	{
		w->offset += written;
		if (w->offset > w->capacity)
		{
			w->offset = w->capacity; // Clamp if truncated
		}
	}
}

static void fck_buffer_writer_push(struct fck_serialiser *s, struct fck_serialiser_params *p)
{
	fck_buffer_writer *w = (fck_buffer_writer *)s;
	fck_buffer_writer_printf(w, "%*s%s {\n", w->indent_level * 2, "", p->name);
	w->indent_level++;
}

static void fck_buffer_writer_pop(struct fck_serialiser *s)
{
	fck_buffer_writer *w = (fck_buffer_writer *)s;
	w->indent_level--;
	fck_buffer_writer_printf(w, "%*s}\n", w->indent_level * 2, "");
}

static void fck_buffer_writer_string(struct fck_serialiser *s, struct fck_serialiser_params *p, void **v, fckc_size_t c)
{
	fck_buffer_writer *w = (fck_buffer_writer *)s;
	fck_buffer_writer_printf(w, "%*s%s :", w->indent_level * 2, "", p->name);

	for (fckc_size_t i = 0; i < c; i++)
	{
		if (v[i])
		{
			fck_buffer_writer_printf(w, " \"%s\"", v[i]);
		}
	}
	fck_buffer_writer_printf(w, "\n");
}

#define DEFINE_FCK_BUFFER_WRITER_FUNC(TYPE_NAME, T, FORMAT)                                                                                \
	static void fck_buffer_writer_##TYPE_NAME(struct fck_serialiser *s, struct fck_serialiser_params *p, T *v, fckc_size_t c)              \
	{                                                                                                                                      \
		fck_buffer_writer *w = (fck_buffer_writer *)s;                                                                                     \
		fck_buffer_writer_printf(w, "%*s%s :", w->indent_level * 2, "", p->name);                                                          \
		for (fckc_size_t i = 0; i < c; i++)                                                                                                \
		{                                                                                                                                  \
			fck_buffer_writer_printf(w, " " FORMAT, v[i]);                                                                                 \
		}                                                                                                                                  \
		fck_buffer_writer_printf(w, "\n");                                                                                                 \
	}

DEFINE_FCK_BUFFER_WRITER_FUNC(i8, fckc_i8, "%hhd")
DEFINE_FCK_BUFFER_WRITER_FUNC(i16, fckc_i16, "%hd")
DEFINE_FCK_BUFFER_WRITER_FUNC(i32, fckc_i32, "%d")
DEFINE_FCK_BUFFER_WRITER_FUNC(i64, fckc_i64, "%lld")
DEFINE_FCK_BUFFER_WRITER_FUNC(u8, fckc_u8, "%hhu")
DEFINE_FCK_BUFFER_WRITER_FUNC(u16, fckc_u16, "%hu")
DEFINE_FCK_BUFFER_WRITER_FUNC(u32, fckc_u32, "%u")
DEFINE_FCK_BUFFER_WRITER_FUNC(u64, fckc_u64, "%llu")
DEFINE_FCK_BUFFER_WRITER_FUNC(f32, fckc_f32, "%f")
DEFINE_FCK_BUFFER_WRITER_FUNC(f64, fckc_f64, "%lf")

static void fck_buffer_writer_destroy(fck_serialiser *s)
{
	fck_buffer_writer *writer = (fck_buffer_writer *)s;
	kll_free(writer->allocator, writer);
}

static void *fck_buffer_writer_buffer(struct fck_serialiser *s)
{
	fck_buffer_writer *w = (fck_buffer_writer *)s;
	return (void *)w->buffer;
}
static fckc_size_t fck_buffer_writer_at(struct fck_serialiser *s)
{
	fck_buffer_writer *w = (fck_buffer_writer *)s;
	return w->offset;
}

static fck_serialiser *fck_buffer_writer_create(kll_allocator *allocator, fckc_size_t buffer_capacity)
{
	fck_buffer_writer *writer = (fck_buffer_writer *)kll_malloc(allocator, sizeof(*writer) + buffer_capacity);
	memset(writer, 0, sizeof(*writer));
	writer->buffer = (char *)fckc_pointer_add(writer, sizeof(*writer));
	writer->allocator = allocator;
	writer->capacity = buffer_capacity;
	writer->offset = 0;
	writer->indent_level = 0;
	writer->base.at = fck_buffer_writer_at;
	writer->base.buffer = fck_buffer_writer_buffer;
	writer->base.destroy = fck_buffer_writer_destroy;
	writer->base.push = fck_buffer_writer_push;
	writer->base.pop = fck_buffer_writer_pop;
	writer->base.i8 = fck_buffer_writer_i8;
	writer->base.i16 = fck_buffer_writer_i16;
	writer->base.i32 = fck_buffer_writer_i32;
	writer->base.i64 = fck_buffer_writer_i64;
	writer->base.u8 = fck_buffer_writer_u8;
	writer->base.u16 = fck_buffer_writer_u16;
	writer->base.u32 = fck_buffer_writer_u32;
	writer->base.u64 = fck_buffer_writer_u64;
	writer->base.f32 = fck_buffer_writer_f32;
	writer->base.f64 = fck_buffer_writer_f64;
	writer->base.string = fck_buffer_writer_string;

	return &writer->base;
}

typedef struct fck_buffer_reader
{
	fck_serialiser base;
	kll_allocator *allocator;
	kll_arena *arena;
	const fckc_char *buffer;
	const fckc_char *cursor;
} fck_buffer_reader;

static void fck_buffer_reader_match_token(fck_buffer_reader *r, const fckc_char *expected)
{
	char buf[256];
	int bytes_consumed = 0;

	// %n populates bytes_consumed with the total characters read up to that point
	if (sscanf(r->cursor, "%255s%n", buf, &bytes_consumed) >= 1)
	{
		r->cursor += bytes_consumed;
		if (strcmp(buf, expected) != 0)
		{
			fprintf(stderr, "Parsing Error: Expected token '%s', got '%s'\n", expected, buf);
		}
	}
}

static void fck_buffer_reader_push(struct fck_serialiser *s, struct fck_serialiser_params *p)
{
	fck_buffer_reader *r = (fck_buffer_reader *)s;
	fck_buffer_reader_match_token(r, p->name);
	fck_buffer_reader_match_token(r, "{");
}

static void fck_buffer_reader_pop(struct fck_serialiser *s)
{
	fck_buffer_reader *r = (fck_buffer_reader *)s;
	fck_buffer_reader_match_token(r, "}");
}

static void fck_buffer_reader_string(struct fck_serialiser *s, struct fck_serialiser_params *p, void **v, fckc_size_t c)
{
	fck_buffer_reader *r = (fck_buffer_reader *)s;
	fck_buffer_reader_match_token(r, p->name);
	fck_buffer_reader_match_token(r, ":");

	for (fckc_size_t i = 0; i < c; i++)
	{
		r->cursor += strspn(r->cursor, " \t\n\r");
		if (r->cursor[0] == '"')
		{
			r->cursor++;
			const fckc_char *closing_quote = strchr(r->cursor, '"');
			if (closing_quote)
			{
				const fckc_size_t len = closing_quote - r->cursor;
				char *str = (char *)kll_malloc((kll_allocator *)r->arena, len + 1);
				if (str)
				{
					memcpy(str, r->cursor, len);
					str[len] = '\0';
					v[i] = str;
				}
				else
				{
					v[i] = NULL;
				}

				r->cursor = closing_quote + 1;
			}
			else
			{
				v[i] = NULL;
			}
		}
		else
		{
			const fckc_size_t len = strcspn(r->cursor, " \t\n\r");
			if (len > 0)
			{
				char *str = (char *)kll_malloc(r->arena, len + 1);
				if (str)
				{
					memcpy(str, r->cursor, len);
					str[len] = '\0';
					v[i] = str;
				}
				r->cursor += len;
			}
			else
			{
				v[i] = NULL;
			}
		}
	}
}

#define DEFINE_FCK_BUFFER_READER_FUNC(TYPE_NAME, T, FORMAT)                                                                                \
	static void fck_buffer_reader_##TYPE_NAME(struct fck_serialiser *s, struct fck_serialiser_params *p, T *v, fckc_size_t c)              \
	{                                                                                                                                      \
		fck_buffer_reader *r = (fck_buffer_reader *)s;                                                                                     \
		fck_buffer_reader_match_token(r, p->name);                                                                                         \
		fck_buffer_reader_match_token(r, ":");                                                                                             \
		for (fckc_size_t i = 0; i < c; i++)                                                                                                \
		{                                                                                                                                  \
			int bytes_consumed = 0;                                                                                                        \
			if (sscanf(r->cursor, FORMAT "%n", &v[i], &bytes_consumed) >= 1)                                                               \
			{                                                                                                                              \
				r->cursor += bytes_consumed;                                                                                               \
			}                                                                                                                              \
		}                                                                                                                                  \
	}

DEFINE_FCK_BUFFER_READER_FUNC(i8, fckc_i8, "%hhd")
DEFINE_FCK_BUFFER_READER_FUNC(i16, fckc_i16, "%hd")
DEFINE_FCK_BUFFER_READER_FUNC(i32, fckc_i32, "%d")
DEFINE_FCK_BUFFER_READER_FUNC(i64, fckc_i64, "%lld")
DEFINE_FCK_BUFFER_READER_FUNC(u8, fckc_u8, "%hhu")
DEFINE_FCK_BUFFER_READER_FUNC(u16, fckc_u16, "%hu")
DEFINE_FCK_BUFFER_READER_FUNC(u32, fckc_u32, "%u")
DEFINE_FCK_BUFFER_READER_FUNC(u64, fckc_u64, "%llu")
DEFINE_FCK_BUFFER_READER_FUNC(f32, fckc_f32, "%f")
DEFINE_FCK_BUFFER_READER_FUNC(f64, fckc_f64, "%lf")

static void *fck_buffer_reader_buffer(struct fck_serialiser *s)
{
	fck_buffer_reader *r = (fck_buffer_reader *)s;
	return (void *)r->buffer;
}
static fckc_size_t fck_buffer_reader_at(struct fck_serialiser *s)
{
	fck_buffer_reader *r = (fck_buffer_reader *)s;
	return (fckc_size_t)(r->cursor - r->buffer);
}

static void fck_buffer_reader_destroy(fck_serialiser *s)
{
	fck_buffer_reader *reader = (fck_buffer_reader *)s;
	kll->arena->destroy(reader->arena);
	kll_free(reader->allocator, reader);
}

static fck_serialiser *fck_buffer_reader_create(kll_allocator *allocator, const fckc_char *source_buffer)
{
	fck_buffer_reader *reader = (fck_buffer_reader *)kll_malloc(allocator, sizeof(*reader));
	memset(reader, 0, sizeof(*reader));
	reader->allocator = allocator;
	reader->arena = kll->arena->create(allocator, 64);
	reader->cursor = source_buffer;
	reader->buffer = source_buffer;
	reader->base.destroy = fck_buffer_reader_destroy;
	reader->base.push = fck_buffer_reader_push;
	reader->base.pop = fck_buffer_reader_pop;
	reader->base.i8 = fck_buffer_reader_i8;
	reader->base.i16 = fck_buffer_reader_i16;
	reader->base.i32 = fck_buffer_reader_i32;
	reader->base.i64 = fck_buffer_reader_i64;
	reader->base.u8 = fck_buffer_reader_u8;
	reader->base.u16 = fck_buffer_reader_u16;
	reader->base.u32 = fck_buffer_reader_u32;
	reader->base.u64 = fck_buffer_reader_u64;
	reader->base.f32 = fck_buffer_reader_f32;
	reader->base.f64 = fck_buffer_reader_f64;
	reader->base.string = fck_buffer_reader_string;
	return &reader->base;
}

static fck_serialiser_text_api serialiser_text_api = {
	.writer = fck_buffer_writer_create,
	.reader = fck_buffer_reader_create,
};

fck_serialiser_text_api *serialiser_text = &serialiser_text_api;