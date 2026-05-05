#ifndef FCK_INPUT_H_INCLUDED
#define FCK_INPUT_H_INCLUDED

#include <fckc_inttypes.h>

typedef enum fck_input_data_type
{
	fck_input_data_none,
	fck_input_data_scalar,
	fck_input_data_float2,
	fck_input_data_unicode
} fck_input_data_type;

inline const char *fck_input_data_type_to_string(fckc_u32 type)
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

// TODO: Revise
typedef union fck_input_data {
	float as_scalar;
	float as_floats[2];
	fckc_u32 as_unicode;
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

typedef struct fck_input_state
{
	fckc_u32 id;
	fck_input_data data;
} fck_input_state;

typedef struct fck_input_source
{
	const char *name;

	fckc_size_t (*owners)(fckc_u64 **owners);
	fckc_size_t (*events)(fck_input_event *events, fckc_size_t size);
	fckc_size_t (*descriptions)(fck_input_description **descriptions);

	/* Example:
	 * uint64_t items[2]
	 * items[0].id = fck_input_mouse_button_left;
	 * items[1].id = fck_input_mouse_button_right;
	 * size_t result = source->state(0, items, fck_arraysize(items))
	 * fck_assert(result ==  fck_arraysize(items));
	 * fckc_u32 any = 0;
	 * for(size_t index = 0; index < result; index++) {
	 *		fck_input_state* state = items + index;
	 *		any = any || state->data.as_boolean;
	 * }
	 */
	fckc_size_t (*state)(fckc_u64 owner, fck_input_state *states, fckc_size_t size);

	// Maybe push makes sense...
	// fckc_size_t (*push)(fck_input_event *events, fckc_size_t size);

	// fckc_u64 type;
} fck_input_source;

typedef struct fck_input
{
	void (*add)(fck_input_source *source);
	void (*remove)(fck_input_source *source);
	fckc_size_t (*sources)(fck_input_source ***source);
	fckc_size_t (*events)(fck_input_event *events, fckc_size_t size);
} fck_input;

#endif // !FCK_INPUT_H_INCLUDED