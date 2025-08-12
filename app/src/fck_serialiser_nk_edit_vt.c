#include "fck_serialiser_nk_edit_vt.h"

#include "fck_serialiser.h"
#include "fck_serialiser_vt.h"

#include "fck_ui.h"

#include <SDL3/SDL_assert.h>
#include <float.h>

#ifndef INT_MIN
#define INT_MIN (int)0xFFFFFFFFF
#endif

#ifndef INT_MAX
#define INT_MAX (int)0xEFFFFFFFF
#endif

static const fckc_size_t FCK_WRITE_BUFFER_STRING_SIZE = 256;
static const fckc_size_t FCK_READ_BUFFER_STRING_SIZE = 256;

void fck_nk_edit_i8(fck_serialiser *s, fck_serialiser_params *p, fckc_i8 *v, fckc_size_t c)
{
	char buffer[FCK_READ_BUFFER_STRING_SIZE];
	static const float ratio[] = {120, 150};

	fck_ui_ctx *ctx = (fck_ui_ctx *)s->user;

	for (fckc_i8 *end = v + c; v != end; v++)
	{
		nk_layout_row(ctx, NK_STATIC, 25, 2, ratio);
		nk_label(ctx, p->name ? p->name : "", NK_TEXT_LEFT);
		int result = SDL_snprintf(buffer, sizeof(buffer), "##%s", p->name);
		int value = *(int *)v;
		nk_property_int(ctx, buffer, INT_MIN, &value, INT_MAX, 1.0, 1.0f);
		*v = (fckc_i8)value;
	}
}

void fck_nk_edit_i16(fck_serialiser *s, fck_serialiser_params *p, fckc_i16 *v, fckc_size_t c)
{
	char buffer[FCK_READ_BUFFER_STRING_SIZE];
	static const float ratio[] = {120, 150};

	fck_ui_ctx *ctx = (fck_ui_ctx *)s->user;

	for (fckc_i16 *end = v + c; v != end; v++)
	{
		nk_layout_row(ctx, NK_STATIC, 25, 2, ratio);
		nk_label(ctx, p->name ? p->name : "", NK_TEXT_LEFT);
		int result = SDL_snprintf(buffer, sizeof(buffer), "##%s", p->name);
		int value = *(int *)v;
		nk_property_int(ctx, buffer, INT_MIN, &value, INT_MAX, 1.0, 1.0f);
		*v = (fckc_i16)value;
	}
}

void fck_nk_edit_i32(fck_serialiser *s, fck_serialiser_params *p, fckc_i32 *v, fckc_size_t c)
{
	char buffer[FCK_READ_BUFFER_STRING_SIZE];
	static const float ratio[] = {120, 150};

	fck_ui_ctx *ctx = (fck_ui_ctx *)s->user;

	for (fckc_i32 *end = v + c; v != end; v++)
	{
		nk_layout_row(ctx, NK_STATIC, 25, 2, ratio);
		nk_label(ctx, p->name ? p->name : "", NK_TEXT_LEFT);
		int result = SDL_snprintf(buffer, sizeof(buffer), "##%s", p->name);
		int value = *(int *)v;
		nk_property_int(ctx, buffer, INT_MIN, &value, INT_MAX, 1.0, 1.0f);
		*v = (fckc_i32)value;
	}
}

void fck_nk_edit_i64(fck_serialiser *s, fck_serialiser_params *p, fckc_i64 *v, fckc_size_t c)
{
	char buffer[FCK_READ_BUFFER_STRING_SIZE];
	static const float ratio[] = {120, 150};

	fck_ui_ctx *ctx = (fck_ui_ctx *)s->user;

	for (fckc_i64 *end = v + c; v != end; v++)
	{
		nk_layout_row(ctx, NK_STATIC, 25, 2, ratio);
		nk_label(ctx, p->name ? p->name : "", NK_TEXT_LEFT);
		int result = SDL_snprintf(buffer, sizeof(buffer), "##%s", p->name);
		int value = *(int *)v;
		nk_property_int(ctx, buffer, INT_MIN, &value, INT_MAX, 1.0, 1.0f);
		*v = (fckc_i64)value;
	}
}

void fck_nk_edit_u8(fck_serialiser *s, fck_serialiser_params *p, fckc_u8 *v, fckc_size_t c)
{
	char buffer[FCK_READ_BUFFER_STRING_SIZE];
	static const float ratio[] = {120, 150};

	fck_ui_ctx *ctx = (fck_ui_ctx *)s->user;

	for (fckc_u8 *end = v + c; v != end; v++)
	{
		nk_layout_row(ctx, NK_STATIC, 25, 2, ratio);
		nk_label(ctx, p->name ? p->name : "", NK_TEXT_LEFT);
		int result = SDL_snprintf(buffer, sizeof(buffer), "##%s", p->name);
		int value = *(int *)v;
		nk_property_int(ctx, buffer, 0, &value, INT_MAX, 1.0, 1.0f);
		*v = (fckc_u8)value;
	}
}

