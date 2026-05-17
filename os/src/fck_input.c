

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_log.h>

#include "fck_input.h"
#include "fckc_apidef.h"
#include "fckc_assert.h"
#include "fckc_math.h"

#define fck_input_source_capacity 64

FCK_IMPORT_API fck_input_source *input_mouse;
FCK_IMPORT_API fck_input_source *input_physical_keyboard;
FCK_IMPORT_API fck_input_source *input_text;
FCK_IMPORT_API fck_input_source *dualsense_source;

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

int fck_input_is(fck_input_source *source, const char *name)
{
	if (SDL_strcmp(source->name, name) == 0)
	{
		return 1;
	}
	return 0;
}

fck_input input_api = (fck_input){
	.add = fck_input_add,
	.remove = fck_input_remove,
	.sources = fck_input_sources,
	.events = fck_input_events,
	.is = fck_input_is,
};

FCK_EXPORT_API fck_input *fck_input_load(void)
{
	input_api.add(dualsense_source);
	input_api.add(input_mouse);
	input_api.add(input_physical_keyboard);
	input_api.add(input_text);
	return &input_api;
}