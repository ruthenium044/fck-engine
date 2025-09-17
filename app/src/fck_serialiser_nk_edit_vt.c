#include "fck_serialiser_nk_edit_vt.h"

#include "fck_serialiser.h"
#include "fck_serialiser_vt.h"

#include "fck_ui.h"

#include <SDL3/SDL_assert.h>

#ifndef INT_MIN
#define INT_MIN (int)0xFFFFFFFFF
#endif

#ifndef INT_MAX
#define INT_MAX (int)0xEFFFFFFFF
#endif

#define fck_nk_scope_str_concat(lhs, rhs) lhs##rhs
#define fck_nk_scope_unique(lhs, rhs) fck_nk_scope_str_concat(lhs, rhs)
#define fck_nk_scope(ctor, dtor)                                                                                                           \
	for (int fck_nk_scope_unique(i, __LINE__) = ((ctor) ? 0 : 1); fck_nk_scope_unique(i, __LINE__) == 0;                                   \
	     fck_nk_scope_unique(i, __LINE__) += 1, (dtor))

static void fck_nk_edit_precondition(fck_serialiser *s)
{

	SDL_assert(s->vt == fck_nk_edit_vt);
}

static float fck_nk_edit_generic_label(const char *type, fck_serialiser *s, fck_serialiser_params *p, fckc_size_t c)
{
	fck_nk_edit_precondition(s);

	fck_ui_ctx *ctx = (fck_ui_ctx *)((fck_nk_serialiser *)s)->ctx;
	fckc_size_t col_count = (p->name != NULL ? 1 : 0) + c;

	const float label_ratio = 0.2f;

	if (p->name != NULL && c == 1)
	{
		// Value
		nk_layout_row_begin(ctx, NK_DYNAMIC, 25, (int)2);
		nk_layout_row_push(ctx, label_ratio);
		nk_labelf(ctx, NK_TEXT_LEFT, "%s : %s", p->name, type);
		return 1.0f - label_ratio;
	}
	else
	{
		// Array
		nk_layout_row_begin(ctx, NK_DYNAMIC, 25, (int)1);
		return 1.0f;
	}
}

static const fckc_size_t FCK_WRITE_BUFFER_STRING_SIZE = 256;
static const fckc_size_t FCK_READ_BUFFER_STRING_SIZE = 256;

void fck_nk_edit_i8(fck_serialiser *s, fck_serialiser_params *p, fckc_i8 *v, fckc_size_t c)
{
	fck_nk_edit_precondition(s);

	const char type_name[] = "u16";
	const int min = -127;
	const int max = 127;

	if (p->name == NULL && c == 0)
	{
		return;
	}

	fck_ui_ctx *ctx = (fck_ui_ctx *)((fck_nk_serialiser *)s)->ctx;
	const float value_ratio = fck_nk_edit_generic_label(type_name, s, p, c);
	const char *name_format_invisible_token = c == 1 ? "##" : "#";
	char buffer[FCK_READ_BUFFER_STRING_SIZE];

	for (fckc_size_t i = 0; i < c; i++)
	{
		float *value = v + i;
		int result = SDL_snprintf(buffer, sizeof(buffer), "%s[%u] : %s", name_format_invisible_token, (unsigned int)i, type_name);

		nk_layout_row_push(ctx, value_ratio);
		nk_property_int(ctx, buffer, min, value, max, 1, 1.0f);
		*value = SDL_clamp(*value, min, max);
	}
	nk_layout_row_end(ctx);
}

void fck_nk_edit_i16(fck_serialiser *s, fck_serialiser_params *p, fckc_i16 *v, fckc_size_t c)
{
	fck_nk_edit_precondition(s);

	const char type_name[] = "u16";
	const int min = -32767;
	const int max = 32767;

	if (p->name == NULL && c == 0)
	{
		return;
	}

	fck_ui_ctx *ctx = (fck_ui_ctx *)((fck_nk_serialiser *)s)->ctx;
	const float value_ratio = fck_nk_edit_generic_label(type_name, s, p, c);
	const char *name_format_invisible_token = c == 1 ? "##" : "#";
	char buffer[FCK_READ_BUFFER_STRING_SIZE];

	for (fckc_size_t i = 0; i < c; i++)
	{
		float *value = v + i;
		int result = SDL_snprintf(buffer, sizeof(buffer), "%s[%u] : %s", name_format_invisible_token, (unsigned int)i, type_name);

		nk_layout_row_push(ctx, value_ratio);
		nk_property_int(ctx, buffer, min, value, max, 1, 1.0f);
		*value = SDL_clamp(*value, min, max);
	}
	nk_layout_row_end(ctx);
}

