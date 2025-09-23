#include "fck_serialiser_nk_edit_vt.h"

#include "fck_serialiser_vt.h"
#include <SDL3/SDL_log.h>

#include "fck_ui.h"

#include <SDL3/SDL_assert.h>

#include <limits.h>

static const fckc_size_t FCK_WRITE_BUFFER_STRING_SIZE = 256;
static const fckc_size_t FCK_READ_BUFFER_STRING_SIZE = 256;

#define fck_nk_scope_str_concat(lhs, rhs) lhs##rhs
#define fck_nk_scope_unique(lhs, rhs) fck_nk_scope_str_concat(lhs, rhs)
#define fck_nk_scope(ctor, dtor)                                                                                                           \
	for (int fck_nk_scope_unique(i, __LINE__) = ((ctor) ? 0 : 1); fck_nk_scope_unique(i, __LINE__) == 0;                                   \
	     fck_nk_scope_unique(i, __LINE__) += 1, (dtor))

static void fck_nk_edit_precondition(fck_serialiser *s)
{
	SDL_assert(s->vt == fck_nk_edit_vt);
}

static float fck_nk_edit_value_label(const char *type, fck_ui_ctx *ctx, const char *name, fckc_size_t c)
{
	fckc_size_t col_count = (name != NULL ? 1 : 0) + c;

	const float label_ratio = 0.2f;

	// Value
	nk_layout_row_begin(ctx, NK_DYNAMIC, 25, (int)col_count);
	nk_layout_row_push(ctx, label_ratio);
	nk_labelf(ctx, NK_TEXT_LEFT, "%s : %s", name, type);
	return 1.0f - label_ratio;
}

static float fck_nk_edit_array_label(fck_ui_ctx *ctx)
{
	nk_layout_row_begin(ctx, NK_DYNAMIC, 25, (int)1);
	return 1.0f;
}

static int fck_nk_edit_tree_push(fck_ui_ctx *ctx, float *out_content_ratio, const char *type, const char *name, fckc_size_t count)
{
	if (count == 0)
	{
		return 0;
	}
	if (count == 1)
	{
		*out_content_ratio = fck_nk_edit_value_label(type, ctx, name, count);
		// 1 is ALWAYS open...
		return 1;
	}

	char buffer[FCK_READ_BUFFER_STRING_SIZE];
	int result = SDL_snprintf(buffer, sizeof(buffer), "%s : %s[%llu]", name, type, (fckc_u64)count);
	if (nk_tree_push_hashed(ctx, NK_TREE_NODE, buffer, NK_MINIMIZED, buffer, result, __LINE__))
	{
		*out_content_ratio = fck_nk_edit_array_label(ctx);
		return 1;
	}
	return 0;
}

static void fck_nk_edit_tree_pop(fck_ui_ctx *ctx, fckc_size_t count)
{
	nk_layout_row_end(ctx);
	if (count > 1)
	{
		// Only need to pop if more than 1;
		nk_tree_pop(ctx);
	}
}

#define fck_nk_tree_scope(ctx, content_ratio, type_name, name, count)                                                                      \
	fck_nk_scope(fck_nk_edit_tree_push(ctx, content_ratio, type_name, name, count), fck_nk_edit_tree_pop(ctx, count))

static const char *fck_nk_property_prefix(fckc_size_t count)
{
	return count == 1 ? "##" : "#";
}

void fck_nk_edit_i8(fck_serialiser *s, fck_serialiser_params *p, fckc_i8 *v, fckc_size_t c)
{
	fck_nk_edit_precondition(s);

	const char type_name[] = "i8";
	const int min = -127;
	const int max = 127;

	fck_ui_ctx *ctx = (fck_ui_ctx *)((fck_nk_serialiser *)s)->ctx;

	float content_ratio = 0.0f;
	fck_nk_tree_scope(ctx, &content_ratio, type_name, p->name, c)
	{
		char buffer[FCK_READ_BUFFER_STRING_SIZE];
		for (fckc_size_t i = 0; i < c; i++)
		{
			int value = (int)v[i];
			int result = SDL_snprintf(buffer, sizeof(buffer), "%s[%u] : %s", fck_nk_property_prefix(c), (unsigned int)i, type_name);

			nk_layout_row_push(ctx, content_ratio);
			nk_property_int(ctx, buffer, min, &value, max, 1.0f, 1.0f);
			v[i] = (fckc_i8)value;
		}
	}
}

void fck_nk_edit_i16(fck_serialiser *s, fck_serialiser_params *p, fckc_i16 *v, fckc_size_t c)
{
	fck_nk_edit_precondition(s);

	const char type_name[] = "i16";
	const int min = -32767;
	const int max = 32767;

	fck_ui_ctx *ctx = (fck_ui_ctx *)((fck_nk_serialiser *)s)->ctx;

	float content_ratio = 0.0f;
	fck_nk_tree_scope(ctx, &content_ratio, type_name, p->name, c)
	{
		char buffer[FCK_READ_BUFFER_STRING_SIZE];
		for (fckc_size_t i = 0; i < c; i++)
		{
			int value = (int)v[i];
			int result = SDL_snprintf(buffer, sizeof(buffer), "%s[%u] : %s", fck_nk_property_prefix(c), (unsigned int)i, type_name);

			nk_layout_row_push(ctx, content_ratio);
			nk_property_int(ctx, buffer, min, &value, max, 1.0f, 1.0f);
			v[i] = (fckc_i16)value;
		}
	}
}

