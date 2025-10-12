
#include "fck_serialiser_string_vt.h"

#include "fck_serialiser.h"
#include "fck_serialiser_vt.h"

#include <SDL3/SDL_assert.h>

#define FCK_WRITE_BUFFER_STRING_SIZE 32
#define FCK_READ_BUFFER_STRING_SIZE 32

void fck_write_string_i8(fck_serialiser *s, fck_serialiser_params *p, fckc_i8 *v, fckc_size_t c)
{
	fck_memory_serialiser_maybe_realloc(s, FCK_WRITE_BUFFER_STRING_SIZE); // Should be enough

	for (fckc_i8 *end = v + c; v != end; v++)
	{
		fckc_u8 *at = ((fck_memory_serialiser*)s)->bytes + ((fck_memory_serialiser*)s)->at;
		int result = SDL_snprintf((char *)at, ((fck_memory_serialiser*)s)->capacity - ((fck_memory_serialiser*)s)->at, "%llu\n", (fckc_u64)*v);
		((fck_memory_serialiser*)s)->at = ((fck_memory_serialiser*)s)->at + result;
	}
}

void fck_write_string_i16(fck_serialiser *s, fck_serialiser_params *p, fckc_i16 *v, fckc_size_t c)
{
	fck_memory_serialiser_maybe_realloc(s, FCK_WRITE_BUFFER_STRING_SIZE); // Should be enough

	for (fckc_i16 *end = v + c; v != end; v++)
	{
		fckc_u8 *at = ((fck_memory_serialiser*)s)->bytes + ((fck_memory_serialiser*)s)->at;
		int result = SDL_snprintf((char *)at, ((fck_memory_serialiser*)s)->capacity - ((fck_memory_serialiser*)s)->at, "%llu\n", (fckc_u64)*v);
		((fck_memory_serialiser*)s)->at = ((fck_memory_serialiser*)s)->at + result;
	}
}

void fck_write_string_i32(fck_serialiser *s, fck_serialiser_params *p, fckc_i32 *v, fckc_size_t c)
{
	fck_memory_serialiser_maybe_realloc(s, FCK_WRITE_BUFFER_STRING_SIZE); // Should be enough

	for (fckc_i32 *end = v + c; v != end; v++)
	{
		fckc_u8 *at = ((fck_memory_serialiser*)s)->bytes + ((fck_memory_serialiser*)s)->at;
		int result = SDL_snprintf((char *)at, ((fck_memory_serialiser*)s)->capacity - ((fck_memory_serialiser*)s)->at, "%llu\n", (fckc_u64)*v);
		((fck_memory_serialiser*)s)->at = ((fck_memory_serialiser*)s)->at + result;
	}
}

void fck_write_string_i64(fck_serialiser *s, fck_serialiser_params *p, fckc_i64 *v, fckc_size_t c)
{
	fck_memory_serialiser_maybe_realloc(s, FCK_WRITE_BUFFER_STRING_SIZE); // Should be enough

	for (fckc_i64 *end = v + c; v != end; v++)
	{
		fckc_u8 *at = ((fck_memory_serialiser*)s)->bytes + ((fck_memory_serialiser*)s)->at;
		int result = SDL_snprintf((char *)at, ((fck_memory_serialiser*)s)->capacity - ((fck_memory_serialiser*)s)->at, "%llu\n", (fckc_u64)*v);
		((fck_memory_serialiser*)s)->at = ((fck_memory_serialiser*)s)->at + result;
	}
}

void fck_write_string_u8(fck_serialiser *s, fck_serialiser_params *p, fckc_u8 *v, fckc_size_t c)
{
	fck_memory_serialiser_maybe_realloc(s, FCK_WRITE_BUFFER_STRING_SIZE); // Should be enough

	for (fckc_u8 *end = v + c; v != end; v++)
	{
		fckc_u8 *at = ((fck_memory_serialiser*)s)->bytes + ((fck_memory_serialiser*)s)->at;
		int result = SDL_snprintf((char *)at, ((fck_memory_serialiser*)s)->capacity - ((fck_memory_serialiser*)s)->at, "%llu\n", (fckc_u64)*v);
		((fck_memory_serialiser*)s)->at = ((fck_memory_serialiser*)s)->at + result;
	}
}

