#ifndef FCK_KEYBOARD_INCLUDED
#define FCK_KEYBOARD_INCLUDED

#include <SDL3/SDL_keyboard.h>

struct fck_keyboard_state
{
	SDL_bool current_state[SDL_NUM_SCANCODES];
	SDL_bool previous_state[SDL_NUM_SCANCODES];
};

// TODO: Put in source file

inline void fck_keyboard_state_update(fck_keyboard_state *keyboard_state)
{
	int num_keys = 0;
	SDL_bool const *current_state = SDL_GetKeyboardState(&num_keys);

	SDL_memcpy((SDL_bool *)keyboard_state->previous_state, (SDL_bool *)keyboard_state->current_state, num_keys);

	SDL_memcpy((SDL_bool *)keyboard_state->current_state, (SDL_bool *)current_state, num_keys);
}

inline bool fck_key_down(fck_keyboard_state const *keyboard_state, SDL_Scancode scancode)
{
	return keyboard_state->current_state[(size_t)scancode];
}

inline bool fck_key_up(fck_keyboard_state const *keyboard_state, SDL_Scancode scancode)
{
	return !fck_key_down(keyboard_state, scancode);
}

#endif // !FCK_KEYBOARD_INCLUDED
