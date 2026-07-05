

#include "fck_input.h"
#include "fckc_apidef.h"
#include "fckc_assert.h"
#include <fckc_inttypes.h>
#include <fck_apis.h>

#include <SDL3/SDL_stdinc.h>

#include <stddef.h>

#define fck_input_source_capacity 64

static fck_api_registry *apis;

static void fck_input_add(fck_input_source *source)
{
	apis->add(fck_input_source_name, source);
}

static void fck_input_remove(fck_input_source *source)
{
	apis->remove(fck_input_source_name, source);
}

static fckc_size_t fck_input_sources(fck_input_source ***sources)
{
	void **result;
	const fckc_size_t count = apis->implementations(fck_input_source_name, &result);
	*sources = (fck_input_source **)result;
	return count;
}

static fckc_size_t fck_input_events(fck_input_event *events, fckc_size_t size)
{
	fck_input_source **sources;
	const fckc_size_t sources_count = fck_input_sources(&sources);
	if (sources_count == 0)
	{
		return 0;
	}

	const fckc_size_t batch = size / sources_count;
	fckc_size_t rest = size % sources_count;
	fckc_size_t offset = 0;
	for (fckc_size_t index = 0; index < sources_count; index++)
	{
		const fckc_size_t count = batch + rest;
		fck_input_source *source_pointer = sources[index];
		fck_input_event *begin = events + offset;
		const fckc_size_t result = source_pointer->events(begin, count);
		rest = count - result;
		offset = offset + result;
	}
	fck_assert(offset <= size);
	return offset;
}

static int fck_input_is(fck_input_source *source, const char *name)
{
	if (SDL_strcmp(source->name, name) == 0)
	{
		return 1;
	}
	return 0;
}

static fck_input input_api = {
	.add = fck_input_add,
	.remove = fck_input_remove,
	.sources = fck_input_sources,
	.events = fck_input_events,
	.is = fck_input_is,
};

FCK_EXPORT_API fck_input *fck_input_all_load(fck_api_registry *registry, void *old)
{	
	(void)old;
	apis = registry;
	registry->add(fck_input_api_name, &input_api);
	return &input_api;
}