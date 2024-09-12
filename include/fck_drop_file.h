#ifndef FCK_DROP_FILE_INCLUDED
#define FCK_DROP_FILE_INCLUDED

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

#endif // !FCK_DROP_FILE_INCLUDED