void fck_nk_edit_i32(fck_serialiser *s, fck_serialiser_params *p, fckc_i32 *v, fckc_size_t c)
{
	fck_nk_edit_precondition(s);

	const char type_name[] = "i32";
	const int min = INT_MIN;
	const int max = INT_MAX;

	fck_ui_ctx *ctx = (fck_ui_ctx *)((fck_nk_serialiser *)s)->ctx;

	float content_ratio = 0.0f;
	fck_nk_tree_scope(ctx, &content_ratio, type_name, p->name, c)
	{
		char buffer[FCK_READ_BUFFER_STRING_SIZE];
		for (fckc_size_t i = 0; i < c; i++)
		{
			int value = (int)v[i];
			int result = SDL_snprintf(buffer, sizeof(buffer), "%s[%u] : %s", fck_nk_property_prefix(c), (unsigned int)i, type_name);

			nk_layout_row_push(ctx, content_ratio);
			nk_property_int(ctx, buffer, min, &value, max, 1.0f, 1.0f);
			v[i] = (fckc_i32)value;
		}
	}
}

void fck_nk_edit_i64(fck_serialiser *s, fck_serialiser_params *p, fckc_i64 *v, fckc_size_t c)
{
	fck_nk_edit_precondition(s);

	const char type_name[] = "i64";
	const int min = INT_MIN;
	const int max = INT_MAX;

	fck_ui_ctx *ctx = (fck_ui_ctx *)((fck_nk_serialiser *)s)->ctx;

	float content_ratio = 0.0f;
	fck_nk_tree_scope(ctx, &content_ratio, type_name, p->name, c)
	{
		char buffer[FCK_READ_BUFFER_STRING_SIZE];
		for (fckc_size_t i = 0; i < c; i++)
		{
			int value = (int)v[i];
			int result = SDL_snprintf(buffer, sizeof(buffer), "%s[%u] : %s", fck_nk_property_prefix(c), (unsigned int)i, type_name);

			nk_layout_row_push(ctx, content_ratio);
			nk_property_int(ctx, buffer, min, &value, max, 1.0f, 1.0f);
			v[i] = (fckc_i64)value;
		}
	}
}

void fck_nk_edit_u8(fck_serialiser *s, fck_serialiser_params *p, fckc_u8 *v, fckc_size_t c)
{
	fck_nk_edit_precondition(s);

	const char type_name[] = "u8";
	const int min = 0;
	const int max = 0xFF;

	fck_ui_ctx *ctx = (fck_ui_ctx *)((fck_nk_serialiser *)s)->ctx;

	float content_ratio = 0.0f;
	fck_nk_tree_scope(ctx, &content_ratio, type_name, p->name, c)
	{
		char buffer[FCK_READ_BUFFER_STRING_SIZE];
		for (fckc_size_t i = 0; i < c; i++)
		{
			int value = (int)v[i];
			int result = SDL_snprintf(buffer, sizeof(buffer), "%s[%u] : %s", fck_nk_property_prefix(c), (unsigned int)i, type_name);

			nk_layout_row_push(ctx, content_ratio);
			nk_property_int(ctx, buffer, min, &value, max, 1.0f, 1.0f);
			v[i] = (fckc_u8)value;
		}
	}
}

void fck_nk_edit_u16(fck_serialiser *s, fck_serialiser_params *p, fckc_u16 *v, fckc_size_t c)
{
	fck_nk_edit_precondition(s);

	const char type_name[] = "u16";
	const int min = 0;
	const int max = 0xFFFF;

	fck_ui_ctx *ctx = (fck_ui_ctx *)((fck_nk_serialiser *)s)->ctx;

	float content_ratio = 0.0f;
	fck_nk_tree_scope(ctx, &content_ratio, type_name, p->name, c)
	{
		char buffer[FCK_READ_BUFFER_STRING_SIZE];
		for (fckc_size_t i = 0; i < c; i++)
		{
			int value = (int)v[i];
			int result = SDL_snprintf(buffer, sizeof(buffer), "%s[%u] : %s", fck_nk_property_prefix(c), (unsigned int)i, type_name);

			nk_layout_row_push(ctx, content_ratio);
			nk_property_int(ctx, buffer, min, &value, max, 1.0f, 1.0f);
			v[i] = (fckc_u16)value;
		}
	}
}

