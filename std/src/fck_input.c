
#include "fck_input.h"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_log.h>

#include "fckc_apidef.h"
#include "fckc_assert.h"
#include "fckc_math.h"

#define fck_input_source_capacity 64

static fck_input_source *sources[fck_input_source_capacity] = {0};
static fckc_size_t sources_count = 0;

void fck_input_add(fck_input_source *source)
{
	if (sources_count >= fck_input_source_capacity)
	{
		// EHHH, maybe assert... Idk
		return;
	}

	fck_input_source **source_pointer = sources + sources_count;
	*source_pointer = source;
	sources_count = sources_count + 1;
}

void fck_input_remove(fck_input_source *source)
{
	for (fckc_size_t index = 0; index < sources_count; index++)
	{
		fck_input_source **source_pointer = sources + index;
		if (*source_pointer == source)
		{
			fck_input_source **last = sources + sources_count - 1;
			*source_pointer = *last;
			*last = NULL;
			sources_count = sources_count - 1;
			return;
		}
	}
}

fckc_size_t fck_input_sources(fck_input_source ***source)
{
	*source = sources;
	return sources_count;
}

fckc_size_t fck_input_events(fck_input_event *events, fckc_size_t size)
{
	if (sources_count == 0)
	{
		return 0;
	}

	fckc_size_t batch = size / sources_count;
	fckc_size_t rest = size % sources_count;
	fckc_size_t offset = 0;
	for (fckc_size_t index = 0; index < sources_count; index++)
	{
		fckc_size_t count = batch + rest;
		fck_input_source *source_pointer = sources[index];
		fck_input_event *begin = events + offset;
		fckc_size_t result = source_pointer->events(begin, count);
		rest = count - result;
		offset = offset + result;
	}
	fck_assert(offset <= size);
	return offset;
}

fck_input input = (fck_input){
	.add = fck_input_add,
	.remove = fck_input_remove,
	.sources = fck_input_sources,
	.events = fck_input_events,
};

FCK_EXPORT_API fck_input *input_api = &input;
