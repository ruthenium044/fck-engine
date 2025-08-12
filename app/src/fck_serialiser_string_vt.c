
#include "fck_serialiser_string_vt.h"

#include "fck_serialiser.h"
#include "fck_serialiser_vt.h"

#include <SDL3/SDL_assert.h>

static const fckc_size_t FCK_WRITE_BUFFER_STRING_SIZE = 32;
static const fckc_size_t FCK_READ_BUFFER_STRING_SIZE = 32;

void fck_write_string_i8(fck_serialiser *s, fck_serialiser_params *p, fckc_i8 *v, fckc_size_t c)
{
	fck_serialiser_maybe_realloc(s, FCK_WRITE_BUFFER_STRING_SIZE); // Should be enough

	for (fckc_i8 *end = v + c; v != end; v++)
	{
		fckc_u8 *at = s->bytes + s->at;
		int result = SDL_snprintf((char *)at, s->capacity - s->at, "%llu\n", (fckc_u64)*v);
		s->at = s->at + result;
	}
}

void fck_write_string_i16(fck_serialiser *s, fck_serialiser_params *p, fckc_i16 *v, fckc_size_t c)
{
	fck_serialiser_maybe_realloc(s, FCK_WRITE_BUFFER_STRING_SIZE); // Should be enough

	for (fckc_i16 *end = v + c; v != end; v++)
	{
		fckc_u8 *at = s->bytes + s->at;
		int result = SDL_snprintf((char *)at, s->capacity - s->at, "%llu\n", (fckc_u64)*v);
		s->at = s->at + result;
	}
}

void fck_write_string_i32(fck_serialiser *s, fck_serialiser_params *p, fckc_i32 *v, fckc_size_t c)
{
	fck_serialiser_maybe_realloc(s, FCK_WRITE_BUFFER_STRING_SIZE); // Should be enough

	for (fckc_i32 *end = v + c; v != end; v++)
	{
		fckc_u8 *at = s->bytes + s->at;
		int result = SDL_snprintf((char *)at, s->capacity - s->at, "%llu\n", (fckc_u64)*v);
		s->at = s->at + result;
	}
}

void fck_write_string_i64(fck_serialiser *s, fck_serialiser_params *p, fckc_i64 *v, fckc_size_t c)
{
	fck_serialiser_maybe_realloc(s, FCK_WRITE_BUFFER_STRING_SIZE); // Should be enough

	for (fckc_i64 *end = v + c; v != end; v++)
	{
		fckc_u8 *at = s->bytes + s->at;
		int result = SDL_snprintf((char *)at, s->capacity - s->at, "%llu\n", (fckc_u64)*v);
		s->at = s->at + result;
	}
}

void fck_write_string_u8(fck_serialiser *s, fck_serialiser_params *p, fckc_u8 *v, fckc_size_t c)
{
	fck_serialiser_maybe_realloc(s, FCK_WRITE_BUFFER_STRING_SIZE); // Should be enough

	for (fckc_u8 *end = v + c; v != end; v++)
	{
		fckc_u8 *at = s->bytes + s->at;
		int result = SDL_snprintf((char *)at, s->capacity - s->at, "%llu\n", (fckc_u64)*v);
		s->at = s->at + result;
	}
}

void fck_write_string_u16(fck_serialiser *s, fck_serialiser_params *p, fckc_u16 *v, fckc_size_t c)
{
	fck_serialiser_maybe_realloc(s, FCK_WRITE_BUFFER_STRING_SIZE); // Should be enough

	for (fckc_u16 *end = v + c; v != end; v++)
	{
		fckc_u8 *at = s->bytes + s->at;
		int result = SDL_snprintf((char *)at, s->capacity - s->at, "%llu\n", (fckc_u64)*v);
		s->at = s->at + result;
	}
}

void fck_write_string_u32(fck_serialiser *s, fck_serialiser_params *p, fckc_u32 *v, fckc_size_t c)
{
	fck_serialiser_maybe_realloc(s, FCK_WRITE_BUFFER_STRING_SIZE); // Should be enough

	for (fckc_u32 *end = v + c; v != end; v++)
	{
		fckc_u8 *at = s->bytes + s->at;
		int result = SDL_snprintf((char *)at, s->capacity - s->at, "%llu\n", (fckc_u64)*v);
		s->at = s->at + result;
	}
}

void fck_write_string_u64(fck_serialiser *s, fck_serialiser_params *p, fckc_u64 *v, fckc_size_t c)
{
	fck_serialiser_maybe_realloc(s, FCK_WRITE_BUFFER_STRING_SIZE); // Should be enough

	for (fckc_u64 *end = v + c; v != end; v++)
	{
		fckc_u8 *at = s->bytes + s->at;
		int result = SDL_snprintf((char *)at, s->capacity - s->at, "%llu\n", (fckc_u64)*v);
		s->at = s->at + result;
	}
}

