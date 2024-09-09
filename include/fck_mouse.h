#ifndef FCK_MOUSE_INCLUDED
#define FCK_MOUSE_INCLUDED

#include <SDL3/SDL_mouse.h>

struct fck_mouse_state
{
	float current_x;
	float current_y;
	float previous_x;
	float previous_y;

	Uint32 current_button_state;
	Uint32 previous_button_state;
};

// TODO: Put in source file

inline void fck_mouse_state_update(fck_mouse_state *mouse_state)
{
	mouse_state->previous_x = mouse_state->current_x;
	mouse_state->previous_y = mouse_state->current_y;

	mouse_state->previous_button_state = mouse_state->current_button_state;
	mouse_state->current_button_state = SDL_GetMouseState(&mouse_state->current_x, &mouse_state->current_y);
}

inline bool fck_button_down(fck_mouse_state const *mouse_state, int button_index /* 1 ... n - Mice are weird*/)
{
	int button_mask = SDL_BUTTON(button_index);
	return (mouse_state->current_button_state & button_mask) == button_mask;
}

inline bool fck_button_up(fck_mouse_state const *mouse_state, int button_index /* 1 ... n - Mice are weird*/)
{
	return !fck_button_down(mouse_state, button_index);
}

#endif // FCK_MOUSE_STATE_INCLUDED