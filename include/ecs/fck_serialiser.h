#ifndef FCK_SERIALISER_INCLUDED
#define FCK_SERIALISER_INCLUDED

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_stdinc.h>

/* TODO: Dynamic array allocation instead of what we have atm
struct cool_array
{
    size_t capacity;
    uint8_t data[0];
};

struct cool_array *cool_array_alloc(size_t capacity)
{
    struct cool_array *arr;

    arr = (struct cool_array *)SDL_malloc(sizeof(*arr) + capacity);
    arr->capacity = capacity;
    return arr;
}
This way we can always send the header information along...
*/

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
	fck_serialiser_interface self;

	size_t capacity;
	uint8_t *data;

	size_t at;
};

void fck_serialiser_alloc(fck_serialiser *serialiser);
void fck_serialiser_create(fck_serialiser *serialiser, uint8_t *data, size_t count);
void fck_serialiser_create_full(fck_serialiser* serialiser, uint8_t* data, size_t count);

void fck_serialiser_reset(fck_serialiser *serialiser);
void fck_serialiser_free(fck_serialiser *serialiser);

void fck_serialiser_byte_reader(fck_serialiser_interface *interface);
void fck_serialiser_byte_writer(fck_serialiser_interface *interface);

bool fck_serialiser_is_byte_reader(fck_serialiser *const serialiser);
bool fck_serialiser_is_byte_writer(fck_serialiser *const serialiser);

inline void fck_serialise(fck_serialiser *serialisers, uint8_t *value, size_t count = 1)
{
	SDL_assert(serialisers != nullptr);
	SDL_assert(serialisers->self.u8 != nullptr);

	serialisers->self.u8(serialisers, value, count);
}
inline void fck_serialise(fck_serialiser *serialisers, uint16_t *value, size_t count = 1)
{
	SDL_assert(serialisers != nullptr);
	SDL_assert(serialisers->self.u16 != nullptr);

	serialisers->self.u16(serialisers, value, count);
}
inline void fck_serialise(fck_serialiser *serialisers, uint32_t *value, size_t count = 1)
{
	SDL_assert(serialisers != nullptr);
	SDL_assert(serialisers->self.u32 != nullptr);

	serialisers->self.u32(serialisers, value, count);
}
inline void fck_serialise(fck_serialiser *serialisers, uint64_t *value, size_t count = 1)
{
	SDL_assert(serialisers != nullptr);
	SDL_assert(serialisers->self.u64 != nullptr);

	serialisers->self.u64(serialisers, value, count);
}
inline void fck_serialise(fck_serialiser *serialisers, int8_t *value, size_t count = 1)
{
	SDL_assert(serialisers != nullptr);
	SDL_assert(serialisers->self.i8 != nullptr);

	serialisers->self.i8(serialisers, value, count);
}
inline void fck_serialise(fck_serialiser *serialisers, int16_t *value, size_t count = 1)
{
	SDL_assert(serialisers != nullptr);
	SDL_assert(serialisers->self.i16 != nullptr);

	serialisers->self.i16(serialisers, value, count);
}
inline void fck_serialise(fck_serialiser *serialisers, int32_t *value, size_t count = 1)
{
	SDL_assert(serialisers != nullptr);
	SDL_assert(serialisers->self.i32 != nullptr);

	serialisers->self.i32(serialisers, value, count);
}
inline void fck_serialise(fck_serialiser *serialisers, int64_t *value, size_t count = 1)
{
	SDL_assert(serialisers != nullptr);
	SDL_assert(serialisers->self.i64 != nullptr);

	serialisers->self.i64(serialisers, value, count);
}
inline void fck_serialise(fck_serialiser *serialisers, float *value, size_t count = 1)
{
	SDL_assert(serialisers != nullptr);
	SDL_assert(serialisers->self.f32 != nullptr);

	serialisers->self.f32(serialisers, value, count);
}

// Hope that works :D Maybe count for serialisers? Idk
inline void fck_serialise(fck_serialiser *serialisers, fck_serialiser *other)
{
	SDL_assert(serialisers != nullptr);
	SDL_assert(serialisers->self.f32 != nullptr);

	serialisers->self.u8(serialisers, other->data, other->at);
}

#endif // FCK_SERIALISER_INCLUDED