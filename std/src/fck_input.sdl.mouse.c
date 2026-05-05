
#include "fck_input.h"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_log.h>

#include "fckc_apidef.h"
#include "fckc_assert.h"
#include "fckc_math.h"

#define fck_input_source_mouse_support_device_changes 0

static fckc_size_t fck_input_none_owners(fckc_u64 **owners)
{
	return 0;
}
static fckc_size_t fck_input_none_events(fck_input_event *events, fckc_size_t size)
{
	return 0;
}
static fckc_size_t fck_input_none_descriptions(fck_input_description **descriptions)
{
	return 0;
}

static fckc_size_t fck_input_none_state(fckc_u64 owner, fck_input_state *states, fckc_size_t size)
{
	return 0;
}

typedef enum fck_input_mouse_description_type
{
	fck_input_mouse_description_none,
	fck_input_mouse_description_left,
	fck_input_mouse_description_middle,
	fck_input_mouse_description_right,
	fck_input_mouse_description_button_4,
	fck_input_mouse_description_button_5,

	fck_input_mouse_description_position,
	fck_input_mouse_description_wheel,
	fck_input_mouse_description_count
} fck_input_mouse_description_type;

#define fck_input_mouse_owner_capacity 4

#define fck_input_description_table_emplace(desc_id, desc_name, desc_data_type)                                                            \
	[(desc_id)] = {.id = (desc_id), .name = (desc_name), .data_type = (desc_data_type)}

static fckc_size_t fck_input_mouse_owners(fckc_u64 **owners);
static fckc_size_t fck_input_mouse_events(fck_input_event *events, fckc_size_t size);
static fckc_size_t fck_input_mouse_descriptions(fck_input_description **descriptions);
static fckc_size_t fck_input_mouse_state(fckc_u64 owner, fck_input_state *states, fckc_size_t size);

typedef struct fck_input_source_mouse
{
	fck_input_source source;

	fckc_u64 owners[fck_input_mouse_owner_capacity]; // Always a number to double check stuff - Zero is nice, but make it something cool!
	fck_input_description descriptions[fck_input_mouse_description_count];
	fck_input_data data[fck_input_mouse_description_count];
} fck_input_source_mouse;

static fck_input_source_mouse input_mouse = (fck_input_source_mouse){
	.source =
		(fck_input_source){
			.name = "mouse",
			.owners = fck_input_mouse_owners,
			.events = fck_input_mouse_events,
			.descriptions = fck_input_mouse_descriptions,
			.state = fck_input_mouse_state,
		},
	.owners = {0},
	.descriptions =
		{
			fck_input_description_table_emplace(fck_input_mouse_description_none, "none", fck_input_data_none),
			fck_input_description_table_emplace(fck_input_mouse_description_left, "button-1(left)", fck_input_data_scalar),
			fck_input_description_table_emplace(fck_input_mouse_description_middle, "button-2(middle)", fck_input_data_scalar),
			fck_input_description_table_emplace(fck_input_mouse_description_right, "button-3(right)", fck_input_data_scalar),
			fck_input_description_table_emplace(fck_input_mouse_description_button_4, "button-4", fck_input_data_scalar),
			fck_input_description_table_emplace(fck_input_mouse_description_button_5, "button-5", fck_input_data_scalar),
			fck_input_description_table_emplace(fck_input_mouse_description_position, "position", fck_input_data_float2),
			fck_input_description_table_emplace(fck_input_mouse_description_wheel, "wheel", fck_input_data_float2),
		},
	.data = {0},
};

FCK_EXPORT_API fck_input_source *input_source_mouse = &input_mouse.source;

static fckc_size_t fck_input_mouse_owners(fckc_u64 **owners)
{
	*owners = input_mouse.owners;
	return 1;
}

static fckc_size_t fck_input_mouse_events(fck_input_event *events, fckc_size_t size)
{
	if (events == NULL || size == 0)
	{
		return 0;
	}

	fckc_u32 min_event = SDL_EVENT_MOUSE_MOTION;
	fckc_u32 max_event = fck_input_source_mouse_support_device_changes ? SDL_EVENT_MOUSE_REMOVED : SDL_EVENT_MOUSE_WHEEL;

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
			// Maybe we should log errors...
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
			case SDL_EVENT_MOUSE_ADDED:
				SDL_Log("Added");
				continue;
			case SDL_EVENT_MOUSE_REMOVED:
				SDL_Log("Removed");
				continue;
			case SDL_EVENT_MOUSE_MOTION:
				event->description = &input_mouse.descriptions[fck_input_mouse_description_position];
				event->data.as_floats[0] = e->motion.x;
				event->data.as_floats[1] = e->motion.y;
				event->owner = to_u64(e->motion.which);
				break;
			case SDL_EVENT_MOUSE_BUTTON_UP:
				event->description = &input_mouse.descriptions[e->button.button];
				event->data.as_scalar = 0.0f;
				event->owner = to_u64(e->button.which);
				break;
			case SDL_EVENT_MOUSE_BUTTON_DOWN:
				event->description = &input_mouse.descriptions[e->button.button];
				event->data.as_scalar = 1.0f;
				event->owner = to_u64(e->button.which);
				break;
			case SDL_EVENT_MOUSE_WHEEL:
				event->description = &input_mouse.descriptions[fck_input_mouse_description_wheel];
				event->data.as_floats[0] = e->wheel.x;
				event->data.as_floats[1] = e->wheel.y;
				event->owner =to_u64(e->wheel.which);
				break;
			}
			event->time = e->common.timestamp;
			event->source = &input_mouse.source;
			event->userdata = NULL;

			input_mouse.data[event->description->id] = event->data;
		}

		used = used + result;
		if (used == size)
		{
			break;
		}
	}
	return used;
}

static fckc_size_t fck_input_mouse_descriptions(fck_input_description **descriptions)
{
	// We exclude "none"
	*descriptions = input_mouse.descriptions + 1;
	return fck_input_mouse_description_count - 1;
}

static fckc_size_t fck_input_mouse_state(fckc_u64 owner, fck_input_state *states, fckc_size_t size)
{
	for (fckc_size_t index = 0; index < size; index++)
	{
		fck_input_state *state = states + index;
		if (state->id >= fck_input_mouse_description_count)
		{
			fck_input_state *last_state = states + size - 1;
			*state = *last_state;
			last_state->id = fck_input_mouse_description_none;
			size = size - 1;
			index = index - 1;
			continue;
		}

		const fck_input_data* data = input_mouse.data + state->id;
		state->data = *data;
	}
	return size;
}

// fck_input_source virtual_keyboard;
// fck_input_source physical_keyboard;