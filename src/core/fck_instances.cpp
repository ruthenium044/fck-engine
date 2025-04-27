#include "core/fck_instances.h"
#include "core/fck_drop_file.h"
#include "core/fck_engine.h"
#include "core/fck_keyboard.h"
#include "core/fck_mouse.h"

#include "fck_ui.h"

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#include "nuklear.h"

void fck_instances_alloc(fck_instances *instances, uint8_t capacity)
{
	SDL_assert(instances != nullptr);
	fck_dense_list_alloc(&instances->data, capacity);
	fck_dense_list_alloc(&instances->pending_destroyed, capacity);
}

void fck_instances_free(fck_instances *instances)
{
	fck_dense_list_free(&instances->pending_destroyed);
	SDL_assert(instances != nullptr);
	for (fck_instance *instance : instances)
	{
		fck_instance_free(instance);
	}
	fck_dense_list_free(&instances->data);
}

bool fck_instances_exists(fck_instances *instances, SDL_WindowID const *window_id)
{
	SDL_assert(instances != nullptr);
	for (fck_instance *instance : instances)
	{
		if (instance->window_id == *window_id)
		{
			return true;
		}
	}
	return false;
}

bool fck_instances_any_active(fck_instances *instances)
{
	SDL_assert(instances != nullptr);

	bool is_any_instance_running = false;
	for (fck_instance *instance : instances)
	{
		is_any_instance_running |= instance->engine->is_running;
	}
	return is_any_instance_running;
}

void fck_instances_add(fck_instances *instances, fck_instance_info const *info, fck_instance_setup_function instance_setup)
{
	SDL_assert(instances != nullptr);

	fck_instance instance;
	fck_instance_alloc(&instance, info, instance_setup);
	SDL_WindowID id = SDL_GetWindowID(instance.engine->window);

	CHECK_ERROR(!fck_instances_exists(instances, &id), "Instances with duplicate window IDs detected");
	instance.window_id = id;
	fck_dense_list_add(&instances->data, &instance);
}

fck_instance *fck_instances_view(fck_instances *instances, SDL_WindowID const *window_id)
{
	SDL_assert(instances != nullptr);
	for (fck_instance *instance : instances)
	{
		if (instance->window_id == *window_id)
		{
			return instance;
		}
	}
	return nullptr;
}

void fck_instances_remove(fck_instances *instances, SDL_WindowID const *window_id)
{
	SDL_assert(instances != nullptr);

	for (size_t index = 0; index < instances->data.count; index++)
	{
		fck_instance *instance = fck_dense_list_view(&instances->data, index);
		if (instance->window_id == *window_id)
		{
			// Hide the window before destroying it so MacOS deals with it better
			CHECK_WARNING(SDL_HideWindow(instance->engine->window), SDL_GetError());
			fck_instance_free(instance);
			fck_dense_list_remove(&instances->data, index);
			return;
		}
	}

	// Log warning?
}

void fck_instances_process_events(fck_instances *instances)
{
	float scroll_delta_x = 0.0f;
	float scroll_delta_y = 0.0f;

	// We defer killing instances by about one tick
	// Ok, it's exactly one tick. This makes sure we do not kill it during event polling
	for (SDL_WindowID *id : &instances->pending_destroyed)
	{
		fck_instances_remove(instances, id);
	}
	fck_dense_list_clear(&instances->pending_destroyed);

	{
		// Poll events and filter per instance
		SDL_Event ev;
		while (SDL_PollEvent(&ev))
		{
			switch (ev.type)
			{
			case SDL_EVENT_KEY_UP:
			case SDL_EVENT_KEY_DOWN:
			case SDL_EVENT_MOUSE_BUTTON_UP:
			case SDL_EVENT_MOUSE_BUTTON_DOWN:
			case SDL_EVENT_MOUSE_MOTION:
			case SDL_EVENT_TEXT_INPUT:
			case SDL_EVENT_MOUSE_WHEEL: {
				fck_instance *instance = fck_instances_view(instances, &ev.drop.windowID);
				if (instance != nullptr)
				{
					fck_queue_push(&instance->event_queue, &ev);
				}
			}
			break;
			default:
				for (fck_instance *instance : instances)
				{
					fck_queue_push(&instance->event_queue, &ev);
				}
				break;
			}
		}
	}

	for (fck_instance *instance : instances)
	{
		fck_ecs *ecs = &instance->ecs;
		fck_engine *engine = fck_ecs_unique_view<fck_engine>(ecs);
		fck_keyboard_state *keyboard = fck_ecs_unique_view<fck_keyboard_state>(ecs);
		fck_mouse_state *mouse = fck_ecs_unique_view<fck_mouse_state>(ecs);
		SDL_WindowFlags window_flags = SDL_GetWindowFlags(engine->window);
		fck_ui *ui = fck_ecs_unique_view<fck_ui>(&instance->ecs);

		if (ui != nullptr)
		{
			nk_input_begin(ui->ctx);
		}

		SDL_Event *target_event;
		while (fck_queue_try_pop(&instance->event_queue, &target_event))
		{
			switch (target_event->type)
			{
			case SDL_EventType::SDL_EVENT_QUIT:
				// Can we come up with a cool little quit way? IDK :-D
				// engine->is_running = false;
				break;
			case SDL_EventType::SDL_EVENT_WINDOW_CLOSE_REQUESTED: {
				fck_dense_list_add(&instances->pending_destroyed, &target_event->window.windowID);
			}
			break;
			case SDL_EventType::SDL_EVENT_DROP_FILE: {
				fck_drop_file_context *drop_file_context = fck_ecs_unique_view<fck_drop_file_context>(&instance->ecs);
				fck_drop_file_context_notify(drop_file_context, &target_event->drop);
			}
			break;
			case SDL_EventType::SDL_EVENT_MOUSE_WHEEL:
				scroll_delta_x = scroll_delta_x + target_event->wheel.x;
				scroll_delta_y = scroll_delta_y + target_event->wheel.y;
				break;
			default:
				break;
			}
			if ((window_flags & SDL_WINDOW_INPUT_FOCUS) == SDL_WINDOW_INPUT_FOCUS)
			{
				fck_ui_handle_grab(&instance->ecs);
				fck_ui_handle_event(&instance->ecs, target_event);
			}
		}
		if (ui != nullptr)
		{
			nk_input_end(ui->ctx);
		}

		if ((window_flags & SDL_WINDOW_INPUT_FOCUS) == SDL_WINDOW_INPUT_FOCUS)
		{
			fck_keyboard_state_update(keyboard);
			fck_mouse_state_update(mouse, scroll_delta_x, scroll_delta_y);
		}
		else
		{
			fck_keyboard_state_update_empty(keyboard);
			fck_mouse_state_update_empty(mouse);
		}
	}
}