void fck_nk_edit_u32(fck_serialiser *s, fck_serialiser_params *p, fckc_u32 *v, fckc_size_t c)
{
	fck_nk_edit_precondition(s);

	const char type_name[] = "u32";
	const int min = 0;
	const int max = INT_MAX;

	fck_ui_ctx *ctx = (fck_ui_ctx *)((fck_nk_serialiser *)s)->ctx;

	float content_ratio = 0.0f;
	fck_nk_tree_scope(ctx, &content_ratio, type_name, p->name, c)
	{
		char buffer[FCK_READ_BUFFER_STRING_SIZE];
		for (fckc_size_t i = 0; i < c; i++)
		{
			int value = (int)v[i];
			int result = SDL_snprintf(buffer, sizeof(buffer), "%s[%u] : %s", fck_nk_property_prefix(c), (unsigned int)i, type_name);

			nk_layout_row_push(ctx, content_ratio);
			nk_property_int(ctx, buffer, min, &value, max, 1.0f, 1.0f);
			v[i] = (fckc_u32)value;
		}
	}
}

static void fck_nk_edit_u64(fck_serialiser *s, fck_serialiser_params *p, fckc_u64 *v, fckc_size_t c)
{
	fck_nk_edit_precondition(s);

	const char type_name[] = "u64";
	const int min = 0;
	const int max = INT_MAX;

	fck_ui_ctx *ctx = (fck_ui_ctx *)((fck_nk_serialiser *)s)->ctx;

	float content_ratio = 0.0f;
	fck_nk_tree_scope(ctx, &content_ratio, type_name, p->name, c)
	{
		char buffer[FCK_READ_BUFFER_STRING_SIZE];
		for (fckc_size_t i = 0; i < c; i++)
		{
			int value = (int)v[i];
			int result = SDL_snprintf(buffer, sizeof(buffer), "%s[%u] : %s", fck_nk_property_prefix(c), (unsigned int)i, type_name);

			nk_layout_row_push(ctx, content_ratio);
			nk_property_int(ctx, buffer, min, &value, max, 1.0f, 1.0f);
			v[i] = (fckc_u64)value;
		}
	}
}

void fck_nk_edit_f32(fck_serialiser *s, fck_serialiser_params *p, float *v, fckc_size_t c)
{
	fck_nk_edit_precondition(s);

	const char type_name[] = "f32";
	const float min = -1e6;
	const float max = 1e6;

	fck_ui_ctx *ctx = (fck_ui_ctx *)((fck_nk_serialiser *)s)->ctx;

	float content_ratio = 0.0f;
	fck_nk_tree_scope(ctx, &content_ratio, type_name, p->name, c)
	{
		char buffer[FCK_READ_BUFFER_STRING_SIZE];
		for (fckc_size_t i = 0; i < c; i++)
		{
			float *value = v + i;
			int result = SDL_snprintf(buffer, sizeof(buffer), "%s[%u] : %s", fck_nk_property_prefix(c), (unsigned int)i, type_name);

			nk_layout_row_push(ctx, content_ratio);
			nk_property_float(ctx, buffer, min, value, max, 1.0f, 1.0f);
			*value = SDL_clamp(*value, min, max);
		}
	}
}

void fck_nk_edit_f64(fck_serialiser *s, fck_serialiser_params *p, double *v, fckc_size_t c)
{
	fck_nk_edit_precondition(s);

	const char type_name[] = "f64";
	const double min = -1e12;
	const double max = 1e12;

	fck_ui_ctx *ctx = (fck_ui_ctx *)((fck_nk_serialiser *)s)->ctx;

	float content_ratio = 0.0f;
	fck_nk_tree_scope(ctx, &content_ratio, type_name, p->name, c)
	{
		char buffer[FCK_READ_BUFFER_STRING_SIZE];
		for (fckc_size_t i = 0; i < c; i++)
		{
			double *value = v + i;
			int result = SDL_snprintf(buffer, sizeof(buffer), "%s[%u] : %s", fck_nk_property_prefix(c), (unsigned int)i, type_name);

			nk_layout_row_push(ctx, content_ratio);
			nk_property_double(ctx, buffer, min, value, max, 1.0f, 1.0f);
			*value = SDL_clamp(*value, min, max);
		}
	}
}

void fck_nk_edit_string(fck_serialiser *s, fck_serialiser_params *p, fck_lstring *v, fckc_size_t c)
{
	fck_nk_edit_precondition(s);

	SDL_assert(false && "NOT SUPPORTED FOR NOW");
}

static fck_serialiser_vt fck_serialiser_nk_edit_vt = {
	.i8 = fck_nk_edit_i8,
	.i16 = fck_nk_edit_i16,
	.i32 = fck_nk_edit_i32,
	.i64 = fck_nk_edit_i64,
	.u8 = fck_nk_edit_u8,
	.u16 = fck_nk_edit_u16,
	.u32 = fck_nk_edit_u32,
	.u64 = fck_nk_edit_u64,
	.f32 = fck_nk_edit_f32,
	.f64 = fck_nk_edit_f64,
	.string = fck_nk_edit_string,
};

fck_serialiser_vt *fck_nk_edit_vt = &fck_serialiser_nk_edit_vt;
