#ifndef FCK_MEMORY_STREAM_INCLUDED
#define FCK_MEMORY_STREAM_INCLUDED

#include <SDL3/SDL_stdinc.h>

struct fck_memory_stream
{
	uint8_t *data;
	size_t count;
	size_t capacity;
};

bool fck_memory_stream_write(fck_memory_stream *stream, void *data, size_t size);

uint8_t *fck_memory_stream_read(fck_memory_stream *stream, size_t size);

void fck_memory_stream_close(fck_memory_stream *stream);

#endif // !FCK_MEMORY_STREAM_INCLUDED