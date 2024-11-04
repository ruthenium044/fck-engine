#ifndef FCK_SERIALISER_INCLUDED
#define FCK_SERIALISER_INCLUDED

#include <SDL3/SDL_stdinc.h>

struct fck_serialiser_interface
{
	template <typename value_type>
	using serialse_function = void (*)(struct fck_serialiser *, value_type *);

	serialse_function<int8_t> i8;
	serialse_function<int16_t> i16;
	serialse_function<int32_t> i32;
	serialse_function<uint8_t> u8;
	serialse_function<uint16_t> u16;
	serialse_function<uint32_t> u32;
	serialse_function<float> f32;
};

struct fck_serialiser
{
	fck_serialiser_interface interface;

	uint8_t *data;
	size_t capacity;

	size_t count; // Write
	size_t index; // Read
};

void fck_serialiser_alloc(fck_serialiser *serialiser);
void fck_serialiser_free(fck_serialiser *serialiser);
void fck_serialiser_maybe_realloc(fck_serialiser *serialiser, size_t slack_count);

void fck_serialiser_byte_reader(fck_serialiser_interface *interface);
void fck_serialiser_byte_writer(fck_serialiser_interface *interface);

#endif // FCK_SERIALISER_INCLUDED