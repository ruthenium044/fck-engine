
#define FCK_STD_EXPORT
#include "fck_os.h"

#include <SDL3/SDL_loadso.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_video.h>

static fck_char_api char_api = {
	.isdigit = SDL_isdigit,
	.isspace = SDL_isspace,
};

static fck_unsafe_string_api unsafe_string_api = {
	.cmp = SDL_strcmp,
	.dup = SDL_strdup,
	.len = SDL_strlen,
};

static fck_string_api string_api = {
	.unsafe = &unsafe_string_api, //
	.cmp = SDL_strncmp,           //
	.dup = SDL_strndup,           //
	.len = SDL_strnlen,           //
	.toll = SDL_strtoll,          //
	.toull = SDL_strtoull,        //
	.tod = SDL_strtod,            //
};

static fck_memory_api memory_api = {
	.cpy = SDL_memcpy,
	.set = SDL_memset,
};

static fck_io_api io_api = {
	.snprintf = SDL_snprintf,
};

static int fck_shared_object_is_valid(fck_shared_object so)
{
	return so.handle != NULL;
}

static fck_shared_object fck_shared_object_load(const char *path)
{
	SDL_SharedObject *so = SDL_LoadObject(path);
	return (fck_shared_object){.handle = (void *)so};
}

static void fck_shared_object_unload(fck_shared_object so)
{
	SDL_SharedObject *sdl_so = NULL;
	SDL_UnloadObject((SDL_SharedObject *)so.handle);
}

static void *fck_shared_object_symbol(fck_shared_object so, const char *name)
{
	return (void *)SDL_LoadFunction((SDL_SharedObject *)so.handle, name);
}

static fck_shared_object_api so_api = {
	.load = fck_shared_object_load,
	.symbol = fck_shared_object_symbol,
	.unload = fck_shared_object_unload,
	.is_valid = fck_shared_object_is_valid,
};

static fck_window_handle fck_window_api_create(const char *name, int w, int h)
{
	SDL_Window* window = SDL_CreateWindow(name, w, h, 0);
	return (fck_window_handle){.handle = window};
}

void fck_window_api_destroy(fck_window_handle window)
{
	SDL_DestroyWindow((SDL_Window*)window.handle);
}

static fck_window_api window_api = {
	.create = fck_window_api_create,
	.destroy = fck_window_api_destroy,
};

static fck_os_api std_api = {
	.chr = &char_api,
	.str = &string_api,
	.mem = &memory_api,
	.io = &io_api,
	.so = &so_api,
	.win = &window_api,

};

fck_os_api *os = &std_api;