void fck_nk_edit_u16(fck_serialiser *s, fck_serialiser_params *p, fckc_u16 *v, fckc_size_t c)
{
	char buffer[FCK_READ_BUFFER_STRING_SIZE];
	static const float ratio[] = {120, 150};

	fck_ui_ctx *ctx = (fck_ui_ctx *)s->user;

	for (fckc_u16 *end = v + c; v != end; v++)
	{
		nk_layout_row(ctx, NK_STATIC, 25, 2, ratio);
		nk_label(ctx, p->name ? p->name : "", NK_TEXT_LEFT);
		int result = SDL_snprintf(buffer, sizeof(buffer), "##%s", p->name);
		int value = *(int *)v;
		nk_property_int(ctx, buffer, 0, &value, INT_MAX, 1.0, 1.0f);
		*v = (fckc_u16)value;
	}
}

void fck_nk_edit_u32(fck_serialiser *s, fck_serialiser_params *p, fckc_u32 *v, fckc_size_t c)
{
	char buffer[FCK_READ_BUFFER_STRING_SIZE];
	static const float ratio[] = {120, 150};

	fck_ui_ctx *ctx = (fck_ui_ctx *)s->user;

	for (fckc_u32 *end = v + c; v != end; v++)
	{
		nk_layout_row(ctx, NK_STATIC, 25, 2, ratio);
		nk_label(ctx, p->name ? p->name : "", NK_TEXT_LEFT);
		int result = SDL_snprintf(buffer, sizeof(buffer), "##%s", p->name);
		int value = *(int *)v;
		nk_property_int(ctx, buffer, 0, &value, INT_MAX, 1.0, 1.0f);
		*v = (fckc_u32)value;
	}
}

void fck_nk_edit_u64(fck_serialiser *s, fck_serialiser_params *p, fckc_u64 *v, fckc_size_t c)
{
	char buffer[FCK_READ_BUFFER_STRING_SIZE];
	static const float ratio[] = {120, 150};

	fck_ui_ctx *ctx = (fck_ui_ctx *)s->user;

	for (fckc_u64 *end = v + c; v != end; v++)
	{
		nk_layout_row(ctx, NK_STATIC, 25, 2, ratio);
		nk_label(ctx, p->name ? p->name : "", NK_TEXT_LEFT);
		int result = SDL_snprintf(buffer, sizeof(buffer), "##%s", p->name);
		int value = *(int *)v;
		nk_property_int(ctx, buffer, 0, &value, INT_MAX, 1.0, 1.0f);
		*v = (fckc_u64)value;
	}
}

void fck_nk_edit_f32(fck_serialiser *s, fck_serialiser_params *p, float *v, fckc_size_t c)
{
	char buffer[FCK_READ_BUFFER_STRING_SIZE];
	static const float ratio[] = {120, 150};
	const float min = -1e6;
	const float max = 1e6;
	fck_ui_ctx *ctx = (fck_ui_ctx *)s->user;

	for (float *end = v + c; v != end; v++)
	{
		nk_layout_row(ctx, NK_STATIC, 25, 2, ratio);
		nk_label(ctx, p->name ? p->name : "", NK_TEXT_LEFT);
		int result = SDL_snprintf(buffer, sizeof(buffer), "##%s", p->name);
		nk_property_float(ctx, buffer, min, v, max, 1.0f, 1.0f);
		*v = SDL_clamp(*v, min, max);
	}
}

void fck_nk_edit_f64(fck_serialiser *s, fck_serialiser_params *p, double *v, fckc_size_t c)
{
	char buffer[FCK_READ_BUFFER_STRING_SIZE];
	static const float ratio[] = {120, 150};
	const double min = -1e12;
	const double max = 1e12;
	fck_ui_ctx *ctx = (fck_ui_ctx *)s->user;

	for (double *end = v + c; v != end; v++)
	{
		nk_layout_row(ctx, NK_STATIC, 25, 2, ratio);
		nk_label(ctx, p->name ? p->name : "", NK_TEXT_LEFT);
		int result = SDL_snprintf(buffer, sizeof(buffer), "##%s", p->name);
		nk_property_double(ctx, buffer, min, v, max, 1.0f, 1.0f);
		*v = SDL_clamp(*v, min, max);
	}
}

void fck_nk_edit_string(fck_serialiser *s, fck_serialiser_params *p, fck_lstring *v, fckc_size_t c)
{
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
