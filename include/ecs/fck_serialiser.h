#ifndef FCK_SERIALISER_INCLUDED
#define FCK_SERIALISER_INCLUDED

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_stdinc.h>

struct fck_serialiser_interface
{
	template <typename value_type>
	using serialise_function = void (*)(struct fck_serialiser *, value_type *, size_t);

	serialise_function<int8_t> i8;
	serialise_function<int16_t> i16;
	serialise_function<int32_t> i32;
	serialise_function<int64_t> i64;

	serialise_function<uint8_t> u8;
	serialise_function<uint16_t> u16;
	serialise_function<uint32_t> u32;
	serialise_function<uint64_t> u64;

	serialise_function<float> f32;
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

inline void fck_serialise(fck_serialiser *serialisers, uint8_t *value, size_t count = 1)
{
	SDL_assert(serialisers != nullptr);
	SDL_assert(serialisers->interface.u8 != nullptr);

	serialisers->interface.u8(serialisers, value, count);
}
inline void fck_serialise(fck_serialiser *serialisers, uint16_t *value, size_t count = 1)
{
	SDL_assert(serialisers != nullptr);
	SDL_assert(serialisers->interface.u16 != nullptr);

	serialisers->interface.u16(serialisers, value, count);
}
inline void fck_serialise(fck_serialiser *serialisers, uint32_t *value, size_t count = 1)
{
	SDL_assert(serialisers != nullptr);
	SDL_assert(serialisers->interface.u32 != nullptr);

	serialisers->interface.u32(serialisers, value, count);
}
inline void fck_serialise(fck_serialiser *serialisers, uint64_t *value, size_t count = 1)
{
	SDL_assert(serialisers != nullptr);
	SDL_assert(serialisers->interface.u64 != nullptr);

	serialisers->interface.u64(serialisers, value, count);
}
inline void fck_serialise(fck_serialiser *serialisers, int8_t *value, size_t count = 1)
{
	SDL_assert(serialisers != nullptr);
	SDL_assert(serialisers->interface.i8 != nullptr);

	serialisers->interface.i8(serialisers, value, count);
}
inline void fck_serialise(fck_serialiser *serialisers, int16_t *value, size_t count = 1)
{
	SDL_assert(serialisers != nullptr);
	SDL_assert(serialisers->interface.i16 != nullptr);

	serialisers->interface.i16(serialisers, value, count);
}
inline void fck_serialise(fck_serialiser *serialisers, int32_t *value, size_t count = 1)
{
	SDL_assert(serialisers != nullptr);
	SDL_assert(serialisers->interface.i32 != nullptr);

	serialisers->interface.i32(serialisers, value, count);
}
inline void fck_serialise(fck_serialiser *serialisers, int64_t *value, size_t count = 1)
{
	SDL_assert(serialisers != nullptr);
	SDL_assert(serialisers->interface.i64 != nullptr);

	serialisers->interface.i64(serialisers, value, count);
}
inline void fck_serialise(fck_serialiser *serialisers, float *value, size_t count = 1)
{
	SDL_assert(serialisers != nullptr);
	SDL_assert(serialisers->interface.f32 != nullptr);

	serialisers->interface.f32(serialisers, value, count);
}

#endif // FCK_SERIALISER_INCLUDED