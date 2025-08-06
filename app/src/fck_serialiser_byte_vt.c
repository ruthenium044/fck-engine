
#include "fck_serialiser_byte_vt.h"
#include "fck_serialiser_vt.h"

#include "fck_serialiser.h"

#include <SDL3/SDL_assert.h>

void fck_read_byte_i8(fck_serialiser *s, fck_serialiser_params* p, fckc_i8 *v, fckc_size_t c)
{
	SDL_assert((s->at + sizeof(*v) * c) <= s->capacity && "Out of bounds read");

	SDL_memcpy(v, s->bytes + s->at, c);
	s->at = s->at + sizeof(*v);
}

void fck_read_byte_i16(fck_serialiser *s, fck_serialiser_params* p, fckc_i16 *v, fckc_size_t c)
{
	SDL_assert((s->at + sizeof(*v) * c) <= s->capacity && "Out of bounds read");

	for (fckc_i16 *end = v + c; v != end; v++)
	{
		fckc_u8 *at = s->bytes + s->at;
		*v = ((fckc_i16)at[0] << 0ull)    // 0xFF00000000000000
		     | ((fckc_i16)at[1] << 8ull); // 0x00FF000000000000
		s->at = s->at + sizeof(*v);
	}
}

void fck_read_byte_i32(fck_serialiser *s, fck_serialiser_params* p, fckc_i32 *v, fckc_size_t c)
{
	SDL_assert((s->at + sizeof(*v) * c) <= s->capacity && "Out of bounds read");

	for (fckc_i32 *end = v + c; v != end; v++)
	{
		fckc_u8 *at = s->bytes + s->at;
		*v = ((fckc_i32)at[0] << 0ull)     // 0xFF00000000000000
		     | ((fckc_i32)at[1] << 8ull)   // 0x00FF000000000000
		     | ((fckc_i32)at[2] << 16ull)  // 0x0000FF0000000000
		     | ((fckc_i32)at[3] << 24ull); // 0x000000FF00000000
		s->at = s->at + sizeof(*v);
	}
}

void fck_read_byte_i64(fck_serialiser *s, fck_serialiser_params* p, fckc_i64 *v, fckc_size_t c)
{
	SDL_assert((s->at + sizeof(*v) * c) <= s->capacity && "Out of bounds read");

	for (fckc_i64 *end = v + c; v != end; v++)
	{
		fckc_u8 *at = s->bytes + s->at;
		*v = ((fckc_i64)at[0] << 0ull)     // 0xFF00000000000000
		     | ((fckc_i64)at[1] << 8ull)   // 0x00FF000000000000
		     | ((fckc_i64)at[2] << 16ull)  // 0x0000FF0000000000
		     | ((fckc_i64)at[3] << 24ull)  // 0x000000FF00000000
		     | ((fckc_i64)at[4] << 32ull)  // 0x00000000FF000000
		     | ((fckc_i64)at[5] << 40ull)  // 0x0000000000FF0000
		     | ((fckc_i64)at[6] << 48ull)  // 0x000000000000FF00
		     | ((fckc_i64)at[7] << 56ull); // 0x00000000000000FF
		s->at = s->at + sizeof(*v);
	}
}

void fck_read_byte_u8(fck_serialiser *s, fck_serialiser_params* p, fckc_u8 *v, fckc_size_t c)
{
	SDL_assert((s->at + sizeof(*v) * c) <= s->capacity && "Out of bounds read");

	SDL_memcpy(v, s->bytes + s->at, c);
	s->at = s->at + sizeof(*v);
}

void fck_read_byte_u16(fck_serialiser *s, fck_serialiser_params* p, fckc_u16 *v, fckc_size_t c)
{
	SDL_assert((s->at + sizeof(*v) * c) <= s->capacity && "Out of bounds read");

	for (fckc_u16 *end = v + c; v != end; v++)
	{
		fckc_u8 *at = s->bytes + s->at;
		*v = ((fckc_u16)at[0] << 0ull)    // 0xFF00000000000000
		     | ((fckc_u16)at[1] << 8ull); // 0x00FF000000000000
		s->at = s->at + sizeof(*v);
	}
}

