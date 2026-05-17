#include "fck_input.h"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_log.h>

#include "fckc_apidef.h"
#include "fckc_assert.h"
#include "fckc_math.h"
#include "fck_pkey.h"

#define fck_input_source_physical_keyboard_support_device_changes 0

#define fck_input_physical_keyboard_owner_capacity 4

#define fck_pkey_case_to_string(pkey) #pkey

#define fck_pkey_table_emplace(desc_id)                                                                                                    \
	[(desc_id)] = {.id = (desc_id), .name = (fck_pkey_case_to_string(desc_id)), .data_type = fck_input_data_scalar}

static fckc_size_t fck_input_physical_keyboard_owners(fckc_u64 **owners);
static fckc_size_t fck_input_physical_keyboard_events(fck_input_event *events, fckc_size_t size);
static fckc_size_t fck_input_physical_keyboard_descriptions(fck_input_description **descriptions);
static fckc_size_t fck_input_physical_keyboard_states(fckc_u64 owner, fckc_u32* ids, fck_input_data* states, fckc_size_t size);

typedef struct fck_input_source_physical_keyboard
{
	fck_input_source source;

	fckc_u64 owners[fck_input_physical_keyboard_owner_capacity];
	fck_input_description descriptions[fck_pkey_count];
	fck_input_data data[fck_pkey_count];
} fck_input_source_physical_keyboard;

