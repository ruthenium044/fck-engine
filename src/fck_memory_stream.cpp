#include "fck_memory_stream.h"
#include <fck_checks.h>
#include <SDL3/SDL_error.h>

bool fck_memory_stream_write(fck_memory_stream *stream, void *data, size_t size)
{
	while (stream->count + size >= stream->capacity)
	{
		size_t new_capacity = stream->capacity == 0 ? 64 : stream->capacity + stream->count + size;
		void *new_data = SDL_realloc(stream->data, new_capacity);

		CHECK_CRITICAL(new_data, SDL_GetError(), return false);

		stream->capacity = new_capacity;
		stream->data = (uint8_t *)new_data;
	}

	SDL_memcpy(stream->data + stream->count, data, size);

	stream->count = stream->count + size;
	return true;
}

uint8_t *fck_memory_stream_read(fck_memory_stream *stream, size_t size)
{
	if (stream->count >= stream->capacity)
	{
		return nullptr;
	}
	size_t current = stream->count;
	stream->count = stream->count + size;
	return stream->data + current;
}

void fck_memory_stream_close(fck_memory_stream *stream)
{
	SDL_free(stream->data);
}
