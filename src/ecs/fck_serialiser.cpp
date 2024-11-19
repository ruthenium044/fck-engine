#include "ecs/fck_serialiser.h"

#include "fck_checks.h"
#include <SDL3/SDL_assert.h>

void fck_serialiser_alloc(fck_serialiser *serialiser)
{
	SDL_assert(serialiser != nullptr);

	SDL_zerop(serialiser);

	constexpr size_t dafault_capacity = 1024;
	serialiser->data = (uint8_t *)SDL_malloc(dafault_capacity);
	serialiser->capacity = dafault_capacity;
	serialiser->at = 0;
}

void fck_serialiser_create(fck_serialiser *serialiser, uint8_t *data, size_t count)
{
	SDL_assert(serialiser != nullptr);

	SDL_zerop(serialiser);

	serialiser->data = data;
	serialiser->capacity = count;
	serialiser->at = 0;
}

void fck_serialiser_create(fck_serialiser *serialiser, uint8_t *data, size_t count, size_t capacity)
{
	SDL_assert(serialiser != nullptr);

	SDL_zerop(serialiser);

	serialiser->data = data;
	serialiser->capacity = capacity;
	serialiser->at = 0;
}

void fck_serialiser_reset(fck_serialiser *serialiser)
{
	SDL_assert(serialiser != nullptr);

	serialiser->at = 0;
}

void fck_serialiser_free(fck_serialiser *serialiser)
{
	SDL_assert(serialiser != nullptr);
	SDL_free(serialiser->data);
	SDL_zerop(serialiser);
}

void fck_serialiser_maybe_realloc(fck_serialiser *serialiser, size_t slack_count)
{
	SDL_assert(serialiser != nullptr);

	// Fix it with a smart calculation :D
	// Just lazy at the moment
	while (serialiser->capacity < serialiser->at + slack_count)
	{
		const size_t new_capacity = serialiser->capacity * 2;
		uint8_t *old_mem = serialiser->data;
		uint8_t *new_mem = (uint8_t *)SDL_realloc(serialiser->data, new_capacity);

		if (new_mem != nullptr)
		{
			serialiser->data = new_mem;
		}
		else
		{
			SDL_LogCritical(0, "Failed to reallocate memory!");
		}

		serialiser->capacity = new_capacity;
	}
}