void fck_nk_edit_i32(fck_serialiser *s, fck_serialiser_params *p, fckc_i32 *v, fckc_size_t c)
{
	fck_nk_edit_precondition(s);

	const char type_name[] = "i32";
	const int min = INT_MIN;
	const int max = INT_MAX;

	if (p->name == NULL && c == 0)
	{
		return;
	}

	fck_ui_ctx *ctx = (fck_ui_ctx *)((fck_nk_serialiser *)s)->ctx;
	const float value_ratio = fck_nk_edit_generic_label(type_name, s, p, c);
	const char *name_format_invisible_token = c == 1 ? "##" : "#";
	char buffer[FCK_READ_BUFFER_STRING_SIZE];

	for (fckc_size_t i = 0; i < c; i++)
	{
		float *value = v + i;
		int result = SDL_snprintf(buffer, sizeof(buffer), "%s[%u] : %s", name_format_invisible_token, (unsigned int)i, type_name);

		nk_layout_row_push(ctx, value_ratio);
		nk_property_int(ctx, buffer, min, value, max, 1, 1.0f);
		*value = SDL_clamp(*value, min, max);
	}
	nk_layout_row_end(ctx);
}

void fck_nk_edit_i64(fck_serialiser *s, fck_serialiser_params *p, fckc_i64 *v, fckc_size_t c)
{
	fck_nk_edit_precondition(s);

	const char type_name[] = "i64";
	const int min = INT_MIN;
	const int max = INT_MAX;

	if (p->name == NULL && c == 0)
	{
		return;
	}

	fck_ui_ctx *ctx = (fck_ui_ctx *)((fck_nk_serialiser *)s)->ctx;
	const float value_ratio = fck_nk_edit_generic_label(type_name, s, p, c);
	const char *name_format_invisible_token = c == 1 ? "##" : "#";
	char buffer[FCK_READ_BUFFER_STRING_SIZE];

	for (fckc_size_t i = 0; i < c; i++)
	{
		float *value = v + i;
		int result = SDL_snprintf(buffer, sizeof(buffer), "%s[%u] : %s", name_format_invisible_token, (unsigned int)i, type_name);

		nk_layout_row_push(ctx, value_ratio);
		nk_property_int(ctx, buffer, min, value, max, 1, 1.0f);
		*value = SDL_clamp(*value, min, max);
	}
	nk_layout_row_end(ctx);
}

void fck_nk_edit_u8(fck_serialiser *s, fck_serialiser_params *p, fckc_u8 *v, fckc_size_t c)
{
	fck_nk_edit_precondition(s);

	const char type_name[] = "u8";
	const int min = 0;
	const int max = 0xFF;

	if (p->name == NULL && c == 0)
	{
		return;
	}

	fck_ui_ctx *ctx = (fck_ui_ctx *)((fck_nk_serialiser *)s)->ctx;
	const float value_ratio = fck_nk_edit_generic_label(type_name, s, p, c);
	const char *name_format_invisible_token = c == 1 ? "##" : "#";
	char buffer[FCK_READ_BUFFER_STRING_SIZE];

	for (fckc_size_t i = 0; i < c; i++)
	{
		float *value = v + i;
		int result = SDL_snprintf(buffer, sizeof(buffer), "%s[%u] : %s", name_format_invisible_token, (unsigned int)i, type_name);

		nk_layout_row_push(ctx, value_ratio);
		nk_property_int(ctx, buffer, min, value, max, 1, 1.0f);
		*value = SDL_clamp(*value, min, max);
	}
	nk_layout_row_end(ctx);
}

