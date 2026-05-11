#ifndef FCK_GAMEPAD_INPUT_H_INCLUDED
#define FCK_GAMEPAD_INPUT_H_INCLUDED

typedef enum fck_gamepad_input_type
{
	fck_gamepad_none,

	fck_gamepad_left_stick,
	fck_gamepad_right_stick,

	fck_gamepad_left_trigger,
	fck_gamepad_right_trigger,

	fck_gamepad_north,
	fck_gamepad_east,
	fck_gamepad_south,
	fck_gamepad_west,

	fck_gamepad_left,
	fck_gamepad_right,
	fck_gamepad_up,
	fck_gamepad_down,

	fck_gamepad_left_1,
	fck_gamepad_right_1,
	fck_gamepad_left_2,
	fck_gamepad_right_2,
	fck_gamepad_left_3,
	fck_gamepad_right_3,
	fck_gamepad_select,
	fck_gamepad_options,

	fck_gamepad_home,

	fck_gamepad_count,

	// TODO: Maybe some convenience
	//fck_gamepad_any_left,
	//fck_gamepad_any_right,
	//fck_gamepad_any_up,
	//fck_gamepad_any_down,
} fck_gamepad_input_type;

#endif // !FCK_GAMEPAD_INPUT_H_INCLUDED
