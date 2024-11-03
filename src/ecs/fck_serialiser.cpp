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