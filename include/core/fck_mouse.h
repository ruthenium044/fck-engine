#ifndef FCK_MOUSE_INCLUDED
#define FCK_MOUSE_INCLUDED

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_mouse.h>

struct fck_mouse_state_frame
{
	float cursor_position_x;
	float cursor_position_y;

	float scroll_delta_x;
	float scroll_delta_y;

	Uint32 button_state;
};

enum
{
	FCK_MOUSE_STATE_FRAME_COUNT = 2
};

struct fck_mouse_state
{
	// 0 = current; 1 = previous; ...
	union {
		fck_mouse_state_frame frames[FCK_MOUSE_STATE_FRAME_COUNT];

		struct
		{
			fck_mouse_state_frame current;
			fck_mouse_state_frame previous;
			// ...
		};
	};
};

// TODO: Put in source file

inline void fck_mouse_state_update(fck_mouse_state *mouse_state, float accumulated_delta_x, float accumulated_delta_y)
{
	mouse_state->previous = mouse_state->current;

	float *x = &mouse_state->current.cursor_position_x;
	float *y = &mouse_state->current.cursor_position_y;
	mouse_state->current.button_state = SDL_GetMouseState(x, y);

	mouse_state->current.scroll_delta_x = accumulated_delta_x;
	mouse_state->current.scroll_delta_y = accumulated_delta_y;
}

inline void fck_mouse_state_update_empty(fck_mouse_state *mouse_state)
{
	mouse_state->previous = mouse_state->current;

	float *x = &mouse_state->current.cursor_position_x;
	float *y = &mouse_state->current.cursor_position_y;
	mouse_state->current.button_state = 0;

	mouse_state->current.scroll_delta_x = 0.0f;
	mouse_state->current.scroll_delta_y = 0.f;
}

inline bool fck_button_down(fck_mouse_state const *mouse_state, int button_index, int frame_index = 0)
{
	SDL_assert(frame_index < FCK_MOUSE_STATE_FRAME_COUNT && "Cannot go that much back in time - Should we fallback to previous?");
	
	int button_mask = SDL_BUTTON_MASK(button_index);
	return (mouse_state->frames[frame_index].button_state & button_mask) == button_mask;
}

inline bool fck_button_up(fck_mouse_state const *mouse_state, int button_index, int frame_index = 0)
{
	return !fck_button_down(mouse_state, button_index, frame_index);
}

inline bool fck_button_just_down(fck_mouse_state const *mouse_state, int button_index)
{
	return fck_button_down(mouse_state, button_index, 0) && !fck_button_down(mouse_state, button_index, 1);
}

#endif // FCK_MOUSE_STATE_INCLUDED