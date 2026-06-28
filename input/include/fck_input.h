#ifndef FCK_INPUT_H_INCLUDED
#define FCK_INPUT_H_INCLUDED

#include <fckc_inttypes.h>

#define fck_input_api_name "fck-input"
#define fck_input_source_name "fck-input-source"

typedef enum fck_input_source_type
{
	fck_input_source_none,
	fck_input_source_keyboard,
	fck_input_source_mouse,
	fck_input_source_gamepad,
	fck_input_source_text,
	fck_input_source_other = 0xFFFF
} fck_input_source_type;

typedef enum fck_input_data_type
{
	fck_input_data_none,
	fck_input_data_scalar,
	fck_input_data_float2,
	fck_input_data_unicode
	// Maybe: value, pair, unicode? :-(
} fck_input_data_type;

static inline const char *fck_input_data_type_to_string(fckc_u32 type)
{
	switch (type)
	{
	default:
		return "none";
	case fck_input_data_scalar:
		return "fck_input_data_scalar";
	case fck_input_data_float2:
		return "fck_input_data_float2";
	case fck_input_data_unicode:
		return "fck_input_data_unicode";
	}
}

typedef struct fck_input_description
{
	fck_alias(fck_input_data_type, fckc_u32) data_type;
	fckc_u32 id;

	const char *name;
} fck_input_description;

// TODO: Revise - I hate the name, I hate the access, I hate quite a lot about this one
// I do like unicode though
typedef union fck_input_data {
	float scalar;
	float floats[2];
	fckc_u32 unicode;
} fck_input_data;

struct fck_input_source;

typedef struct fck_input_event
{
	fckc_u64 time;
	fckc_u64 owner;

	fck_input_data data;
	struct fck_input_source *source;
	// Idk if id is better tbh
	struct fck_input_description *description;

	void *userdata;
} fck_input_event;

typedef struct fck_input_source
{
	const char *name;
	fck_alias(fck_input_source_type, fckc_u32) type;

	fckc_size_t (*owners)(fckc_u64 **owners);
	fckc_size_t (*events)(fck_input_event *events, fckc_size_t size);
	fckc_size_t (*descriptions)(fck_input_description **descriptions);
	/* Example:
	 * fckc_u32 ids[2] = {fck_input_mouse_button_left, fck_input_mouse_button_right};
	 * fck_input_data states[fck_arraysize(items)] = {0};
	 * size_t result = source->state(0, ids, states, fck_arraysize(states))
	 * fck_assert(result ==  fck_arraysize(states));
	 * fckc_u32 any = 0;
	 * for(size_t index = 0; index < result; index++) {
	 *		fck_input_data* state = states + index;
	 *		any = any || state->data.as_boolean;
	 * }
	 */
	// TODO: Evaluate if fckc_size_t (*state)(fckc_u64 owner, fck_input_state *states, fckc_size_t size); or the current one
	// TODO: I need to seriously revise this shit lol
	fckc_size_t (*states)(fckc_u64 owner, fckc_u32 *ids, fck_input_data *states, fckc_size_t size);

	// Maybe push makes sense...
	// fckc_size_t (*push)(fck_input_event *events, fckc_size_t size);

} fck_input_source;

typedef struct fck_input
{
	void (*add)(fck_input_source *source);
	void (*remove)(fck_input_source *source);
	fckc_size_t (*sources)(fck_input_source ***source);
	fckc_size_t (*events)(fck_input_event *events, fckc_size_t size);

	// Candiate
	int (*is)(fck_input_source *source, const char *name);
} fck_input;

/*	Input Utilities */

/*	Example: fck_input_poll(api->input, variable, {
 *	if (app->input->is(e->source, "physical-keyboard"))
 *	      {
 *	          if (e->description->id == fck_pkey_escape)
 *	          {
 *	              if (e->data.as_scalar > 0.0f)
 *	              {
 *	                  return FCK_TEST_APP_RESULT_DONE;
 *	              }
 *	          }
 *	      }
 *	})
 * NOTE: Is this worth it?*/
#define fck_input_poll(input_or_source, event, body)                                                                                       \
	{                                                                                                                                      \
		fckc_size_t _##event##_count = 0;                                                                                                  \
		fck_input_event _##event[32];                                                                                                      \
		fck_input_event *(event) = NULL;                                                                                                   \
		for (;;)                                                                                                                           \
		{                                                                                                                                  \
			_##event##_count = (input_or_source)->events(_##event, fck_arraysize(_##event));                                               \
			if (_##event##_count == 0)                                                                                                     \
			{                                                                                                                              \
				break;                                                                                                                     \
			}                                                                                                                              \
			for (fckc_size_t _##event_##index = 0; _##event_##index < _##event##_count; _##event_##index++)                                \
			{                                                                                                                              \
				(event) = _##event + _##event_##index;                                                                                     \
				body;                                                                                                                      \
			}                                                                                                                              \
		}                                                                                                                                  \
	}

#endif // !FCK_INPUT_H_INCLUDED