void fck_read_byte_u32(fck_serialiser *s, fck_serialiser_params* p, fckc_u32 *v, fckc_size_t c)
{
	SDL_assert((s->at + sizeof(*v) * c) <= s->capacity && "Out of bounds read");

	for (fckc_u32 *end = v + c; v != end; v++)
	{
		fckc_u8 *at = s->bytes + s->at;
		*v = ((fckc_u32)at[0] << 0ull)     // 0xFF00000000000000
		     | ((fckc_u32)at[1] << 8ull)   // 0x00FF000000000000
		     | ((fckc_u32)at[2] << 16ull)  // 0x0000FF0000000000
		     | ((fckc_u32)at[3] << 24ull); // 0x000000FF00000000
		s->at = s->at + sizeof(*v);
	}
}

void fck_read_byte_u64(fck_serialiser *s, fck_serialiser_params* p, fckc_u64 *v, fckc_size_t c)
{
	SDL_assert((s->at + sizeof(*v) * c) <= s->capacity && "Out of bounds read");

	for (fckc_u64 *end = v + c; v != end; v++)
	{
		fckc_u8 *at = s->bytes + s->at;
		*v = ((fckc_u64)at[0] << 0ull)     // 0xFF00000000000000
		     | ((fckc_u64)at[1] << 8ull)   // 0x00FF000000000000
		     | ((fckc_u64)at[2] << 16ull)  // 0x0000FF0000000000
		     | ((fckc_u64)at[3] << 24ull)  // 0x000000FF00000000
		     | ((fckc_u64)at[4] << 32ull)  // 0x00000000FF000000
		     | ((fckc_u64)at[5] << 40ull)  // 0x0000000000FF0000
		     | ((fckc_u64)at[6] << 48ull)  // 0x000000000000FF00
		     | ((fckc_u64)at[7] << 56ull); // 0x00000000000000FF
		s->at = s->at + sizeof(*v);
	}
}

void fck_read_byte_f32(fck_serialiser *s, fck_serialiser_params* p, float *v, fckc_size_t c)
{
	SDL_assert((s->at + sizeof(*v) * c) <= s->capacity && "Out of bounds read");

	for (float *end = v + c; v != end; v++)
	{
		fckc_u8 *at = s->bytes + s->at;
		fckc_u32 as_int = ((fckc_u32)at[0] << 0ull)     // 0xFF00000000000000
		                  | ((fckc_u32)at[1] << 8ull)   // 0x00FF000000000000
		                  | ((fckc_u32)at[2] << 16ull)  // 0x0000FF0000000000
		                  | ((fckc_u32)at[3] << 24ull); // 0x000000FF00000000
		SDL_memcpy(v, &as_int, SDL_min(sizeof(as_int), sizeof(*v)));
		s->at = s->at + sizeof(*v);
	}
}

void fck_read_byte_f64(fck_serialiser *s, fck_serialiser_params* p, double *v, fckc_size_t c)
{
	SDL_assert((s->at + sizeof(*v) * c) <= s->capacity && "Out of bounds read");

	for (double *end = v + c; v != end; v++)
	{
		fckc_u8 *at = s->bytes + s->at;
		fckc_u64 as_int = ((fckc_u64)at[0] << 0ull)     // 0xFF00000000000000
		                  | ((fckc_u64)at[1] << 8ull)   // 0x00FF000000000000
		                  | ((fckc_u64)at[2] << 16ull)  // 0x0000FF0000000000
		                  | ((fckc_u64)at[3] << 24ull)  // 0x000000FF00000000
		                  | ((fckc_u64)at[4] << 32ull)  // 0x00000000FF000000
		                  | ((fckc_u64)at[5] << 40ull)  // 0x0000000000FF0000
		                  | ((fckc_u64)at[6] << 48ull)  // 0x000000000000FF00
		                  | ((fckc_u64)at[7] << 56ull); // 0x00000000000000FF
		SDL_memcpy(v, &as_int, SDL_min(sizeof(as_int), sizeof(*v)));
		s->at = s->at + sizeof(*v);
	}
}

void fck_read_byte_string(fck_serialiser *s, fck_serialiser_params* p, fck_lstring *v, fckc_size_t c)
{
	SDL_assert(false && "NOT SUPPORTED FOR NOW");
}