static fck_input_source_physical_keyboard input_source_physical_keyboard = (fck_input_source_physical_keyboard){
	.source =
		(fck_input_source){
			.name = "physical-keyboard",
			.owners = fck_input_physical_keyboard_owners,
			.events = fck_input_physical_keyboard_events,
			.descriptions = fck_input_physical_keyboard_descriptions,
			.states = fck_input_physical_keyboard_states,
		},
	.owners = {0},
	.descriptions =
		{
			fck_pkey_table_emplace(fck_pkey_unknown),
			fck_pkey_table_emplace(fck_pkey_a),
			fck_pkey_table_emplace(fck_pkey_b),
			fck_pkey_table_emplace(fck_pkey_c),
			fck_pkey_table_emplace(fck_pkey_d),
			fck_pkey_table_emplace(fck_pkey_e),
			fck_pkey_table_emplace(fck_pkey_f),
			fck_pkey_table_emplace(fck_pkey_g),
			fck_pkey_table_emplace(fck_pkey_h),
			fck_pkey_table_emplace(fck_pkey_i),
			fck_pkey_table_emplace(fck_pkey_j),
			fck_pkey_table_emplace(fck_pkey_k),
			fck_pkey_table_emplace(fck_pkey_l),
			fck_pkey_table_emplace(fck_pkey_m),
			fck_pkey_table_emplace(fck_pkey_n),
			fck_pkey_table_emplace(fck_pkey_o),
			fck_pkey_table_emplace(fck_pkey_p),
			fck_pkey_table_emplace(fck_pkey_q),
			fck_pkey_table_emplace(fck_pkey_r),
			fck_pkey_table_emplace(fck_pkey_s),
			fck_pkey_table_emplace(fck_pkey_t),
			fck_pkey_table_emplace(fck_pkey_u),
			fck_pkey_table_emplace(fck_pkey_v),
			fck_pkey_table_emplace(fck_pkey_w),
			fck_pkey_table_emplace(fck_pkey_x),
			fck_pkey_table_emplace(fck_pkey_y),
			fck_pkey_table_emplace(fck_pkey_z),
			fck_pkey_table_emplace(fck_pkey_1),
			fck_pkey_table_emplace(fck_pkey_2),
			fck_pkey_table_emplace(fck_pkey_3),
			fck_pkey_table_emplace(fck_pkey_4),
			fck_pkey_table_emplace(fck_pkey_5),
			fck_pkey_table_emplace(fck_pkey_6),
			fck_pkey_table_emplace(fck_pkey_7),
			fck_pkey_table_emplace(fck_pkey_8),
			fck_pkey_table_emplace(fck_pkey_9),
			fck_pkey_table_emplace(fck_pkey_0),
			fck_pkey_table_emplace(fck_pkey_return),
			fck_pkey_table_emplace(fck_pkey_escape),
			fck_pkey_table_emplace(fck_pkey_backspace),
			fck_pkey_table_emplace(fck_pkey_tab),
			fck_pkey_table_emplace(fck_pkey_space),
			fck_pkey_table_emplace(fck_pkey_minus),
			fck_pkey_table_emplace(fck_pkey_equals),
			fck_pkey_table_emplace(fck_pkey_leftbracket),
			fck_pkey_table_emplace(fck_pkey_rightbracket),
			fck_pkey_table_emplace(fck_pkey_backslash),
			fck_pkey_table_emplace(fck_pkey_nonushash),
			fck_pkey_table_emplace(fck_pkey_semicolon),
			fck_pkey_table_emplace(fck_pkey_apostrophe),
			fck_pkey_table_emplace(fck_pkey_grave),
			fck_pkey_table_emplace(fck_pkey_comma),
			fck_pkey_table_emplace(fck_pkey_period),
			fck_pkey_table_emplace(fck_pkey_slash),
			fck_pkey_table_emplace(fck_pkey_capslock),
			fck_pkey_table_emplace(fck_pkey_f1),
			fck_pkey_table_emplace(fck_pkey_f2),
			fck_pkey_table_emplace(fck_pkey_f3),
			fck_pkey_table_emplace(fck_pkey_f4),
			fck_pkey_table_emplace(fck_pkey_f5),
			fck_pkey_table_emplace(fck_pkey_f6),
			fck_pkey_table_emplace(fck_pkey_f7),
			fck_pkey_table_emplace(fck_pkey_f8),
			fck_pkey_table_emplace(fck_pkey_f9),
			fck_pkey_table_emplace(fck_pkey_f10),
			fck_pkey_table_emplace(fck_pkey_f11),
			fck_pkey_table_emplace(fck_pkey_f12),
			fck_pkey_table_emplace(fck_pkey_printscreen),
			fck_pkey_table_emplace(fck_pkey_scrolllock),
			fck_pkey_table_emplace(fck_pkey_pause),
			fck_pkey_table_emplace(fck_pkey_insert),
			fck_pkey_table_emplace(fck_pkey_home),
			fck_pkey_table_emplace(fck_pkey_pageup),
			fck_pkey_table_emplace(fck_pkey_delete),
			fck_pkey_table_emplace(fck_pkey_end),
			fck_pkey_table_emplace(fck_pkey_pagedown),
			fck_pkey_table_emplace(fck_pkey_right),
			fck_pkey_table_emplace(fck_pkey_left),
			fck_pkey_table_emplace(fck_pkey_down),
			fck_pkey_table_emplace(fck_pkey_up),
			fck_pkey_table_emplace(fck_pkey_numlockclear),
			fck_pkey_table_emplace(fck_pkey_kp_divide),
			fck_pkey_table_emplace(fck_pkey_kp_multiply),
			fck_pkey_table_emplace(fck_pkey_kp_minus),
			fck_pkey_table_emplace(fck_pkey_kp_plus),
			fck_pkey_table_emplace(fck_pkey_kp_enter),
			fck_pkey_table_emplace(fck_pkey_kp_1),
			fck_pkey_table_emplace(fck_pkey_kp_2),
			fck_pkey_table_emplace(fck_pkey_kp_3),
			fck_pkey_table_emplace(fck_pkey_kp_4),
			fck_pkey_table_emplace(fck_pkey_kp_5),
			fck_pkey_table_emplace(fck_pkey_kp_6),
			fck_pkey_table_emplace(fck_pkey_kp_7),
			fck_pkey_table_emplace(fck_pkey_kp_8),
			fck_pkey_table_emplace(fck_pkey_kp_9),
			fck_pkey_table_emplace(fck_pkey_kp_0),
			fck_pkey_table_emplace(fck_pkey_kp_period),
			fck_pkey_table_emplace(fck_pkey_nonusbackslash),
			fck_pkey_table_emplace(fck_pkey_application),
			fck_pkey_table_emplace(fck_pkey_power),
			fck_pkey_table_emplace(fck_pkey_kp_equals),
			fck_pkey_table_emplace(fck_pkey_f13),
			fck_pkey_table_emplace(fck_pkey_f14),
			fck_pkey_table_emplace(fck_pkey_f15),
			fck_pkey_table_emplace(fck_pkey_f16),
			fck_pkey_table_emplace(fck_pkey_f17),
			fck_pkey_table_emplace(fck_pkey_f18),
			fck_pkey_table_emplace(fck_pkey_f19),
			fck_pkey_table_emplace(fck_pkey_f20),
			fck_pkey_table_emplace(fck_pkey_f21),
			fck_pkey_table_emplace(fck_pkey_f22),
			fck_pkey_table_emplace(fck_pkey_f23),
			fck_pkey_table_emplace(fck_pkey_f24),
			fck_pkey_table_emplace(fck_pkey_execute),
			fck_pkey_table_emplace(fck_pkey_help),
			fck_pkey_table_emplace(fck_pkey_menu),
			fck_pkey_table_emplace(fck_pkey_select),
			fck_pkey_table_emplace(fck_pkey_stop),
			fck_pkey_table_emplace(fck_pkey_again),
			fck_pkey_table_emplace(fck_pkey_undo),
			fck_pkey_table_emplace(fck_pkey_cut),
			fck_pkey_table_emplace(fck_pkey_copy),
			fck_pkey_table_emplace(fck_pkey_paste),
			fck_pkey_table_emplace(fck_pkey_find),
			fck_pkey_table_emplace(fck_pkey_mute),
			fck_pkey_table_emplace(fck_pkey_volumeup),
			fck_pkey_table_emplace(fck_pkey_volumedown),
			fck_pkey_table_emplace(fck_pkey_kp_comma),
			fck_pkey_table_emplace(fck_pkey_kp_equalsas400),
			fck_pkey_table_emplace(fck_pkey_international1),
			fck_pkey_table_emplace(fck_pkey_international2),
			fck_pkey_table_emplace(fck_pkey_international3),
			fck_pkey_table_emplace(fck_pkey_international4),
			fck_pkey_table_emplace(fck_pkey_international5),
			fck_pkey_table_emplace(fck_pkey_international6),
			fck_pkey_table_emplace(fck_pkey_international7),
			fck_pkey_table_emplace(fck_pkey_international8),
			fck_pkey_table_emplace(fck_pkey_international9),
			fck_pkey_table_emplace(fck_pkey_lang1),
			fck_pkey_table_emplace(fck_pkey_lang2),
			fck_pkey_table_emplace(fck_pkey_lang3),
			fck_pkey_table_emplace(fck_pkey_lang4),
			fck_pkey_table_emplace(fck_pkey_lang5),
			fck_pkey_table_emplace(fck_pkey_lang6),
			fck_pkey_table_emplace(fck_pkey_lang7),
			fck_pkey_table_emplace(fck_pkey_lang8),
			fck_pkey_table_emplace(fck_pkey_lang9),
			fck_pkey_table_emplace(fck_pkey_alterase),
			fck_pkey_table_emplace(fck_pkey_sysreq),
			fck_pkey_table_emplace(fck_pkey_cancel),
			fck_pkey_table_emplace(fck_pkey_clear),
			fck_pkey_table_emplace(fck_pkey_prior),
			fck_pkey_table_emplace(fck_pkey_return2),
			fck_pkey_table_emplace(fck_pkey_separator),
			fck_pkey_table_emplace(fck_pkey_out),
			fck_pkey_table_emplace(fck_pkey_oper),
			fck_pkey_table_emplace(fck_pkey_clearagain),
			fck_pkey_table_emplace(fck_pkey_crsel),
			fck_pkey_table_emplace(fck_pkey_exsel),
			fck_pkey_table_emplace(fck_pkey_kp_00),
			fck_pkey_table_emplace(fck_pkey_kp_000),
			fck_pkey_table_emplace(fck_pkey_thousandsseparator),
			fck_pkey_table_emplace(fck_pkey_decimalseparator),
			fck_pkey_table_emplace(fck_pkey_currencyunit),
			fck_pkey_table_emplace(fck_pkey_currencysubunit),
			fck_pkey_table_emplace(fck_pkey_kp_leftparen),
			fck_pkey_table_emplace(fck_pkey_kp_rightparen),
			fck_pkey_table_emplace(fck_pkey_kp_leftbrace),
			fck_pkey_table_emplace(fck_pkey_kp_rightbrace),
			fck_pkey_table_emplace(fck_pkey_kp_tab),
			fck_pkey_table_emplace(fck_pkey_kp_backspace),
			fck_pkey_table_emplace(fck_pkey_kp_a),
			fck_pkey_table_emplace(fck_pkey_kp_b),
			fck_pkey_table_emplace(fck_pkey_kp_c),
			fck_pkey_table_emplace(fck_pkey_kp_d),
			fck_pkey_table_emplace(fck_pkey_kp_e),
			fck_pkey_table_emplace(fck_pkey_kp_f),
			fck_pkey_table_emplace(fck_pkey_kp_xor),
			fck_pkey_table_emplace(fck_pkey_kp_power),
			fck_pkey_table_emplace(fck_pkey_kp_percent),
			fck_pkey_table_emplace(fck_pkey_kp_less),
			fck_pkey_table_emplace(fck_pkey_kp_greater),
			fck_pkey_table_emplace(fck_pkey_kp_ampersand),
			fck_pkey_table_emplace(fck_pkey_kp_dblampersand),
			fck_pkey_table_emplace(fck_pkey_kp_verticalbar),
			fck_pkey_table_emplace(fck_pkey_kp_dblverticalbar),
			fck_pkey_table_emplace(fck_pkey_kp_colon),
			fck_pkey_table_emplace(fck_pkey_kp_hash),
			fck_pkey_table_emplace(fck_pkey_kp_space),
			fck_pkey_table_emplace(fck_pkey_kp_at),
			fck_pkey_table_emplace(fck_pkey_kp_exclam),
			fck_pkey_table_emplace(fck_pkey_kp_memstore),
			fck_pkey_table_emplace(fck_pkey_kp_memrecall),
			fck_pkey_table_emplace(fck_pkey_kp_memclear),
			fck_pkey_table_emplace(fck_pkey_kp_memadd),
			fck_pkey_table_emplace(fck_pkey_kp_memsubtract),
			fck_pkey_table_emplace(fck_pkey_kp_memmultiply),
			fck_pkey_table_emplace(fck_pkey_kp_memdivide),
			fck_pkey_table_emplace(fck_pkey_kp_plusminus),
			fck_pkey_table_emplace(fck_pkey_kp_clear),
			fck_pkey_table_emplace(fck_pkey_kp_clearentry),
			fck_pkey_table_emplace(fck_pkey_kp_binary),
			fck_pkey_table_emplace(fck_pkey_kp_octal),
			fck_pkey_table_emplace(fck_pkey_kp_decimal),
			fck_pkey_table_emplace(fck_pkey_kp_hexadecimal),
			fck_pkey_table_emplace(fck_pkey_lctrl),
			fck_pkey_table_emplace(fck_pkey_lshift),
			fck_pkey_table_emplace(fck_pkey_lalt),
			fck_pkey_table_emplace(fck_pkey_lgui),
			fck_pkey_table_emplace(fck_pkey_rctrl),
			fck_pkey_table_emplace(fck_pkey_rshift),
			fck_pkey_table_emplace(fck_pkey_ralt),
			fck_pkey_table_emplace(fck_pkey_rgui),
			fck_pkey_table_emplace(fck_pkey_mode),
			fck_pkey_table_emplace(fck_pkey_sleep),
			fck_pkey_table_emplace(fck_pkey_wake),
			fck_pkey_table_emplace(fck_pkey_channel_increment),
			fck_pkey_table_emplace(fck_pkey_channel_decrement),
			fck_pkey_table_emplace(fck_pkey_media_play),
			fck_pkey_table_emplace(fck_pkey_media_pause),
			fck_pkey_table_emplace(fck_pkey_media_record),
			fck_pkey_table_emplace(fck_pkey_media_fast_forward),
			fck_pkey_table_emplace(fck_pkey_media_rewind),
			fck_pkey_table_emplace(fck_pkey_media_next_track),
			fck_pkey_table_emplace(fck_pkey_media_previous_track),
			fck_pkey_table_emplace(fck_pkey_media_stop),
			fck_pkey_table_emplace(fck_pkey_media_eject),
			fck_pkey_table_emplace(fck_pkey_media_play_pause),
			fck_pkey_table_emplace(fck_pkey_media_select),
			fck_pkey_table_emplace(fck_pkey_ac_new),
			fck_pkey_table_emplace(fck_pkey_ac_open),
			fck_pkey_table_emplace(fck_pkey_ac_close),
			fck_pkey_table_emplace(fck_pkey_ac_exit),
			fck_pkey_table_emplace(fck_pkey_ac_save),
			fck_pkey_table_emplace(fck_pkey_ac_print),
			fck_pkey_table_emplace(fck_pkey_ac_properties),
			fck_pkey_table_emplace(fck_pkey_ac_search),
			fck_pkey_table_emplace(fck_pkey_ac_home),
			fck_pkey_table_emplace(fck_pkey_ac_back),
			fck_pkey_table_emplace(fck_pkey_ac_forward),
			fck_pkey_table_emplace(fck_pkey_ac_stop),
			fck_pkey_table_emplace(fck_pkey_ac_refresh),
			fck_pkey_table_emplace(fck_pkey_ac_bookmarks),
			fck_pkey_table_emplace(fck_pkey_softleft),
			fck_pkey_table_emplace(fck_pkey_softright),
			fck_pkey_table_emplace(fck_pkey_call),
			fck_pkey_table_emplace(fck_pkey_endcall),
			fck_pkey_table_emplace(fck_pkey_reserved),
		},
	.data = {0},
};