void fck_nk_edit_u16(fck_serialiser *s, fck_serialiser_params *p, fckc_u16 *v, fckc_size_t c)
{
	fck_nk_edit_precondition(s);

	const char type_name[] = "u16";
	const int min = 0;
	const int max = 0xFFFF;

	if (p->name == NULL && c == 0)
	{
		return;
	}

	fck_ui_ctx *ctx = (fck_ui_ctx *)((fck_nk_serialiser *)s)->ctx;
	const float value_ratio = fck_nk_edit_generic_label(type_name, s, p, c);
	const char *name_format_invisible_token = c == 1 ? "##" : "#";
	char buffer[FCK_READ_BUFFER_STRING_SIZE];

	for (fckc_size_t i = 0; i < c; i++)
	{
		float *value = v + i;
		int result = SDL_snprintf(buffer, sizeof(buffer), "%s[%u] : %s", name_format_invisible_token, (unsigned int)i, type_name);

		nk_layout_row_push(ctx, value_ratio);
		nk_property_int(ctx, buffer, min, value, max, 1, 1.0f);
		*value = SDL_clamp(*value, min, max);
	}
	nk_layout_row_end(ctx);
}

void fck_nk_edit_u32(fck_serialiser *s, fck_serialiser_params *p, fckc_u32 *v, fckc_size_t c)
{
	fck_nk_edit_precondition(s);

	const char type_name[] = "u32";
	const int min = 0;
	const int max = INT_MAX;

	if (p->name == NULL && c == 0)
	{
		return;
	}

	fck_ui_ctx *ctx = (fck_ui_ctx *)((fck_nk_serialiser *)s)->ctx;
	const float value_ratio = fck_nk_edit_generic_label(type_name, s, p, c);
	const char *name_format_invisible_token = c == 1 ? "##" : "#";
	char buffer[FCK_READ_BUFFER_STRING_SIZE];

	for (fckc_size_t i = 0; i < c; i++)
	{
		float *value = v + i;
		int result = SDL_snprintf(buffer, sizeof(buffer), "%s[%u] : %s", name_format_invisible_token, (unsigned int)i, type_name);

		nk_layout_row_push(ctx, value_ratio);
		nk_property_int(ctx, buffer, min, value, max, 1, 1.0f);
		*value = SDL_clamp(*value, min, max);
	}
	nk_layout_row_end(ctx);
}

static void fck_nk_edit_u64(fck_serialiser *s, fck_serialiser_params *p, fckc_u64 *v, fckc_size_t c)
{
	fck_nk_edit_precondition(s);

	const char type_name[] = "u64";
	const int min = 0;
	const int max = INT_MAX;

	if (p->name == NULL && c == 0)
	{
		return;
	}

	fck_ui_ctx *ctx = (fck_ui_ctx *)((fck_nk_serialiser *)s)->ctx;
	const float value_ratio = fck_nk_edit_generic_label(type_name, s, p, c);
	const char *name_format_invisible_token = c == 1 ? "##" : "#";
	char buffer[FCK_READ_BUFFER_STRING_SIZE];

	for (fckc_size_t i = 0; i < c; i++)
	{
		float *value = v + i;
		int result = SDL_snprintf(buffer, sizeof(buffer), "%s[%u] : %s", name_format_invisible_token, (unsigned int)i, type_name);

		nk_layout_row_push(ctx, value_ratio);
		nk_property_int(ctx, buffer, min, value, max, 1, 1.0f);
		*value = SDL_clamp(*value, min, max);
	}
	nk_layout_row_end(ctx);
}

static int fck_nk_edit_tree_push(fck_ui_ctx *ctx, const char *type, const char *name, fckc_size_t count)
{
	char buffer[FCK_READ_BUFFER_STRING_SIZE];
	int result = SDL_snprintf(buffer, sizeof(buffer), "%s : %s[%llu]", name, type, (fckc_u64)count);
	return nk_tree_push_hashed(ctx, NK_TREE_NODE, buffer, NK_MINIMIZED, buffer, result, __LINE__);
}

static void fck_nk_edit_tree_pop(fck_ui_ctx *ctx)
{
	nk_tree_pop(ctx);
}

#define fck_nk_tree_scope(ctx, type_name, name, count)                                                                                     \
	fck_nk_scope(fck_nk_edit_tree_push(ctx, type_name, name, count), fck_nk_edit_tree_pop(ctx))