static void byte_read_uint8(fck_serialiser *serialiser, uint8_t *value, size_t count)
{
	for (int i = 0; i < count; i++)
	{
		uint8_t *at = serialiser->data + serialiser->at;

		value[i] = *at;
		serialiser->at = serialiser->at + sizeof(*value);
	}
}
static void byte_read_uint16(fck_serialiser *serialiser, uint16_t *value, size_t count)
{
	for (int i = 0; i < count; i++)
	{
		uint8_t *at = serialiser->data + serialiser->at;

		value[i] = ((uint16_t)at[0] << 0)    // 0xFF000000
		           | ((uint16_t)at[1] << 8); // 0x00FF0000

		serialiser->at = serialiser->at + sizeof(*value);
	}
}
static void byte_read_uint32(fck_serialiser *serialiser, uint32_t *value, size_t count)
{
	for (int i = 0; i < count; i++)
	{
		uint8_t *at = serialiser->data + serialiser->at;

		value[i] = (uint32_t(at[0]) << 0)     // 0xFF000000
		           | (uint32_t(at[1]) << 8)   // 0x00FF0000
		           | (uint32_t(at[2]) << 16)  // 0x0000FF00
		           | (uint32_t(at[3]) << 24); // 0x000000FF

		serialiser->at = serialiser->at + sizeof(*value);
	}
}
static void byte_read_uint64(fck_serialiser *serialiser, uint64_t *value, size_t count)
{
	for (int i = 0; i < count; i++)
	{
		uint8_t *at = serialiser->data + serialiser->at;

		value[i] = ((uint64_t)at[0] << 0ull)     // 0xFF00000000000000
		           | ((uint64_t)at[1] << 8ull)   // 0x00FF000000000000
		           | ((uint64_t)at[2] << 16ull)  // 0x0000FF0000000000
		           | ((uint64_t)at[3] << 24ull)  // 0x000000FF00000000
		           | ((uint64_t)at[4] << 32ull)  // 0x00000000FF000000
		           | ((uint64_t)at[5] << 40ull)  // 0x0000000000FF0000
		           | ((uint64_t)at[6] << 48ull)  // 0x000000000000FF00
		           | ((uint64_t)at[7] << 56ull); // 0x00000000000000FF

		serialiser->at = serialiser->at + sizeof(*value);
	}
}
static void byte_read_int8(fck_serialiser *serialiser, int8_t *value, size_t count)
{
	for (int i = 0; i < count; i++)
	{
		uint8_t *at = serialiser->data + serialiser->at;

		value[i] = *at;
		serialiser->at = serialiser->at + sizeof(*value);
	}
}
static void byte_read_int16(fck_serialiser *serialiser, int16_t *value, size_t count)
{
	for (int i = 0; i < count; i++)
	{
		uint8_t *at = serialiser->data + serialiser->at;

		value[i] = ((uint16_t)at[0] << 0)    // 0xFF000000
		           | ((uint16_t)at[1] << 8); // 0x00FF0000

		serialiser->at = serialiser->at + sizeof(*value);
	}
}
static void byte_read_int32(fck_serialiser *serialiser, int32_t *value, size_t count)
{
	for (int i = 0; i < count; i++)
	{
		uint8_t *at = serialiser->data + serialiser->at;

		value[i] = ((uint32_t)at[0] << 0)     // 0xFF000000
		           | ((uint32_t)at[1] << 8)   // 0x00FF0000
		           | ((uint32_t)at[2] << 16)  // 0x0000FF00
		           | ((uint32_t)at[3] << 24); // 0x000000FF

		serialiser->at = serialiser->at + sizeof(*value);
	}
}
static void byte_read_int64(fck_serialiser *serialiser, int64_t *value, size_t count)
{
	for (int i = 0; i < count; i++)
	{
		uint8_t *at = serialiser->data + serialiser->at;

		value[i] = ((int64_t)at[0] << 0ull)     // 0xFF00000000000000
		           | ((int64_t)at[1] << 8ull)   // 0x00FF000000000000
		           | ((int64_t)at[2] << 16ull)  // 0x0000FF0000000000
		           | ((int64_t)at[3] << 24ull)  // 0x000000FF00000000
		           | ((int64_t)at[4] << 32ull)  // 0x00000000FF000000
		           | ((int64_t)at[5] << 40ull)  // 0x0000000000FF0000
		           | ((int64_t)at[6] << 48ull)  // 0x000000000000FF00
		           | ((int64_t)at[7] << 56ull); // 0x00000000000000FF

		serialiser->at = serialiser->at + sizeof(*value);
	}
}
static void byte_read_float32(fck_serialiser *serialiser, float *value, size_t count)
{
	byte_read_uint32(serialiser, (uint32_t *)value, count);
}