void fck_write_string_u16(fck_serialiser *s, fck_serialiser_params *p, fckc_u16 *v, fckc_size_t c)
{
	fck_memory_serialiser_maybe_realloc(s, FCK_WRITE_BUFFER_STRING_SIZE); // Should be enough

	for (fckc_u16 *end = v + c; v != end; v++)
	{
		fckc_u8 *at = ((fck_memory_serialiser*)s)->bytes + ((fck_memory_serialiser*)s)->at;
		int result = SDL_snprintf((char *)at, ((fck_memory_serialiser*)s)->capacity - ((fck_memory_serialiser*)s)->at, "%llu\n", (fckc_u64)*v);
		((fck_memory_serialiser*)s)->at = ((fck_memory_serialiser*)s)->at + result;
	}
}

void fck_write_string_u32(fck_serialiser *s, fck_serialiser_params *p, fckc_u32 *v, fckc_size_t c)
{
	fck_memory_serialiser_maybe_realloc(s, FCK_WRITE_BUFFER_STRING_SIZE); // Should be enough

	for (fckc_u32 *end = v + c; v != end; v++)
	{
		fckc_u8 *at = ((fck_memory_serialiser*)s)->bytes + ((fck_memory_serialiser*)s)->at;
		int result = SDL_snprintf((char *)at, ((fck_memory_serialiser*)s)->capacity - ((fck_memory_serialiser*)s)->at, "%llu\n", (fckc_u64)*v);
		((fck_memory_serialiser*)s)->at = ((fck_memory_serialiser*)s)->at + result;
	}
}

void fck_write_string_u64(fck_serialiser *s, fck_serialiser_params *p, fckc_u64 *v, fckc_size_t c)
{
	fck_memory_serialiser_maybe_realloc(s, FCK_WRITE_BUFFER_STRING_SIZE); // Should be enough

	for (fckc_u64 *end = v + c; v != end; v++)
	{
		fckc_u8 *at = ((fck_memory_serialiser*)s)->bytes + ((fck_memory_serialiser*)s)->at;
		int result = SDL_snprintf((char *)at, ((fck_memory_serialiser*)s)->capacity - ((fck_memory_serialiser*)s)->at, "%llu\n", (fckc_u64)*v);
		((fck_memory_serialiser*)s)->at = ((fck_memory_serialiser*)s)->at + result;
	}
}

void fck_write_string_f32(fck_serialiser *s, fck_serialiser_params *p, float *v, fckc_size_t c)
{
	fck_memory_serialiser_maybe_realloc(s, FCK_WRITE_BUFFER_STRING_SIZE); // Should be enough

	for (float *end = v + c; v != end; v++)
	{
		fckc_u8 *at = ((fck_memory_serialiser*)s)->bytes + ((fck_memory_serialiser*)s)->at;
		int result = SDL_snprintf((char *)at, ((fck_memory_serialiser*)s)->capacity - ((fck_memory_serialiser*)s)->at, "%f\n", *v);
		SDL_assert(result >= 0);
		((fck_memory_serialiser*)s)->at = ((fck_memory_serialiser*)s)->at + result; // Consider null-terminator
	}
}

void fck_write_string_f64(fck_serialiser *s, fck_serialiser_params *p, double *v, fckc_size_t c)
{
	fck_memory_serialiser_maybe_realloc(s, FCK_WRITE_BUFFER_STRING_SIZE); // Should be enough

	for (double *end = v + c; v != end; v++)
	{
		fckc_u8 *at = ((fck_memory_serialiser*)s)->bytes + ((fck_memory_serialiser*)s)->at;
		int result = SDL_snprintf((char *)at, ((fck_memory_serialiser*)s)->capacity - ((fck_memory_serialiser*)s)->at, "%f\n", *v);
		((fck_memory_serialiser*)s)->at = ((fck_memory_serialiser*)s)->at + result;
	}
}

void fck_write_string_string(fck_serialiser *s, fck_serialiser_params *p, fck_lstring *v, fckc_size_t c)
{
	SDL_assert(0 && "NOT SUPPORTED FOR NOW");
}