void fck_write_byte_i8(fck_serialiser *s, fck_serialiser_params* p, fckc_i8 *v, fckc_size_t c)
{
	fck_serialiser_maybe_realloc(s, sizeof(*v) * c);

	SDL_memcpy(s->bytes + s->at, v, c);
	s->at = s->at + sizeof(*v);
}

void fck_write_byte_i16(fck_serialiser *s, fck_serialiser_params* p, fckc_i16 *v, fckc_size_t c)
{
	fck_serialiser_maybe_realloc(s, sizeof(*v) * c);

	for (fckc_i16 *end = v + c; v != end; v++)
	{
		fckc_u8 *at = s->bytes + s->at;
		at[0] = (uint8_t)((*v >> 0) & 0xFF);
		at[1] = (uint8_t)((*v >> 8) & 0xFF);
		s->at = s->at + sizeof(*v);
	}
}

void fck_write_byte_i32(fck_serialiser *s, fck_serialiser_params* p, fckc_i32 *v, fckc_size_t c)
{
	fck_serialiser_maybe_realloc(s, sizeof(*v) * c);

	for (fckc_i32 *end = v + c; v != end; v++)
	{
		fckc_u8 *at = s->bytes + s->at;
		at[0] = (uint8_t)((*v >> 0) & 0xFF);
		at[1] = (uint8_t)((*v >> 8) & 0xFF);
		at[2] = (uint8_t)((*v >> 16) & 0xFF);
		at[3] = (uint8_t)((*v >> 24) & 0xFF);
		s->at = s->at + sizeof(*v);
	}
}

void fck_write_byte_i64(fck_serialiser *s, fck_serialiser_params* p, fckc_i64 *v, fckc_size_t c)
{
	fck_serialiser_maybe_realloc(s, sizeof(*v) * c);

	for (fckc_i64 *end = v + c; v != end; v++)
	{
		fckc_u8 *at = s->bytes + s->at;
		at[0] = (uint8_t)((*v >> 0) & 0xFF);
		at[1] = (uint8_t)((*v >> 8) & 0xFF);
		at[2] = (uint8_t)((*v >> 16) & 0xFF);
		at[3] = (uint8_t)((*v >> 24) & 0xFF);
		at[4] = (uint8_t)((*v >> 32) & 0xFF);
		at[5] = (uint8_t)((*v >> 40) & 0xFF);
		at[6] = (uint8_t)((*v >> 48) & 0xFF);
		at[7] = (uint8_t)((*v >> 56) & 0xFF);
		s->at = s->at + sizeof(*v);
	}
}

void fck_write_byte_u8(fck_serialiser *s, fck_serialiser_params* p, fckc_u8 *v, fckc_size_t c)
{
	fck_serialiser_maybe_realloc(s, sizeof(*v) * c);

	SDL_memcpy(s->bytes + s->at, v, c);
	s->at = s->at + sizeof(*v);
}

void fck_write_byte_u16(fck_serialiser *s, fck_serialiser_params* p, fckc_u16 *v, fckc_size_t c)
{
	fck_serialiser_maybe_realloc(s, sizeof(*v) * c);

	for (fckc_u16 *end = v + c; v != end; v++)
	{
		fckc_u8 *at = s->bytes + s->at;
		at[0] = (uint8_t)((*v >> 0) & 0xFF);
		at[1] = (uint8_t)((*v >> 8) & 0xFF);
		s->at = s->at + sizeof(*v);
	}
}

void fck_write_byte_u32(fck_serialiser *s, fck_serialiser_params* p, fckc_u32 *v, fckc_size_t c)
{
	fck_serialiser_maybe_realloc(s, sizeof(*v) * c);

	for (fckc_u32 *end = v + c; v != end; v++)
	{
		fckc_u8 *at = s->bytes + s->at;
		at[0] = (uint8_t)((*v >> 0) & 0xFF);
		at[1] = (uint8_t)((*v >> 8) & 0xFF);
		at[2] = (uint8_t)((*v >> 16) & 0xFF);
		at[3] = (uint8_t)((*v >> 24) & 0xFF);
		s->at = s->at + sizeof(*v);
	}
}

