#include "fck_input.h"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_stdinc.h>

#include "fckc_apidef.h"
#include "fckc_assert.h"
#include "fckc_math.h"

static fckc_size_t fck_input_text_owners(fckc_u64 **owners);
static fckc_size_t fck_input_text_events(fck_input_event *events, fckc_size_t size);
static fckc_size_t fck_input_text_descriptions(fck_input_description **descriptions);
static fckc_size_t fck_input_text_state(fckc_u64 owner, fck_input_state *states, fckc_size_t size);

typedef struct fck_input_source_text
{
	fck_input_source source;

	fckc_u64 owner;
	fck_input_description description;
	// fck_input_data data[fck_pkey_count];
} fck_input_source_text;

static fck_input_source_text input_text = (fck_input_source_text){
	.source =
		(fck_input_source){
			.name = "text",
			.owners = fck_input_text_owners,
			.events = fck_input_text_events,
			.descriptions = fck_input_text_descriptions,
			.state = fck_input_text_state,
		},
	.owner = 0,
	.description = {.data_type = fck_input_data_unicode, .id = 0, .name = "input"},
	//.data = {0},
};

FCK_EXPORT_API fck_input_source *input_source_text = &input_text.source;

fckc_size_t fck_input_text_owners(fckc_u64 **owners)
{
	return 0;
}

fckc_size_t fck_input_text_events(fck_input_event *events, fckc_size_t size)
{
	if (events == NULL || size == 0)
	{
		return 0;
	}

	fckc_u32 min_event = SDL_EVENT_TEXT_INPUT;
	fckc_u32 max_event = SDL_EVENT_TEXT_INPUT;

	SDL_Event sdl_events[32];
	fckc_size_t used = 0;

	SDL_PumpEvents();

	for (;;)
	{
		const fckc_size_t rest = size - used;
		fckc_size_t available = fck_min(fck_arraysize(sdl_events), rest);

		int result = SDL_PeepEvents(sdl_events, available, SDL_GETEVENT, min_event, max_event);
		if (result <= 0)
		{
			break;
		}

		for (fckc_size_t index = 0; index < to_size_t(result); index++)
		{
			const SDL_Event *e = sdl_events + index;
			fck_input_event *event = events + used + index;

			switch (e->type)
			{
			default:
				fck_assert(false);
				continue;
			case SDL_EVENT_TEXT_INPUT:
				event->description = &input_text.description;
				const char* text = e->text.text;
				event->data.as_unicode = SDL_StepUTF8(&text, NULL);
				event->owner = 0;
				break;
			}

			event->time = e->common.timestamp;
			event->source = &input_text.source;
			event->userdata = NULL;
		}

		used = used + result;
		if (used == size)
		{
			break;
		}
	}
	return used;
}

fckc_size_t fck_input_text_descriptions(fck_input_description **descriptions)
{
	*descriptions = &input_text;
	return 1;
}

fckc_size_t fck_input_text_state(fckc_u64 owner, fck_input_state *states, fckc_size_t size)
{
	// Can we make text events state somehow? Maybe accumulate N unicodes and ringbuffer them? 
	// Oh... no...
	return 0;
}
