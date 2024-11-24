#include "core/fck_drop_file.h"
#include "core/fck_engine.h"
#include "core/fck_instances.h"
#include "core/fck_keyboard.h"

void fck_instances_alloc(fck_instances *instances, uint8_t capacity)
{
	SDL_assert(instances != nullptr);
	fck_sparse_array_alloc(&instances->data, capacity);
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
	fck_sparse_array_free(&instances->data);
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

fck_instance *fck_instances_add(fck_instances *instances, fck_instance_info const *info, fck_instance_setup_function instance_setup)
{
	SDL_assert(instances != nullptr);

	fck_instance instance;
	fck_instance_alloc(&instance, info, instance_setup);
	SDL_WindowID id = SDL_GetWindowID(instance.engine->window);
	return fck_sparse_array_emplace(&instances->data, id, &instance);
}

fck_instance *fck_instances_view(fck_instances *instances, SDL_WindowID const *windowId)
{
	SDL_assert(instances != nullptr);
	return fck_sparse_array_view(&instances->data, *windowId);
}

void fck_instances_remove(fck_instances *instances, SDL_WindowID const *windowId)
{
	SDL_assert(instances != nullptr);

	SDL_WindowID id = *windowId;
	fck_instance *instance = fck_instances_view(instances, &id);
	// Hide the window before destroying it so MacOS deals with it better
	CHECK_WARNING(SDL_HideWindow(instance->engine->window), SDL_GetError());

	fck_instance_free(instance);
	fck_sparse_array_remove(&instances->data, id);
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
	SDL_Event ev;
	while (SDL_PollEvent(&ev))
	{
		switch (ev.type)
		{
		case SDL_EventType::SDL_EVENT_QUIT:
			// Can we come up with a cool little quit way? IDK :-D
			// engine->is_running = false;
			break;
		case SDL_EventType::SDL_EVENT_WINDOW_CLOSE_REQUESTED: {
			fck_dense_list_add(&instances->pending_destroyed, &ev.window.windowID);
		}
		break;
		case SDL_EventType::SDL_EVENT_DROP_FILE: {
			fck_instance *instance = fck_instances_view(instances, &ev.drop.windowID);
			fck_drop_file_context *drop_file_context = fck_ecs_unique_view<fck_drop_file_context>(&instance->ecs);
			fck_drop_file_context_notify(drop_file_context, &ev.drop);
		}
		break;
		case SDL_EventType::SDL_EVENT_MOUSE_WHEEL:
			scroll_delta_x = scroll_delta_x + ev.wheel.x;
			scroll_delta_y = scroll_delta_y + ev.wheel.y;
			break;
		default:
			break;
		}
	}

	for (fck_instance *instance : instances)
	{
		// Maybe we should cache these modules, maybe access is fast enough, maybe, maybe
		fck_ecs *ecs = &instance->ecs;
		fck_engine *engine = fck_ecs_unique_view<fck_engine>(ecs);
		fck_keyboard_state *keyboard = fck_ecs_unique_view<fck_keyboard_state>(ecs);
		fck_mouse_state *mouse = fck_ecs_unique_view<fck_mouse_state>(ecs);
		SDL_WindowFlags window_flags = SDL_GetWindowFlags(engine->window);

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