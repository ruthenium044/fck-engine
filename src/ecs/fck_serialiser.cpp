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
	serialiser->count = 0;
	serialiser->index = 0;
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
	while (serialiser->capacity < serialiser->count + slack_count)
	{
		const size_t new_capacity = serialiser->capacity * 2;
		uint8_t *old_mem = serialiser->data;
		uint8_t *new_mem = (uint8_t *)SDL_realloc(serialiser->data, new_capacity);

		if (new_mem != nullptr)
		{
			SDL_LogCritical(0, "Failed to reallocate memory!");
			serialiser->data = new_mem;
		}

		serialiser->capacity = new_capacity;
	}
}

static void byte_read_uint8(fck_serialiser *serialiser, uint8_t *value)
{
	uint8_t *at = serialiser->data + serialiser->index;

	value[0] = *at;
	serialiser->index = serialiser->index + sizeof(*value);
}
static void byte_read_uint16(fck_serialiser *serialiser, uint16_t *value)
{
	uint8_t *at = serialiser->data + serialiser->index;

	*value = ((uint16_t)at[0] << 0)    // 0xFF000000
	         | ((uint16_t)at[1] << 8); // 0x00FF0000

	serialiser->index = serialiser->index + sizeof(*value);
}
static void byte_read_uint32(fck_serialiser *serialiser, uint32_t *value)
{
	uint8_t *at = serialiser->data + serialiser->index;

	*value = ((uint32_t)at[0] << 0)     // 0xFF000000
	         | ((uint32_t)at[1] << 8)   // 0x00FF0000
	         | ((uint32_t)at[2] << 16)  // 0x0000FF00
	         | ((uint32_t)at[3] << 24); // 0x000000FF

	serialiser->index = serialiser->index + sizeof(*value);
}
static void byte_read_int8(fck_serialiser *serialiser, int8_t *value)
{
	uint8_t *at = serialiser->data + serialiser->index;

	value[0] = *at;
	serialiser->index = serialiser->index + sizeof(*value);
}
static void byte_read_int16(fck_serialiser *serialiser, int16_t *value)
{
	uint8_t *at = serialiser->data + serialiser->index;

	*value = ((uint16_t)at[0] << 0)    // 0xFF000000
	         | ((uint16_t)at[1] << 8); // 0x00FF0000

	serialiser->index = serialiser->index + sizeof(*value);
}
static void byte_read_int32(fck_serialiser *serialiser, int32_t *value)
{
	uint8_t *at = serialiser->data + serialiser->index;

	*value = ((uint32_t)at[0] << 0)     // 0xFF000000
	         | ((uint32_t)at[1] << 8)   // 0x00FF0000
	         | ((uint32_t)at[2] << 16)  // 0x0000FF00
	         | ((uint32_t)at[3] << 24); // 0x000000FF

	serialiser->index = serialiser->index + sizeof(*value);
}
static void byte_read_float32(fck_serialiser *serialiser, float *value)
{
	union {
		float f;
		uint32_t u;
	} data = {};
	byte_read_uint32(serialiser, &data.u);

	*value = data.f;
}

static void byte_write_uint8(fck_serialiser *serialiser, uint8_t *value)
{
	fck_serialiser_maybe_realloc(serialiser, sizeof(*value));

	uint8_t *at = serialiser->data + serialiser->count;

	*at = value[0];
	serialiser->count = serialiser->count + sizeof(*value);
}
static void byte_write_uint16(fck_serialiser *serialiser, uint16_t *value)
{
	fck_serialiser_maybe_realloc(serialiser, sizeof(*value));

	uint8_t *at = serialiser->data + serialiser->count;

	at[0] = (uint8_t)((*value >> 0) & 0xFF);
	at[1] = (uint8_t)((*value >> 8) & 0xFF);

	serialiser->count = serialiser->count + sizeof(*value);
}
static void byte_write_uint32(fck_serialiser *serialiser, uint32_t *value)
{
	fck_serialiser_maybe_realloc(serialiser, sizeof(*value));

	uint8_t *at = serialiser->data + serialiser->count;

	at[0] = (uint8_t)((*value >> 0) & 0xFF);
	at[1] = (uint8_t)((*value >> 8) & 0xFF);
	at[2] = (uint8_t)((*value >> 16) & 0xFF);
	at[3] = (uint8_t)((*value >> 24) & 0xFF);

	serialiser->count = serialiser->count + sizeof(*value);
}
static void byte_write_int8(fck_serialiser *serialiser, int8_t *value)
{
	fck_serialiser_maybe_realloc(serialiser, sizeof(*value));

	uint8_t *at = serialiser->data + serialiser->count;

	*at = value[0];
	serialiser->count = serialiser->count + sizeof(*value);
}
static void byte_write_int16(fck_serialiser *serialiser, int16_t *value)
{
	fck_serialiser_maybe_realloc(serialiser, sizeof(*value));

	uint8_t *at = serialiser->data + serialiser->count;

	at[0] = (uint8_t)((*value >> 0) & 0xFF);
	at[1] = (uint8_t)((*value >> 8) & 0xFF);

	serialiser->count = serialiser->count + sizeof(*value);
}
static void byte_write_int32(fck_serialiser *serialiser, int32_t *value)
{
	fck_serialiser_maybe_realloc(serialiser, sizeof(*value));

	uint8_t *at = serialiser->data + serialiser->count;

	at[0] = (uint8_t)((*value >> 0) & 0xFF);
	at[1] = (uint8_t)((*value >> 8) & 0xFF);
	at[2] = (uint8_t)((*value >> 16) & 0xFF);
	at[3] = (uint8_t)((*value >> 24) & 0xFF);

	serialiser->count = serialiser->count + sizeof(*value);
}
static void byte_write_float32(fck_serialiser *serialiser, float *value)
{
	union {
		float f;
		uint32_t u;
	} data = {.f = *value};
	byte_write_uint32(serialiser, &data.u);
}

void fck_serialiser_byte_reader(fck_serialiser_interface *interface)
{
	SDL_assert(interface != nullptr);
	interface->u8 = byte_read_uint8;
	interface->u16 = byte_read_uint16;
	interface->u32 = byte_read_uint32;

	interface->i8 = byte_read_int8;
	interface->i16 = byte_read_int16;
	interface->i32 = byte_read_int32;

	interface->f32 = byte_read_float32;
}

void fck_serialiser_byte_writer(fck_serialiser_interface *interface)
{
	SDL_assert(interface != nullptr);
	interface->u8 = byte_write_uint8;
	interface->u16 = byte_write_uint16;
	interface->u32 = byte_write_uint32;

	interface->i8 = byte_write_int8;
	interface->i16 = byte_write_int16;
	interface->i32 = byte_write_int32;

	interface->f32 = byte_write_float32;
}