void fck_write_byte_u64(fck_serialiser *s, fck_serialiser_params* p, fckc_u64 *v, fckc_size_t c)
{
	fck_serialiser_maybe_realloc(s, sizeof(*v) * c);

	for (fckc_u64 *end = v + c; v != end; v++)
	{
		fckc_u8 *at = s->bytes + s->at;
		at[0] = (uint8_t)((*v >> 0) & 0xFF);
		at[1] = (uint8_t)((*v >> 8) & 0xFF);
		at[2] = (uint8_t)((*v >> 16) & 0xFF);
		at[3] = (uint8_t)((*v >> 24) & 0xFF);
		at[4] = (uint8_t)((*v >> 32) & 0xFF);
		at[5] = (uint8_t)((*v >> 40) & 0xFF);
		at[6] = (uint8_t)((*v >> 48) & 0xFF);
		at[7] = (uint8_t)((*v >> 56) & 0xFF);
		s->at = s->at + sizeof(*v);
	}
}

void fck_write_byte_f32(fck_serialiser *s, fck_serialiser_params* p, float *v, fckc_size_t c)
{
	fck_serialiser_maybe_realloc(s, sizeof(*v) * c);

	for (float *end = v + c; v != end; v++)
	{
		fckc_u8 *at = s->bytes + s->at;
		fckc_u32 as_int;
		SDL_memcpy(&as_int, v, SDL_min(sizeof(as_int), sizeof(*v)));

		at[0] = (uint8_t)((as_int >> 0) & 0xFF);
		at[1] = (uint8_t)((as_int >> 8) & 0xFF);
		at[2] = (uint8_t)((as_int >> 16) & 0xFF);
		at[3] = (uint8_t)((as_int >> 24) & 0xFF);
		s->at = s->at + sizeof(*v);
	}
}

void fck_write_byte_f64(fck_serialiser *s, fck_serialiser_params* p, double *v, fckc_size_t c)
{
	fck_serialiser_maybe_realloc(s, sizeof(*v) * c);

	for (double *end = v + c; v != end; v++)
	{
		fckc_u8 *at = s->bytes + s->at;
		fckc_u64 as_int;
		SDL_memcpy(&as_int, v, SDL_min(sizeof(as_int), sizeof(*v)));

		at[0] = (uint8_t)((as_int >> 0) & 0xFF);
		at[1] = (uint8_t)((as_int >> 8) & 0xFF);
		at[2] = (uint8_t)((as_int >> 16) & 0xFF);
		at[3] = (uint8_t)((as_int >> 24) & 0xFF);
		at[4] = (uint8_t)((as_int >> 32) & 0xFF);
		at[5] = (uint8_t)((as_int >> 40) & 0xFF);
		at[6] = (uint8_t)((as_int >> 48) & 0xFF);
		at[7] = (uint8_t)((as_int >> 56) & 0xFF);
		s->at = s->at + sizeof(*v);
	}
}

void fck_write_byte_string(fck_serialiser *s, fck_serialiser_params* p, fck_lstring *v, fckc_size_t c)
{
	SDL_assert(false && "NOT SUPPORTED FOR NOW");
}

static fck_serialiser_vt fck_serialiser_byte_writer_vt = {
	.i8 = fck_write_byte_i8,
	.i16 = fck_write_byte_i16,
	.i32 = fck_write_byte_i32,
	.i64 = fck_write_byte_i64,
	.u8 = fck_write_byte_u8,
	.u16 = fck_write_byte_u16,
	.u32 = fck_write_byte_u32,
	.u64 = fck_write_byte_u64,
	.f32 = fck_write_byte_f32,
	.f64 = fck_write_byte_f64,
	.string = fck_write_byte_string,
};

static fck_serialiser_vt fck_serialiser_byte_reader_vt = {
	.i8 = fck_read_byte_i8,
	.i16 = fck_read_byte_i16,
	.i32 = fck_read_byte_i32,
	.i64 = fck_read_byte_i64,
	.u8 = fck_read_byte_u8,
	.u16 = fck_read_byte_u16,
	.u32 = fck_read_byte_u32,
	.u64 = fck_read_byte_u64,
	.f32 = fck_read_byte_f32,
	.f64 = fck_read_byte_f64,
	.string = fck_read_byte_string,
};

fck_serialiser_vt *fck_byte_writer_vt = &fck_serialiser_byte_writer_vt;
fck_serialiser_vt *fck_byte_reader_vt = &fck_serialiser_byte_reader_vt;