void fck_write_string_f32(fck_serialiser *s, fck_serialiser_params *p, float *v, fckc_size_t c)
{
	fck_serialiser_maybe_realloc(s, FCK_WRITE_BUFFER_STRING_SIZE); // Should be enough

	for (float *end = v + c; v != end; v++)
	{
		fckc_u8 *at = s->bytes + s->at;
		int result = SDL_snprintf((char *)at, s->capacity - s->at, "%f\n", *v);
		SDL_assert(result >= 0);
		s->at = s->at + result; // Consider null-terminator
	}
}

void fck_write_string_f64(fck_serialiser *s, fck_serialiser_params *p, double *v, fckc_size_t c)
{
	fck_serialiser_maybe_realloc(s, FCK_WRITE_BUFFER_STRING_SIZE); // Should be enough

	for (double *end = v + c; v != end; v++)
	{
		fckc_u8 *at = s->bytes + s->at;
		int result = SDL_snprintf((char *)at, s->capacity - s->at, "%f\n", *v);
		s->at = s->at + result;
	}
}

void fck_write_string_string(fck_serialiser *s, fck_serialiser_params *p, fck_lstring *v, fckc_size_t c)
{
	SDL_assert(false && "NOT SUPPORTED FOR NOW");
}

static fckc_size_t fck_read_string_number_length(fck_serialiser *s)
{
	for (fckc_size_t index = s->at; index < s->capacity; index++)
	{
		if (!SDL_isspace(s->bytes[index]))
		{
			if (SDL_isdigit(s->bytes[index]))
			{
				continue;
			}
			if (s->bytes[index] == '.')
			{
				continue;
			}
		}
		return index - s->at;
	}
	return 0;
}

void fck_read_string_i8(fck_serialiser *s, fck_serialiser_params *p, fckc_i8 *v, fckc_size_t c)
{
	char buffer[FCK_READ_BUFFER_STRING_SIZE];

	for (fckc_i8 *end = v + c; v != end; v++)
	{
		fckc_size_t len = fck_read_string_number_length(s);
		SDL_assert(len != 0 && len < sizeof(buffer));

		fckc_u8 *at = s->bytes + s->at;
		SDL_memcpy(buffer, at, len);
		buffer[len] = '\0';

		long long value = SDL_strtoll(buffer, NULL, 10);
		*v = (fckc_i8)value;
		s->at = s->at + len + 1;
	}
}

void fck_read_string_i16(fck_serialiser *s, fck_serialiser_params *p, fckc_i16 *v, fckc_size_t c)
{
	char buffer[FCK_READ_BUFFER_STRING_SIZE];

	for (fckc_i16 *end = v + c; v != end; v++)
	{
		fckc_size_t len = fck_read_string_number_length(s);
		SDL_assert(len != 0 && len < sizeof(buffer));

		fckc_u8 *at = s->bytes + s->at;
		SDL_memcpy(buffer, at, len);
		buffer[len] = '\0';

		long long value = SDL_strtoll(buffer, NULL, 10);
		*v = (fckc_i16)value;
		s->at = s->at + len + 1;
	}
}

void fck_read_string_i32(fck_serialiser *s, fck_serialiser_params *p, fckc_i32 *v, fckc_size_t c)
{
	char buffer[FCK_READ_BUFFER_STRING_SIZE];

	for (fckc_i32 *end = v + c; v != end; v++)
	{
		fckc_size_t len = fck_read_string_number_length(s);
		SDL_assert(len != 0 && len < sizeof(buffer));

		fckc_u8 *at = s->bytes + s->at;
		SDL_memcpy(buffer, at, len);
		buffer[len] = '\0';

		long long value = SDL_strtoll(buffer, NULL, 10);
		*v = (fckc_i32)value;
		s->at = s->at + len + 1;
	}
}

void fck_read_string_i64(fck_serialiser *s, fck_serialiser_params *p, fckc_i64 *v, fckc_size_t c)
{
	char buffer[FCK_READ_BUFFER_STRING_SIZE];

	for (fckc_i64 *end = v + c; v != end; v++)
	{
		fckc_size_t len = fck_read_string_number_length(s);
		SDL_assert(len != 0 && len < sizeof(buffer));

		fckc_u8 *at = s->bytes + s->at;
		SDL_memcpy(buffer, at, len);
		buffer[len] = '\0';

		long long value = SDL_strtoll(buffer, NULL, 10);
		*v = (fckc_i64)value;
		s->at = s->at + len + 1;
	}
}

void fck_read_string_u8(fck_serialiser *s, fck_serialiser_params *p, fckc_u8 *v, fckc_size_t c)
{
	char buffer[FCK_READ_BUFFER_STRING_SIZE];

	for (fckc_u8 *end = v + c; v != end; v++)
	{
		fckc_size_t len = fck_read_string_number_length(s);
		SDL_assert(len != 0 && len < sizeof(buffer));

		fckc_u8 *at = s->bytes + s->at;
		SDL_memcpy(buffer, at, len);
		buffer[len] = '\0';

		unsigned long long value = SDL_strtoull(buffer, NULL, 10);
		*v = (fckc_u8)value;
		s->at = s->at + len + 1;
	}
}

