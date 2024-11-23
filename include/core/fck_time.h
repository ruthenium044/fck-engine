#ifndef FCK_TIME_INCLUDED
#define FCK_TIME_INCLUDED

#include <SDL3/SDL_stdinc.h>

using fck_milliseconds = uint64_t;

struct fck_time
{
	fck_milliseconds delta;
	fck_milliseconds current;
};

#endif // !FCK_TIME_INCLUDED
