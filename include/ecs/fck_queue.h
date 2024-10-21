#ifndef FCK_QUEUE_INCLUDED
#define FCK_QUEUE_INCLUDED

#include "SDL3/SDL_assert.h"
#include "fck_iterator.h"
#include "fck_template_utility.h"

template <typename index_type, typename type>
struct fck_queue
{
	using index_info = fck_indexer_info<index_type>;
	static_assert(index_info::is_implemented, "Index type not implemented");

	type *data;
	index_type capacity;

	index_type head;
	index_type tail;
};

template <typename index_type, typename type>
void fck_queue_alloc(fck_queue<index_type, type> *queue, typename fck_ignore_deduction<index_type>::type capacity)
{
	SDL_assert(queue != nullptr);
	SDL_zerop(queue);

	queue->capacity = capacity;
	queue->data = (type *)SDL_malloc(sizeof(type) * capacity);
}

template <typename index_type, typename T>
void fck_queue_free(fck_queue<index_type, T> *list)
{
	SDL_assert(list != nullptr);
	SDL_free(list->data);
	SDL_zerop(list);
}

template <typename index_type, typename type>
void fck_queue_push(fck_queue<index_type, type> *queue, typename fck_ignore_deduction<type>::type const *value)
{
	SDL_assert(queue != nullptr);
	queue->data[queue->tail] = *value;
	queue->tail = (queue->tail + 1) % queue->capacity;
}

template <typename index_type, typename type>
bool fck_queue_try_pop(fck_queue<index_type, type> *queue, typename fck_ignore_deduction<type>::type **value)
{
	SDL_assert(queue != nullptr);
	if (queue->head == queue->tail)
	{
		return false;
	}
	*value = queue->data + queue->head;
	queue->head = (queue->head + 1) % queue->capacity;
	return true;
}

#endif // FCK_QUEUE_INCLUDED