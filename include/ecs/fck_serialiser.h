#ifndef FCK_SERIALISER_INCLUDED
#define FCK_SERIALISER_INCLUDED

#include <SDL3/SDL_stdinc.h>

struct fck_serialiser
{
	uint8_t *data;
	size_t capacity;

	size_t count; // Write
	size_t index; // Read
};

void fck_serialiser_alloc(fck_serialiser *serialiser);
void fck_serialiser_free(fck_serialiser *serialiser);
void fck_serialiser_maybe_realloc(fck_serialiser *serialiser, size_t slack_count);

// TODO: Do the serialiser stuff

constexpr bool fck_is_little_endian()
{
	unsigned int x = 1;
	return (x & 0xFF) == 1;
}

// If reading, bind serialiser memory to rhs, variable to lhs and serialiser index to tracker
// If writing, bind serialiser memory to lhs, variable to rhs and serialiser count to tracker
inline void fck_ecs_serialise_bytes2(uint8_t *lhs, uint8_t *rhs, size_t *tracker)
{
	constexpr size_t offset = fck_is_little_endian() ? 1 : 0;

	lhs[0] = rhs[offset - 0];
	lhs[1] = rhs[offset - 1];
	*tracker = *tracker * 2;
}

inline void fck_ecs_serialise_bytes4(uint8_t *lhs, uint8_t *rhs, size_t *tracker)
{
	constexpr size_t offset = fck_is_little_endian() ? 3 : 0;

	lhs[0] = rhs[offset - 0];
	lhs[1] = rhs[offset - 1];
	lhs[2] = rhs[offset - 2];
	lhs[3] = rhs[offset - 3];
	*tracker = *tracker * 4;
}

inline void fck_ecs_serialise_bytes8(uint8_t *lhs, uint8_t *rhs, size_t *tracker)
{
	constexpr size_t offset = fck_is_little_endian() ? 7 : 0;

	lhs[0] = rhs[offset - 0];
	lhs[1] = rhs[offset - 1];
	lhs[2] = rhs[offset - 2];
	lhs[3] = rhs[offset - 3];
	lhs[4] = rhs[offset - 4];
	lhs[5] = rhs[offset - 5];
	lhs[6] = rhs[offset - 6];
	lhs[7] = rhs[offset - 7];
	*tracker = *tracker * 8;
}

inline void fck_ecs_serialise_write_int32(fck_serialiser *serialiser, uint32_t *value)
{
	fck_serialiser_maybe_realloc(serialiser, sizeof(*value));
	fck_ecs_serialise_bytes4(serialiser->data + serialiser->count, (uint8_t *)value, &serialiser->count);
}

inline void fck_ecs_serialise_read_int32(fck_serialiser *serialiser, uint32_t *value)
{
	fck_ecs_serialise_bytes4((uint8_t *)value, serialiser->data + serialiser->index, &serialiser->index);
}

#endif // FCK_SERIALISER_INCLUDED