static fckc_size_t fck_read_string_number_length(fck_serialiser *s)
{
	for (fckc_size_t index = ((fck_memory_serialiser*)s)->at; index < ((fck_memory_serialiser*)s)->capacity; index++)
	{
		if (!SDL_isspace(((fck_memory_serialiser*)s)->bytes[index]))
		{
			if (SDL_isdigit(((fck_memory_serialiser*)s)->bytes[index]))
			{
				continue;
			}
			if (((fck_memory_serialiser*)s)->bytes[index] == '.')
			{
				continue;
			}
		}
		return index - ((fck_memory_serialiser*)s)->at;
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

		fckc_u8 *at = ((fck_memory_serialiser*)s)->bytes + ((fck_memory_serialiser*)s)->at;
		SDL_memcpy(buffer, at, len);
		buffer[len] = '\0';

		long long value = SDL_strtoll(buffer, NULL, 10);
		*v = (fckc_i8)value;
		((fck_memory_serialiser*)s)->at = ((fck_memory_serialiser*)s)->at + len + 1;
	}
}

void fck_read_string_i16(fck_serialiser *s, fck_serialiser_params *p, fckc_i16 *v, fckc_size_t c)
{
	char buffer[FCK_READ_BUFFER_STRING_SIZE];

	for (fckc_i16 *end = v + c; v != end; v++)
	{
		fckc_size_t len = fck_read_string_number_length(s);
		SDL_assert(len != 0 && len < sizeof(buffer));

		fckc_u8 *at = ((fck_memory_serialiser*)s)->bytes + ((fck_memory_serialiser*)s)->at;
		SDL_memcpy(buffer, at, len);
		buffer[len] = '\0';

		long long value = SDL_strtoll(buffer, NULL, 10);
		*v = (fckc_i16)value;
		((fck_memory_serialiser*)s)->at = ((fck_memory_serialiser*)s)->at + len + 1;
	}
}

void fck_read_string_i32(fck_serialiser *s, fck_serialiser_params *p, fckc_i32 *v, fckc_size_t c)
{
	char buffer[FCK_READ_BUFFER_STRING_SIZE];

	for (fckc_i32 *end = v + c; v != end; v++)
	{
		fckc_size_t len = fck_read_string_number_length(s);
		SDL_assert(len != 0 && len < sizeof(buffer));

		fckc_u8 *at = ((fck_memory_serialiser*)s)->bytes + ((fck_memory_serialiser*)s)->at;
		SDL_memcpy(buffer, at, len);
		buffer[len] = '\0';

		long long value = SDL_strtoll(buffer, NULL, 10);
		*v = (fckc_i32)value;
		((fck_memory_serialiser*)s)->at = ((fck_memory_serialiser*)s)->at + len + 1;
	}
}

void fck_read_string_i64(fck_serialiser *s, fck_serialiser_params *p, fckc_i64 *v, fckc_size_t c)
{
	char buffer[FCK_READ_BUFFER_STRING_SIZE];

	for (fckc_i64 *end = v + c; v != end; v++)
	{
		fckc_size_t len = fck_read_string_number_length(s);
		SDL_assert(len != 0 && len < sizeof(buffer));

		fckc_u8 *at = ((fck_memory_serialiser*)s)->bytes + ((fck_memory_serialiser*)s)->at;
		SDL_memcpy(buffer, at, len);
		buffer[len] = '\0';

		long long value = SDL_strtoll(buffer, NULL, 10);
		*v = (fckc_i64)value;
		((fck_memory_serialiser*)s)->at = ((fck_memory_serialiser*)s)->at + len + 1;
	}
}

void fck_read_string_u8(fck_serialiser *s, fck_serialiser_params *p, fckc_u8 *v, fckc_size_t c)
{
	char buffer[FCK_READ_BUFFER_STRING_SIZE];

	for (fckc_u8 *end = v + c; v != end; v++)
	{
		fckc_size_t len = fck_read_string_number_length(s);
		SDL_assert(len != 0 && len < sizeof(buffer));

		fckc_u8 *at = ((fck_memory_serialiser*)s)->bytes + ((fck_memory_serialiser*)s)->at;
		SDL_memcpy(buffer, at, len);
		buffer[len] = '\0';

		unsigned long long value = SDL_strtoull(buffer, NULL, 10);
		*v = (fckc_u8)value;
		((fck_memory_serialiser*)s)->at = ((fck_memory_serialiser*)s)->at + len + 1;
	}
}

