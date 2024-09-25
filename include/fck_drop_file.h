#ifndef FCK_DROP_FILE_INCLUDED
#define FCK_DROP_FILE_INCLUDED

#include <SDL3/SDL_assert.h>

struct fck_drop_file_context;

typedef bool (*fck_try_receive_drop_file)(fck_drop_file_context const *context, SDL_DropEvent const *drop_event);

struct fck_drop_file_context
{
	fck_try_receive_drop_file *drop_events;

	size_t count;
	size_t capacity;
};

inline void fck_drop_file_context_allocate(fck_drop_file_context *drop_file_context, size_t initial_capacity)
{
	SDL_assert(drop_file_context != nullptr);
	SDL_assert(drop_file_context->drop_events == nullptr && "Already allocated");

	const size_t element_size = sizeof(*drop_file_context->drop_events);
	drop_file_context->drop_events = (fck_try_receive_drop_file *)SDL_calloc(initial_capacity, element_size);
	drop_file_context->capacity = initial_capacity;
	drop_file_context->count = 0;
}

inline bool fck_drop_file_context_push(fck_drop_file_context *drop_file_context, fck_try_receive_drop_file func)
{
	SDL_assert(drop_file_context != nullptr);
	SDL_assert(drop_file_context->drop_events != nullptr);

	if (drop_file_context->count >= drop_file_context->capacity)
	{
		return false;
	}

	drop_file_context->drop_events[drop_file_context->count] = func;
	drop_file_context->count = drop_file_context->count + 1;
	return true;
}

inline void fck_drop_file_context_notify(fck_drop_file_context *drop_file_context, SDL_DropEvent const *drop_event)
{
	SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Trying to load file: %s ...", drop_event->data);

	SDL_assert(drop_file_context != nullptr);
	SDL_assert(drop_file_context->drop_events != nullptr);

	for (size_t index = 0; index < drop_file_context->count; index++)
	{
		fck_try_receive_drop_file try_receive = drop_file_context->drop_events[index];
		bool result = try_receive(drop_file_context, drop_event);
		if (result)
		{
			break;
		}
	}
}

inline void fck_drop_file_context_free(fck_drop_file_context *drop_file_context)
{
	SDL_assert(drop_file_context != nullptr);
	SDL_assert(drop_file_context->drop_events != nullptr && "Non-sensical - Nothing to deallocate");

	const size_t element_size = sizeof(*drop_file_context->drop_events);
	SDL_free(drop_file_context->drop_events);
	drop_file_context->capacity = 0;
	drop_file_context->count = 0;
}

inline bool fck_drop_file_receive_png(fck_drop_file_context const *context, SDL_DropEvent const *drop_event)
{
	SDL_assert(context != nullptr);
	SDL_assert(drop_event != nullptr);

	SDL_IOStream *stream = SDL_IOFromFile(drop_event->data, "r");
	CHECK_ERROR(stream, SDL_GetError());
	if (!IMG_isPNG(stream))
	{
		// We only allow pngs for now!
		SDL_CloseIO(stream);
		return false;
	}
	const char resource_path_base[] = FCK_RESOURCE_DIRECTORY_PATH;

	const char *target_file_path = drop_event->data;

	// Spin till the end
	const char *target_file_name = SDL_strrchr(target_file_path, '\\');
	SDL_assert(target_file_name != nullptr && "Potential file name is directory?");
	target_file_name = target_file_name + 1;

	char path_buffer[512];
	SDL_zero(path_buffer);
	size_t added_length = SDL_strlcat(path_buffer, resource_path_base, sizeof(path_buffer));

	// There is actually no possible way the path is longer than 2024... Let's
	// just pray
	SDL_strlcat(path_buffer + added_length, target_file_name, sizeof(path_buffer));

	SDL_bool result = SDL_CopyFile(drop_event->data, path_buffer);
	CHECK_INFO(result, SDL_GetError());

	SDL_CloseIO(stream);

	return true;
}

#endif // !FCK_DROP_FILE_INCLUDED