FCK_EXPORT_API fck_input_source *input_physical_keyboard = &input_source_physical_keyboard.source;

static fckc_size_t fck_input_physical_keyboard_owners(fckc_u64 **owners)
{
	if (!SDL_HasKeyboard())
	{
		return 0;
	}
	*owners = input_source_physical_keyboard.owners;
	return 1;
}

static fckc_size_t fck_input_physical_keyboard_events(fck_input_event *events, fckc_size_t size)
{
	if (events == NULL || size == 0)
	{
		return 0;
	}

	fckc_u32 min_event = SDL_EVENT_KEY_DOWN;
	fckc_u32 max_event = fck_input_source_physical_keyboard_support_device_changes ? SDL_EVENT_KEYBOARD_REMOVED : SDL_EVENT_KEY_UP;

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
			case SDL_EVENT_KEY_DOWN:
				event->description = &input_source_physical_keyboard.descriptions[e->key.scancode];
				event->data.scalar = 1.0f;
				event->owner = to_u64(e->key.which);
				break;
			case SDL_EVENT_KEY_UP:
				event->description = &input_source_physical_keyboard.descriptions[e->key.scancode];
				event->data.scalar = 0.0f;
				event->owner = to_u64(e->key.which);
				break;
			}
			event->time = e->common.timestamp;
			event->source = &input_source_physical_keyboard.source;
			event->userdata = NULL;

			input_source_physical_keyboard.data[event->description->id] = event->data;
		}

		used = used + result;
		if (used == size)
		{
			break;
		}
	}
	return used;
}

static fckc_size_t fck_input_physical_keyboard_descriptions(fck_input_description **descriptions)
{
	// We exclude "none"
	*descriptions = input_source_physical_keyboard.descriptions + 1;
	return fck_pkey_count - 1;
}

static fckc_size_t fck_input_physical_keyboard_states(fckc_u64 owner, fckc_u32* ids, fck_input_data* states, fckc_size_t size)
{
	(void)owner;

	for (fckc_size_t index = 0; index < size; index++)
	{
		fckc_u32* id = ids + index;
		fck_input_data* state = states + index;
		if (*id >= fck_pkey_count)
		{
			fckc_size_t last = size - 1;
			fck_input_data* last_state = states + last;
			fckc_u32* last_id = ids + last;
			*state = *last_state;

			*id = *last_id;
			size = size - 1;
			index = index - 1;
			continue;
		}

		const fck_input_data* data = input_source_physical_keyboard.data + *id;
		*state = *data;
	}
	return size;
}
