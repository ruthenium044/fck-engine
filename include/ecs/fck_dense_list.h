#ifndef FCK_DENSE_LIST_INCLUDED
#define FCK_DENSE_LIST_INCLUDED

#include "fck_iterator.h"
#include "fck_template_utility.h"
#include "SDL3/SDL_assert.h"

template <typename index_type, typename T>
struct fck_dense_list
{
	using index_info = fck_indexer_info<index_type>;
	static_assert(index_info::is_implemented, "Index type not implemented");

	T *data;
	index_type capacity;
	index_type count;
};

template <typename index_type, typename T>
void fck_dense_list_alloc(fck_dense_list<index_type, T> *list, typename fck_ignore_deduction<index_type>::type capacity)
{
	SDL_assert(list != nullptr);
	SDL_zerop(list);

	list->capacity = capacity;
	list->data = (T *)SDL_malloc(sizeof(T) * capacity);
}

template <typename index_type, typename T>
void fck_dense_list_free(fck_dense_list<index_type, T> *list)
{
	SDL_assert(list != nullptr);
	SDL_free(list->data);
	SDL_zerop(list);
}

template <typename index_type, typename T>
void fck_dense_list_assert_in_range(fck_dense_list<index_type, T> *list, typename fck_ignore_deduction<index_type>::type at)
{
	SDL_assert(list != nullptr);
	SDL_assert(list->data != nullptr);
	SDL_assert(list->count > at);
}

template <typename index_type, typename T>
void fck_dense_list_set(fck_dense_list<index_type, T> *list, typename fck_ignore_deduction<index_type>::type at, T const *value)
{
	fck_dense_list_assert_in_range(list, at);

	list->data[at] = *value;
}

template <typename index_type, typename T>
void fck_dense_list_set_empty(fck_dense_list<index_type, T> *list, typename fck_ignore_deduction<index_type>::type at)
{
	fck_dense_list_assert_in_range(list, at);

	SDL_zerop(list->data + at);
}

template <typename index_type, typename T>
T *fck_dense_list_view(fck_dense_list<index_type, T> *list, typename fck_ignore_deduction<index_type>::type at)
{
	fck_dense_list_assert_in_range(list, at);

	return &list->data[at];
}

template <typename index_type>
void *fck_dense_list_view_raw(fck_dense_list<index_type, void> *list, typename fck_ignore_deduction<index_type>::type at,
                              size_t type_size_in_bytes)
{
	fck_dense_list_assert_in_range(list, at);

	return (uint8_t *)list->data + (at * type_size_in_bytes);
}

template <typename index_type, typename T>
T fck_dense_list_get(fck_dense_list<index_type, T> *list, typename fck_ignore_deduction<index_type>::type at)
{
	return *fck_dense_list_view(list, at);
}

template <typename index_type, typename T>
void fck_dense_list_add(fck_dense_list<index_type, T> *list, typename fck_ignore_deduction<T>::type const *value)
{
	SDL_assert(list != nullptr);
	SDL_assert(list->data != nullptr);

	index_type last = list->count;
	list->count = last + 1;

	fck_dense_list_assert_in_range(list, last);
	list->data[last] = *value;
}

template <typename index_type>
void *fck_dense_list_reserve_raw(fck_dense_list<index_type, void> *list, size_t type_size_bytes)
{
	SDL_assert(list != nullptr);
	SDL_assert(list->data != nullptr);

	index_type last = list->count;
	list->count = last + 1;

	fck_dense_list_assert_in_range(list, last);

	uint8_t *start_as_byte = (uint8_t *)list->data;
	size_t offset_last = last * type_size_bytes;
	return start_as_byte + offset_last;
}

template <typename index_type>
void fck_dense_list_add_raw(fck_dense_list<index_type, void> *list, typename fck_ignore_deduction<void>::type const *value,
                            size_t type_size_bytes)
{
	void *ptr_last = fck_dense_list_reserve_raw(list, type_size_bytes);

	SDL_memcpy(ptr_last, value, type_size_bytes);
}

template <typename index_type, typename T>
void fck_dense_list_add_empty(fck_dense_list<index_type, T> *list)
{
	SDL_assert(list != nullptr);
	SDL_assert(list->data != nullptr);

	index_type last = list->count;
	list->count = last + 1;
	fck_dense_list_assert_in_range(list, last);
	SDL_zerop(list->data + last);
}

template <typename index_type, typename T>
void fck_dense_list_swap(fck_dense_list<index_type, T> *list, typename fck_ignore_deduction<index_type>::type lhs,
                         typename fck_ignore_deduction<index_type>::type rhs)
{
	SDL_assert(list != nullptr);
	SDL_assert(list->data != nullptr);
	fck_dense_list_assert_in_range(list, lhs);
	fck_dense_list_assert_in_range(list, rhs);

	T temp = list->data[lhs];
	list->data[lhs] = list->data[rhs];
	list->data[rhs] = temp;
}

template <typename index_type, typename T>
void fck_dense_list_remove(fck_dense_list<index_type, T> *list, typename fck_ignore_deduction<index_type>::type at)
{
	SDL_assert(list != nullptr);
	SDL_assert(list->data != nullptr);
	fck_dense_list_assert_in_range(list, at);

	// TODO: Ash suggested to not swap, instead just overwrite
	// Let's do that!
	index_type last = list->count - 1;
	fck_dense_list_swap(list, at, last);
	list->count = last;
}

// It comforts me a bit more that the size is coming from the caller-side, but should it?
// This is an exercise for the reader of this comment
template <typename index_type>
void fck_dense_list_remove_raw(fck_dense_list<index_type, void> *list, typename fck_ignore_deduction<index_type>::type at,
                               size_t type_size_bytes)
{
	SDL_assert(list != nullptr);
	SDL_assert(list->data != nullptr);
	fck_dense_list_assert_in_range(list, at);

	index_type last = list->count - 1;

	uint8_t *start_as_byte = (uint8_t *)list->data;
	size_t offset_current = at * type_size_bytes;
	size_t offset_last = last * type_size_bytes;
	uint8_t *ptr_at = start_as_byte + offset_current;
	uint8_t *ptr_last = start_as_byte + offset_last;

	SDL_memcpy(ptr_at, ptr_last, type_size_bytes);
	SDL_memset(ptr_last, 0, type_size_bytes);

	list->count = last;
}

template <typename index_type, typename T>
void fck_dense_list_clear(fck_dense_list<index_type, T> *list)
{
	SDL_assert(list != nullptr);
	SDL_assert(list->data != nullptr);

	list->count = 0;
}

template <typename index_type, typename value_type>
fck_iterator<value_type> begin(fck_dense_list<index_type, value_type> *list)
{
	return {list->data};
}

template <typename index_type, typename value_type>
fck_iterator<value_type> end(fck_dense_list<index_type, value_type> *list)
{
	return {list->data + list->count};
}

#endif // FCK_DENSE_LIST_INCLUDED