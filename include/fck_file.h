#ifndef FCK_FILE_INCLUDED
#define FCK_FILE_INCLUDED

#include "fck_checks.h"

#include <SDL3/SDL_stdinc.h>

struct fck_file_memory
{
	Uint8 *data;
	size_t size;
};

bool fck_file_write(const char *path, const char *name, const char *extension, const void *source, size_t size);

bool fck_file_exists(const char *path, const char *name, const char *extension);

bool fck_file_read(const char *path, const char *name, const char *extension, fck_file_memory *output);

void fck_file_free(fck_file_memory *file_memory);
#endif // !FCK_FILE_INCLUDED
