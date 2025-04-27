#ifndef GAME_COMPONENTS_INCLUDED
#define GAME_COMPONENTS_INCLUDED

#include "ecs/fck_ecs_component_traits.h"

#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_scancode.h>

#include "gen/gen_assets.h"

enum game_input_type
{
	FCK_INPUT_TYPE_LEFT,
	FCK_INPUT_TYPE_RIGHT,
	FCK_INPUT_TYPE_UP,
	FCK_INPUT_TYPE_DOWN,
	FCK_INPUT_TYPE_SHOOT,
};

enum game_input_flag
{
	FCK_INPUT_FLAG_ZERO = 0,
	FCK_INPUT_FLAG_LEFT = 1 << FCK_INPUT_TYPE_LEFT,
	FCK_INPUT_FLAG_RIGHT = 1 << FCK_INPUT_TYPE_RIGHT,
	FCK_INPUT_FLAG_UP = 1 << FCK_INPUT_TYPE_UP,
	FCK_INPUT_FLAG_DOWN = 1 << FCK_INPUT_TYPE_DOWN,

	FCK_INPUT_FLAG_SHOOT = 1 << FCK_INPUT_TYPE_SHOOT,
};

FCK_SERIALISE_OFF(game_control_layout)
struct game_control_layout
{
	SDL_Scancode left;
	SDL_Scancode right;
	SDL_Scancode up;
	SDL_Scancode down;

	int shoot_mouse;
};

struct game_controller
{
	game_input_flag input;
};

struct game_position
{
	float x;
	float y;
};

FCK_SERIALISE_OFF(game_cursor)
struct game_cursor
{
};

struct game_size
{
	float w;
	float h;
};

struct game_debug_colour
{
	uint8_t r, g, b, a;
};

struct game_sprite
{
	gen_png texture;
	SDL_FRect src;
};

// FCK_SERIALISE_TRAIT SECTION
#include "ecs/fck_serialiser.h"
inline void fck_serialise(fck_serialiser *serialiser, game_position *positions, size_t count)
{
	const size_t total_floats = count * 2;
	fck_serialise(serialiser, (float *)positions, total_floats);
}

// !FCK_SERIALISE_TRAIT SECTION

#endif // !GAME_COMPONENTS_INCLUDED