#include "fck_file.h"

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_iostream.h>

bool fck_file_write(const char *path, const char *name, const char *extension, const void *source, size_t size)
{
	SDL_assert(path != nullptr && "NULL is not a path");
	SDL_assert(name != nullptr && "NULL is not a name");
	SDL_assert(extension != nullptr && "NULL is not an extension");
	SDL_assert(source != nullptr && size != 0);

	char path_buffer[512];
	SDL_zero(path_buffer);

	size_t added_length = SDL_strlcat(path_buffer, FCK_RESOURCE_DIRECTORY_PATH, sizeof(path_buffer));
	added_length = added_length + SDL_strlcat(path_buffer + added_length, path, sizeof(path_buffer));
	added_length = added_length + SDL_strlcat(path_buffer + added_length, name, sizeof(path_buffer));
	SDL_strlcat(path_buffer + added_length, extension, sizeof(path_buffer));

	SDL_IOStream *stream = SDL_IOFromFile(path_buffer, "wb");
	CHECK_WARNING(stream, SDL_GetError(), return false);

	size_t written_size = SDL_WriteIO(stream, source, size);
	CHECK_WARNING(written_size >= 0, SDL_GetError());

	SDL_bool result = SDL_CloseIO(stream);
	CHECK_WARNING(result, SDL_GetError(), return false);

	return true;
}

bool fck_file_exists(const char *path, const char *name, const char *extension)
{
	SDL_assert(path != nullptr && "NULL is not a path");
	SDL_assert(name != nullptr && "NULL is not a name");
	SDL_assert(extension != nullptr && "NULL is not an extension");

	char path_buffer[512];
	SDL_zero(path_buffer);

	size_t added_length = SDL_strlcat(path_buffer, FCK_RESOURCE_DIRECTORY_PATH, sizeof(path_buffer));
	added_length = added_length + SDL_strlcat(path_buffer + added_length, path, sizeof(path_buffer));
	added_length = added_length + SDL_strlcat(path_buffer + added_length, name, sizeof(path_buffer));
	SDL_strlcat(path_buffer + added_length, extension, sizeof(path_buffer));

	return SDL_GetPathInfo(path_buffer, nullptr);
}

bool fck_file_read(const char *path, const char *name, const char *extension, fck_file_memory *output)
{
	SDL_assert(path != nullptr && "NULL is not a path");
	SDL_assert(name != nullptr && "NULL is not a name");
	SDL_assert(extension != nullptr && "NULL is not an extension");
	SDL_assert(output != nullptr);

	char path_buffer[512];
	SDL_zero(path_buffer);

	size_t added_length = SDL_strlcat(path_buffer, FCK_RESOURCE_DIRECTORY_PATH, sizeof(path_buffer));
	added_length = added_length + SDL_strlcat(path_buffer + added_length, path, sizeof(path_buffer));
	added_length = added_length + SDL_strlcat(path_buffer + added_length, name, sizeof(path_buffer));
	SDL_strlcat(path_buffer + added_length, extension, sizeof(path_buffer));

	SDL_IOStream *stream = SDL_IOFromFile(path_buffer, "rb");
	CHECK_ERROR(stream, SDL_GetError(), return false);

	Sint64 stream_size = SDL_GetIOSize(stream);
	CHECK_ERROR(stream_size >= 0, SDL_GetError());

	Uint8 *data = (Uint8 *)SDL_malloc(stream_size);

	size_t read_size = SDL_ReadIO(stream, data, stream_size);
	SDL_bool has_error_in_reading = read_size < stream_size;
	CHECK_ERROR(!has_error_in_reading, SDL_GetError());

	SDL_bool result = SDL_CloseIO(stream);
	CHECK_ERROR(result, SDL_GetError());

	if (has_error_in_reading)
	{
		SDL_free(data);
		output->data = nullptr;
		output->size = 0;
		return false;
	}

	output->data = data;
	output->size = read_size;

	return true;
}

void fck_file_free(fck_file_memory *file_memory)
{
	SDL_assert(file_memory != nullptr);

	SDL_free(file_memory->data);
	file_memory->data = nullptr;
	file_memory->size = 0;
}
