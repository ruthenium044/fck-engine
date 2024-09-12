#ifndef FCK_KEYBOARD_INCLUDED
#define FCK_KEYBOARD_INCLUDED

#include <SDL3/SDL_keyboard.h>

struct fck_keyboard_state_frame
{
	SDL_bool state[SDL_NUM_SCANCODES];
};

enum fck_keyboard_state_frame_count
{
	FCK_KEYBOARD_STATE_FRAME_COUNT = 2
};

struct fck_keyboard_state
{
	union {
		fck_keyboard_state_frame state[FCK_KEYBOARD_STATE_FRAME_COUNT];

		struct
		{
			fck_keyboard_state_frame current;
			fck_keyboard_state_frame previous;
			// ...
		};
	};
};

// TODO: Put in source file

inline void fck_keyboard_state_update(fck_keyboard_state *keyboard_state)
{
	int num_keys = 0;
	SDL_bool const *current_state = SDL_GetKeyboardState(&num_keys);

	SDL_memcpy((SDL_bool *)keyboard_state->previous.state, (SDL_bool *)keyboard_state->current.state, num_keys);

	SDL_memcpy((SDL_bool *)keyboard_state->current.state, (SDL_bool *)current_state, num_keys);
}

inline bool fck_key_down(fck_keyboard_state const *keyboard_state, SDL_Scancode scancode)
{
	return keyboard_state->current.state[(size_t)scancode];
}

inline bool fck_key_up(fck_keyboard_state const *keyboard_state, SDL_Scancode scancode)
{
	return !fck_key_down(keyboard_state, scancode);
}

#endif // !FCK_KEYBOARD_INCLUDED
