
#define FCK_STD_EXPORT
#include "fck_os.h"

#include <SDL3/SDL_stdinc.h>

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

static fck_std_api std_api = {
	.character = &char_api,
	.str = &string_api,
	.mem = &memory_api,
	.io = &io_api,
};

fck_std_api *std = &std_api;