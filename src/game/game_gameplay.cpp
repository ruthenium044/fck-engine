#include "core/fck_mouse.h"
#include "game/game_systems.h"

#include "core/fck_engine.h"
#include "core/fck_keyboard.h"
#include "ecs/fck_ecs.h"
#include "game/game_components.h"
#include "game/game_core.h"

void game_authority_controllable_create(fck_ecs *ecs, fck_system_once_info *)
{
	fck_ecs::entity_type cammy = game_controllable_create(ecs);

	// These are not getting replicated
	game_control_layout *layout = fck_ecs_component_create<game_control_layout>(ecs, cammy);
	*layout = {SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, 1};
}

void game_input_process(fck_ecs *ecs, fck_system_update_info *)
{
	fck_keyboard_state *keyboard = fck_ecs_unique_view<fck_keyboard_state>(ecs);
	fck_mouse_state *mouse = fck_ecs_unique_view<fck_mouse_state>(ecs);

	fck_ecs_apply(ecs, [keyboard, mouse](game_control_layout *layout, game_controller *controller) {
		int input_flag = 0;

		if (fck_key_down(keyboard, layout->left))
		{
			input_flag = input_flag | FCK_INPUT_FLAG_LEFT;
		}
		if (fck_key_down(keyboard, layout->right))
		{
			input_flag = input_flag | FCK_INPUT_FLAG_RIGHT;
		}
		if (fck_key_down(keyboard, layout->down))
		{
			input_flag = input_flag | FCK_INPUT_FLAG_DOWN;
		}
		if (fck_key_down(keyboard, layout->up))
		{
			input_flag = input_flag | FCK_INPUT_FLAG_UP;
		}
		if (fck_button_just_down(mouse, layout->shoot_mouse))
		{
			input_flag = input_flag | FCK_INPUT_FLAG_SHOOT;
		}
		controller->input = (game_input_flag)input_flag;
	});
}

struct game_projectile
{
	float vx;
	float vy;
};

void game_create_projectile(fck_ecs *ecs, float x, float y)
{
	fck_mouse_state *mouse = fck_ecs_unique_view<fck_mouse_state>(ecs);

	float vx = mouse->current.cursor_position_x - x;
	float vy = mouse->current.cursor_position_y - y;
	float d = vx * vx + vy * vy;
	if (d > 0.0f)
	{
		d = SDL_sqrtf(d);
		vx = vx / d;
		vy = vy / d;
	}

	vx = vx * 64.0f;
	vy = vy * 64.0f;

	fck_ecs::entity_type e = fck_ecs_entity_create(ecs);
	*fck_ecs_component_create<game_position>(ecs, e) = {x - 4.0f, y - 4.0f};
	*fck_ecs_component_create<game_size>(ecs, e) = {8.0f, 8.0f};
	*fck_ecs_component_create<game_debug_colour>(ecs, e) = {0, 255, 0, 255};
	*fck_ecs_component_create<game_projectile>(ecs, e) = {vx, vy};
}

void game_gameplay_process(fck_ecs *ecs, fck_system_update_info *)
{
	fck_engine *engine = fck_ecs_unique_view<fck_engine>(ecs);
	fck_mouse_state *mouse = fck_ecs_unique_view<fck_mouse_state>(ecs);

	// STATE EVAL
	// Movement
	fck_ecs_apply(ecs, [mouse, ecs](game_controller *controller, game_position *position, game_size *size) {
		game_input_flag input_flag = controller->input;
		{
			float direction_x = 0.0f;
			if ((input_flag & FCK_INPUT_FLAG_RIGHT) == FCK_INPUT_FLAG_RIGHT)
			{
				direction_x = direction_x + 1.0f;
			}
			if ((input_flag & FCK_INPUT_FLAG_LEFT) == FCK_INPUT_FLAG_LEFT)
			{
				direction_x = direction_x - 1.0f;
			}

			float direction_y = 0.0f;
			if ((input_flag & FCK_INPUT_FLAG_DOWN) == FCK_INPUT_FLAG_DOWN)
			{
				direction_y = direction_y + 1.0f;
			}
			if ((input_flag & FCK_INPUT_FLAG_UP) == FCK_INPUT_FLAG_UP)
			{
				direction_y = direction_y - 1.0f;
			}
			float length = (direction_x * direction_x) + (direction_y * direction_y);
			if (length > 0.0f)
			{
				length = SDL_sqrtf(length);
				direction_x = direction_x / length;
				direction_y = direction_y / length;
			}

			float speed = 2.0f;
			position->x = position->x + (direction_x * speed);
			position->y = position->y + (direction_y * speed);

			if ((input_flag & FCK_INPUT_FLAG_SHOOT) == FCK_INPUT_FLAG_SHOOT)
			{
				game_create_projectile(ecs, position->x + (size->w / 2), position->y + (size->h / 2));
			}
		}
	});

	fck_time *time = fck_ecs_unique_view<fck_time>(ecs);
	float dt_ms = time->delta / 1000.0f;

	fck_ecs_apply(ecs, [dt_ms](game_position *position, game_projectile *projectile) {
		position->x = position->x + (projectile->vx * dt_ms);
		position->y = position->y + (projectile->vy * dt_ms);
	});
}