void fck_read_string_u16(fck_serialiser *s, fck_serialiser_params *p, fckc_u16 *v, fckc_size_t c)
{
	char buffer[FCK_READ_BUFFER_STRING_SIZE];

	for (fckc_u16 *end = v + c; v != end; v++)
	{
		fckc_size_t len = fck_read_string_number_length(s);
		SDL_assert(len != 0 && len < sizeof(buffer));

		fckc_u8 *at = ((fck_memory_serialiser*)s)->bytes + ((fck_memory_serialiser*)s)->at;
		SDL_memcpy(buffer, at, len);
		buffer[len] = '\0';

		unsigned long long value = SDL_strtoull(buffer, NULL, 10);
		*v = (fckc_u16)value;
		((fck_memory_serialiser*)s)->at = ((fck_memory_serialiser*)s)->at + len + 1;
	}
}

void fck_read_string_u32(fck_serialiser *s, fck_serialiser_params *p, fckc_u32 *v, fckc_size_t c)
{
	char buffer[FCK_READ_BUFFER_STRING_SIZE];

	for (fckc_u32 *end = v + c; v != end; v++)
	{
		fckc_size_t len = fck_read_string_number_length(s);
		SDL_assert(len != 0 && len < sizeof(buffer));

		fckc_u8 *at = ((fck_memory_serialiser*)s)->bytes + ((fck_memory_serialiser*)s)->at;
		SDL_memcpy(buffer, at, len);
		buffer[len] = '\0';

		unsigned long long value = SDL_strtoull(buffer, NULL, 10);
		*v = (fckc_u32)value;
		((fck_memory_serialiser*)s)->at = ((fck_memory_serialiser*)s)->at + len + 1;
	}
}

void fck_read_string_u64(fck_serialiser *s, fck_serialiser_params *p, fckc_u64 *v, fckc_size_t c)
{
	char buffer[FCK_READ_BUFFER_STRING_SIZE];

	for (fckc_u64 *end = v + c; v != end; v++)
	{
		fckc_size_t len = fck_read_string_number_length(s);
		SDL_assert(len != 0 && len < sizeof(buffer));

		fckc_u8 *at = ((fck_memory_serialiser*)s)->bytes + ((fck_memory_serialiser*)s)->at;
		SDL_memcpy(buffer, at, len);
		buffer[len] = '\0';

		unsigned long long value = SDL_strtoull(buffer, NULL, 10);
		*v = (fckc_u64)value;
		((fck_memory_serialiser*)s)->at = ((fck_memory_serialiser*)s)->at + len + 1;
	}
}

void fck_read_string_f32(fck_serialiser *s, fck_serialiser_params *p, float *v, fckc_size_t c)
{
	char buffer[FCK_READ_BUFFER_STRING_SIZE];

	for (float *end = v + c; v != end; v++)
	{
		fckc_size_t len = fck_read_string_number_length(s);
		SDL_assert(len != 0 && len < sizeof(buffer));

		fckc_u8 *at = ((fck_memory_serialiser*)s)->bytes + ((fck_memory_serialiser*)s)->at;
		SDL_memcpy(buffer, at, len);
		buffer[len] = '\0';

		double value = SDL_strtod(buffer, NULL);
		*v = (float)value;
		((fck_memory_serialiser*)s)->at = ((fck_memory_serialiser*)s)->at + len + 1;
	}
}

void fck_read_string_f64(fck_serialiser *s, fck_serialiser_params *p, double *v, fckc_size_t c)
{
	char buffer[FCK_READ_BUFFER_STRING_SIZE];

	for (double *end = v + c; v != end; v++)
	{
		fckc_size_t len = fck_read_string_number_length(s);
		SDL_assert(len != 0 && len < sizeof(buffer));

		fckc_u8 *at = ((fck_memory_serialiser*)s)->bytes + ((fck_memory_serialiser*)s)->at;
		SDL_memcpy(buffer, at, len);
		buffer[len] = '\0';

		double value = SDL_strtod(buffer, NULL);
		*v = (double)value;
		((fck_memory_serialiser*)s)->at = ((fck_memory_serialiser*)s)->at + len + 1;
	}
}

void fck_read_string_string(fck_serialiser *s, fck_serialiser_params *p, fck_lstring *v, fckc_size_t c)
{
	SDL_assert(0 && "NOT SUPPORTED FOR NOW");
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