static float fck_nk_edit_value_label(const char *type, fck_serialiser *s, fck_serialiser_params *p, fckc_size_t c)
{
	fck_nk_edit_precondition(s);

	fck_ui_ctx *ctx = (fck_ui_ctx *)((fck_nk_serialiser *)s)->ctx;
	fckc_size_t col_count = (p->name != NULL ? 1 : 0) + c;

	const float label_ratio = 0.2f;

	// Value
	nk_layout_row_begin(ctx, NK_DYNAMIC, 25, (int)2);
	nk_layout_row_push(ctx, label_ratio);
	nk_labelf(ctx, NK_TEXT_LEFT, "%s : %s", p->name, type);
	return 1.0f - label_ratio;
}

static float fck_nk_edit_array_label(fck_serialiser *s)
{
	fck_nk_edit_precondition(s);

	fck_ui_ctx *ctx = (fck_ui_ctx *)((fck_nk_serialiser *)s)->ctx;
	nk_layout_row_begin(ctx, NK_DYNAMIC, 25, (int)1);
	return 1.0f;
}

void fck_nk_edit_properties_f32(fck_ui_ctx* ctx, float* v, float value_ratio, fckc_size_t c)
{
	char buffer[FCK_READ_BUFFER_STRING_SIZE];

	const char type_name[] = "f32";
	const float min = -1e6;
	const float max = 1e6;
	const char* name_format_invisible_token = c == 1 ? "##" : "#";

	for (fckc_size_t i = 0; i < c; i++)
	{
		float* value = v + i;
		int result = SDL_snprintf(buffer, sizeof(buffer), "%s[%u] : %s", name_format_invisible_token, (unsigned int)i, type_name);

		nk_layout_row_push(ctx, value_ratio);
		nk_property_float(ctx, buffer, min, value, max, 1.0f, 1.0f);
		*value = SDL_clamp(*value, min, max);
	}
	nk_layout_row_end(ctx);
}

void fck_nk_edit_f32(fck_serialiser *s, fck_serialiser_params *p, float *v, fckc_size_t c)
{
	fck_nk_edit_precondition(s);

	const char type_name[] = "f32";
	const float min = -1e6;
	const float max = 1e6;

	if (p->name == NULL && c == 0)
	{
		return;
	}

	const char *name_format_invisible_token = c == 1 ? "##" : "#";

	fck_ui_ctx *ctx = (fck_ui_ctx *)((fck_nk_serialiser *)s)->ctx;

	if (c == 1)
	{
		const float value_ratio = fck_nk_edit_value_label(type_name, s, p, c);
		fck_nk_edit_properties_f32(ctx, v, value_ratio, c);
	}
	else
	{
		fck_nk_tree_scope(ctx, type_name, p->name, c)
		{
			const float value_ratio = fck_nk_edit_array_label(s);
			fck_nk_edit_properties_f32(ctx, v, value_ratio, c);
		}
	}
}

void fck_nk_edit_f64(fck_serialiser *s, fck_serialiser_params *p, double *v, fckc_size_t c)
{
	fck_nk_edit_precondition(s);

	const char type_name[] = "f64";

	const double min = -1e12;
	const double max = 1e12;
	if (p->name == NULL && c == 0)
	{
		return;
	}

	fck_ui_ctx *ctx = (fck_ui_ctx *)((fck_nk_serialiser *)s)->ctx;
	const float value_ratio = fck_nk_edit_generic_label(type_name, s, p, c);
	const char *name_format_invisible_token = c == 1 ? "##" : "#";
	char buffer[FCK_READ_BUFFER_STRING_SIZE];

	for (fckc_size_t i = 0; i < c; i++)
	{
		float *value = v + i;
		int result = SDL_snprintf(buffer, sizeof(buffer), "%s[%u] : %s", name_format_invisible_token, (unsigned int)i, type_name);

		nk_layout_row_push(ctx, value_ratio);
		nk_property_double(ctx, buffer, min, value, max, 1.0f, 1.0f);
		*value = SDL_clamp(*value, min, max);
	}

	nk_layout_row_end(ctx);
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
