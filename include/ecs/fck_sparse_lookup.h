#ifndef FCK_SPARSE_LOOKUP_INCLUDED
#define FCK_SPARSE_LOOKUP_INCLUDED

#include "fck_template_utility.h"

template <typename index_type, typename value_type>
struct fck_sparse_lookup
{
	using index_info = fck_indexer_info<index_type>;
	static_assert(index_info::is_implemented, "Index type not implemented");

	value_type *data;
	index_type capacity;

	value_type tombstone;
};

template <typename index_type, typename value_type>
void fck_sparse_lookup_alloc(fck_sparse_lookup<index_type, value_type> *list, typename fck_ignore_deduction<index_type>::type capacity,
                             value_type tombstone = {})
{

	SDL_assert(list != nullptr);
	SDL_zerop(list);

	list->capacity = capacity;
	list->tombstone = tombstone;
	list->data = (value_type *)SDL_malloc(capacity * sizeof(value_type));
	for (size_t index = 0; index < list->capacity; index++)
	{
		list->data[index] = tombstone;
	}
}

template <typename index_type, typename value_type>
void fck_sparse_lookup_free(fck_sparse_lookup<index_type, value_type> *list)
{
	SDL_assert(list != nullptr);
	SDL_free(list->data);
	SDL_zerop(list);
}

template <typename index_type, typename value_type>
void fck_sparse_lookup_assert_in_range(fck_sparse_lookup<index_type, value_type> *list, typename fck_ignore_deduction<index_type>::type at)
{
	SDL_assert(list != nullptr);
	SDL_assert(list->data != nullptr);
	SDL_assert(list->capacity > at);
}

template <typename index_type, typename value_type>
void fck_sparse_lookup_set(fck_sparse_lookup<index_type, value_type> *list, typename fck_ignore_deduction<index_type>::type at,
                           value_type const *value)
{
	SDL_assert(list != nullptr);
	SDL_assert(list->data != nullptr);
	SDL_assert(list->capacity > at);
	fck_sparse_lookup_assert_in_range(list, at);

	list->data[at] = *value;
}

template <typename index_type, typename value_type>
value_type *fck_sparse_lookup_view(fck_sparse_lookup<index_type, value_type> *list, typename fck_ignore_deduction<index_type>::type at)
{
	SDL_assert(list != nullptr);
	SDL_assert(list->data != nullptr);
	SDL_assert(list->capacity > at);
	fck_sparse_lookup_assert_in_range(list, at);

	return &list->data[at];
}

template <typename index_type, typename value_type>
value_type fck_sparse_lookup_get(fck_sparse_lookup<index_type, value_type> *list, typename fck_ignore_deduction<index_type>::type at)
{
	return *fck_sparse_lookup_view(list, at);
}

#endif // FCK_SPARSE_LOOKUP_INCLUDED