static void byte_write_uint8(fck_serialiser *serialiser, uint8_t *value, size_t count)
{
	for (int i = 0; i < count; i++)
	{
		fck_serialiser_maybe_realloc(serialiser, sizeof(*value));

		uint8_t *at = serialiser->data + serialiser->at;

		*at = value[i];
		serialiser->at = serialiser->at + sizeof(*value);
	}
}
static void byte_write_uint16(fck_serialiser *serialiser, uint16_t *value, size_t count)
{
	for (int i = 0; i < count; i++)
	{
		fck_serialiser_maybe_realloc(serialiser, sizeof(*value));

		uint8_t *at = serialiser->data + serialiser->at;

		at[0] = (uint8_t)((value[i] >> 0) & 0xFF);
		at[1] = (uint8_t)((value[i] >> 8) & 0xFF);

		serialiser->at = serialiser->at + sizeof(*value);
	}
}
static void byte_write_uint32(fck_serialiser *serialiser, uint32_t *value, size_t count)
{
	for (int i = 0; i < count; i++)
	{
		fck_serialiser_maybe_realloc(serialiser, sizeof(*value));

		uint8_t *at = serialiser->data + serialiser->at;

		at[0] = (uint8_t)((value[i] >> 0) & 0xFF);
		at[1] = (uint8_t)((value[i] >> 8) & 0xFF);
		at[2] = (uint8_t)((value[i] >> 16) & 0xFF);
		at[3] = (uint8_t)((value[i] >> 24) & 0xFF);

		serialiser->at = serialiser->at + sizeof(*value);
	}
}
static void byte_write_uint64(fck_serialiser *serialiser, uint64_t *value, size_t count)
{
	for (int i = 0; i < count; i++)
	{
		fck_serialiser_maybe_realloc(serialiser, sizeof(*value));

		uint8_t *at = serialiser->data + serialiser->at;

		at[0] = (uint8_t)((value[i] >> 0) & 0xFF);
		at[1] = (uint8_t)((value[i] >> 8) & 0xFF);
		at[2] = (uint8_t)((value[i] >> 16) & 0xFF);
		at[3] = (uint8_t)((value[i] >> 24) & 0xFF);
		at[4] = (uint8_t)((value[i] >> 32) & 0xFF);
		at[5] = (uint8_t)((value[i] >> 40) & 0xFF);
		at[6] = (uint8_t)((value[i] >> 48) & 0xFF);
		at[7] = (uint8_t)((value[i] >> 56) & 0xFF);

		serialiser->at = serialiser->at + sizeof(*value);
	}
}
static void byte_write_int8(fck_serialiser *serialiser, int8_t *value, size_t count)
{
	for (int i = 0; i < count; i++)
	{
		fck_serialiser_maybe_realloc(serialiser, sizeof(*value));

		uint8_t *at = serialiser->data + serialiser->at;

		*at = value[i];
		serialiser->at = serialiser->at + sizeof(*value);
	}
}
static void byte_write_int16(fck_serialiser *serialiser, int16_t *value, size_t count)
{
	for (int i = 0; i < count; i++)
	{
		fck_serialiser_maybe_realloc(serialiser, sizeof(*value));

		uint8_t *at = serialiser->data + serialiser->at;

		at[0] = (uint8_t)((value[i] >> 0) & 0xFF);
		at[1] = (uint8_t)((value[i] >> 8) & 0xFF);

		serialiser->at = serialiser->at + sizeof(*value);
	}
}
static void byte_write_int32(fck_serialiser *serialiser, int32_t *value, size_t count)
{
	for (int i = 0; i < count; i++)
	{
		fck_serialiser_maybe_realloc(serialiser, sizeof(*value));

		uint8_t *at = serialiser->data + serialiser->at;

		at[0] = (uint8_t)((value[i] >> 0) & 0xFF);
		at[1] = (uint8_t)((value[i] >> 8) & 0xFF);
		at[2] = (uint8_t)((value[i] >> 16) & 0xFF);
		at[3] = (uint8_t)((value[i] >> 24) & 0xFF);

		serialiser->at = serialiser->at + sizeof(*value);
	}
}
static void byte_write_int64(fck_serialiser *serialiser, int64_t *value, size_t count)
{
	for (int i = 0; i < count; i++)
	{
		fck_serialiser_maybe_realloc(serialiser, sizeof(*value));

		uint8_t *at = serialiser->data + serialiser->at;

		at[0] = (uint8_t)((value[i] >> 0) & 0xFF);
		at[1] = (uint8_t)((value[i] >> 8) & 0xFF);
		at[2] = (uint8_t)((value[i] >> 16) & 0xFF);
		at[3] = (uint8_t)((value[i] >> 24) & 0xFF);
		at[4] = (uint8_t)((value[i] >> 32) & 0xFF);
		at[5] = (uint8_t)((value[i] >> 40) & 0xFF);
		at[6] = (uint8_t)((value[i] >> 48) & 0xFF);
		at[7] = (uint8_t)((value[i] >> 56) & 0xFF);

		serialiser->at = serialiser->at + sizeof(*value);
	}
}
static void byte_write_float32(fck_serialiser *serialiser, float *value, size_t count)
{
	byte_write_uint32(serialiser, (uint32_t *)value, count);
}

void fck_serialiser_byte_reader(fck_serialiser_interface *interface)
{
	SDL_assert(interface != nullptr);

	interface->u8 = byte_read_uint8;
	interface->u16 = byte_read_uint16;
	interface->u32 = byte_read_uint32;
	interface->u64 = byte_read_uint64;

	interface->i8 = byte_read_int8;
	interface->i16 = byte_read_int16;
	interface->i32 = byte_read_int32;
	interface->i64 = byte_read_int64;

	interface->f32 = byte_read_float32;
}

void fck_serialiser_byte_writer(fck_serialiser_interface *interface)
{
	SDL_assert(interface != nullptr);

	interface->u8 = byte_write_uint8;
	interface->u16 = byte_write_uint16;
	interface->u32 = byte_write_uint32;
	interface->u64 = byte_write_uint64;

	interface->i8 = byte_write_int8;
	interface->i16 = byte_write_int16;
	interface->i32 = byte_write_int32;
	interface->i64 = byte_write_int64;

	interface->f32 = byte_write_float32;
}