void fck_read_string_u16(fck_serialiser *s, fck_serialiser_params *p, fckc_u16 *v, fckc_size_t c)
{
	char buffer[FCK_READ_BUFFER_STRING_SIZE];

	for (fckc_u16 *end = v + c; v != end; v++)
	{
		fckc_size_t len = fck_read_string_number_length(s);
		SDL_assert(len != 0 && len < sizeof(buffer));

		fckc_u8 *at = s->bytes + s->at;
		SDL_memcpy(buffer, at, len);
		buffer[len] = '\0';

		unsigned long long value = SDL_strtoull(buffer, NULL, 10);
		*v = (fckc_u16)value;
		s->at = s->at + len + 1;
	}
}

void fck_read_string_u32(fck_serialiser *s, fck_serialiser_params *p, fckc_u32 *v, fckc_size_t c)
{
	char buffer[FCK_READ_BUFFER_STRING_SIZE];

	for (fckc_u32 *end = v + c; v != end; v++)
	{
		fckc_size_t len = fck_read_string_number_length(s);
		SDL_assert(len != 0 && len < sizeof(buffer));

		fckc_u8 *at = s->bytes + s->at;
		SDL_memcpy(buffer, at, len);
		buffer[len] = '\0';

		unsigned long long value = SDL_strtoull(buffer, NULL, 10);
		*v = (fckc_u32)value;
		s->at = s->at + len + 1;
	}
}

void fck_read_string_u64(fck_serialiser *s, fck_serialiser_params *p, fckc_u64 *v, fckc_size_t c)
{
	char buffer[FCK_READ_BUFFER_STRING_SIZE];

	for (fckc_u64 *end = v + c; v != end; v++)
	{
		fckc_size_t len = fck_read_string_number_length(s);
		SDL_assert(len != 0 && len < sizeof(buffer));

		fckc_u8 *at = s->bytes + s->at;
		SDL_memcpy(buffer, at, len);
		buffer[len] = '\0';

		unsigned long long value = SDL_strtoull(buffer, NULL, 10);
		*v = (fckc_u64)value;
		s->at = s->at + len + 1;
	}
}

void fck_read_string_f32(fck_serialiser *s, fck_serialiser_params *p, float *v, fckc_size_t c)
{
	char buffer[FCK_READ_BUFFER_STRING_SIZE];

	for (float *end = v + c; v != end; v++)
	{
		fckc_size_t len = fck_read_string_number_length(s);
		SDL_assert(len != 0 && len < sizeof(buffer));

		fckc_u8 *at = s->bytes + s->at;
		SDL_memcpy(buffer, at, len);
		buffer[len] = '\0';

		double value = SDL_strtod(buffer, NULL);
		*v = (float)value;
		s->at = s->at + len + 1;
	}
}

void fck_read_string_f64(fck_serialiser *s, fck_serialiser_params *p, double *v, fckc_size_t c)
{
	char buffer[FCK_READ_BUFFER_STRING_SIZE];

	for (double *end = v + c; v != end; v++)
	{
		fckc_size_t len = fck_read_string_number_length(s);
		SDL_assert(len != 0 && len < sizeof(buffer));

		fckc_u8 *at = s->bytes + s->at;
		SDL_memcpy(buffer, at, len);
		buffer[len] = '\0';

		double value = SDL_strtod(buffer, NULL);
		*v = (double)value;
		s->at = s->at + len + 1;
	}
}

void fck_read_string_string(fck_serialiser *s, fck_serialiser_params *p, fck_lstring *v, fckc_size_t c)
{
	SDL_assert(false && "NOT SUPPORTED FOR NOW");
}

static fck_serialiser_vt fck_serialiser_string_writer_vt = {
	.i8 = fck_write_string_i8,
	.i16 = fck_write_string_i16,
	.i32 = fck_write_string_i32,
	.i64 = fck_write_string_i64,
	.u8 = fck_write_string_u8,
	.u16 = fck_write_string_u16,
	.u32 = fck_write_string_u32,
	.u64 = fck_write_string_u64,
	.f32 = fck_write_string_f32,
	.f64 = fck_write_string_f64,
	.string = fck_write_string_string,
};

static fck_serialiser_vt fck_serialiser_string_reader_vt = {
	.i8 = fck_read_string_i8,
	.i16 = fck_read_string_i16,
	.i32 = fck_read_string_i32,
	.i64 = fck_read_string_i64,
	.u8 = fck_read_string_u8,
	.u16 = fck_read_string_u16,
	.u32 = fck_read_string_u32,
	.u64 = fck_read_string_u64,
	.f32 = fck_read_string_f32,
	.f64 = fck_read_string_f64,
	.string = fck_read_string_string,
};

fck_serialiser_vt *fck_string_writer_vt = &fck_serialiser_string_writer_vt;
fck_serialiser_vt *fck_string_reader_vt = &fck_serialiser_string_reader_vt;
