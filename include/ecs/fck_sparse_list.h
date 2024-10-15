#ifndef FCK_SPARSE_LIST_INCLUDED
#define FCK_SPARSE_LIST_INCLUDED

#include "fck_dense_list.h"
#include "fck_item.h"
#include "fck_sparse_lookup.h"

template <typename index_type, typename value_type>
struct fck_sparse_list
{
	using index_info = fck_indexer_info<index_type>;
	static_assert(index_info::is_implemented, "Index type not implemented");

	// Should free_list_next simply be another lookup to keep it user friendly?
	// if free bit is flipped, it is a free_list_next
	fck_sparse_lookup<index_type, index_type> sparse;

	// We lazily make use of the deterministic nature of owner and dense
	// Let's just hope they stay deterministic - I should start tracking the nightmare parts of this code
	fck_dense_list<index_type, index_type> owner;
	fck_dense_list<index_type, value_type> dense;

	// Required for list semantics, but also to make the free list resilient to
	// using add and emplace at the same time. This is the wet dream data structure for the user
	// You get stable indices, can get a good index proposed by the list, but also can just wildly emplace
	fck_sparse_lookup<index_type, index_type> free_list_prev;
	index_type free_list_head;
};

template <typename index_type, typename value_type>
void fck_sparse_list_alloc(fck_sparse_list<index_type, value_type> *list, typename fck_ignore_deduction<index_type>::type capacity)
{
	SDL_assert(list != nullptr);
	SDL_zerop(list);
	using index_info = typename fck_sparse_list<index_type, value_type>::index_info;
	list->free_list_head = index_info::invalid;

	fck_sparse_lookup_alloc(&list->sparse, capacity, index_info::invalid);
	fck_sparse_lookup_alloc(&list->free_list_prev, capacity, index_info::invalid);

	fck_dense_list_alloc(&list->owner, capacity);
	fck_dense_list_alloc(&list->dense, capacity);

	// Construct a free list
	const index_type last = capacity - 1;
	for (index_type index = 0; index < capacity; index++)
	{
		index_type reverse_index = last - index;
		fck_sparse_lookup_set(&list->sparse, reverse_index, &list->free_list_head);
		list->free_list_head = reverse_index | index_info::free_mask;
	}
	index_type next = index_info::invalid;
	for (index_type index = 0; index < capacity; index++)
	{
		fck_sparse_lookup_set(&list->free_list_prev, index, &next);
		next = index;
	}
}

template <typename index_type, typename value_type>
void fck_sparse_list_free(fck_sparse_list<index_type, value_type> *list)
{
	SDL_assert(list != nullptr);
	SDL_zerop(list);

	fck_sparse_lookup_free(&list->sparse);
	fck_sparse_lookup_free(&list->free_list_prev);

	fck_dense_list_free(&list->owner);
	fck_dense_list_free(&list->dense);
}

template <typename index_type, typename value_type>
bool fck_sparse_list_exists(fck_sparse_list<index_type, value_type> *list, typename fck_ignore_deduction<index_type>::type *index)
{
	SDL_assert(list != nullptr);

	static constexpr index_type free_mask = fck_sparse_list<index_type, value_type>::index_info::free_mask;

	return (fck_sparse_lookup_get(list->sparse, index) & free_mask) != free_mask;
}

template <typename index_type, typename value_type>
bool fck_sparse_list_try_view(fck_sparse_list<index_type, value_type> *list, typename fck_ignore_deduction<index_type>::type *index,
                              value_type **out_value)
{
	SDL_assert(list != nullptr);

	static constexpr index_type free_mask = fck_sparse_list<index_type, value_type>::index_info::free_mask;

	const index_type dense_index = fck_sparse_lookup_get(list->sparse, index);
	if ((dense_index & free_mask) != free_mask)
	{
		*out_value = fck_dense_list_view(list->dense, dense_index);
		return true;
	}
	return false;
}

template <typename index_type, typename value_type>
void fck_sparse_list_emplace(fck_sparse_list<index_type, value_type> *list, typename fck_ignore_deduction<index_type>::type index,
                             value_type const *value)
{
	SDL_assert(list != nullptr);

	static constexpr index_type invalid = fck_sparse_list<index_type, value_type>::index_info::invalid;
	static constexpr index_type free_mask = fck_sparse_list<index_type, value_type>::index_info::free_mask;

	index_type *dense_index = fck_sparse_lookup_view(&list->sparse, index);

	if ((*dense_index & free_mask) == free_mask)
	{
		// Clean up free list - take it and use it
		index_type *prev = fck_sparse_lookup_view(&list->free_list_prev, index);
		index_type free_next = *dense_index;
		index_type next = free_next & ~free_mask;

		if (free_next != invalid)
		{
			fck_sparse_lookup_set(&list->free_list_prev, next, prev);
		}
		if (*prev != invalid)
		{
			fck_sparse_lookup_set(&list->sparse, *prev, &free_next);
		}
		else
		{
			list->free_list_head = free_next;
		}

		*dense_index = list->dense.count;
		fck_dense_list_add(&list->dense, value);
		fck_dense_list_add(&list->owner, &index);
		fck_sparse_lookup_set(&list->free_list_prev, index, &invalid);

		return;
	}

	// Set dense data
	fck_dense_list_set(&list->dense, *dense_index, value);
}

template <typename index_type, typename value_type>
index_type fck_sparse_list_add(fck_sparse_list<index_type, value_type> *list, value_type const *value)
{
	SDL_assert(list != nullptr);

	static constexpr index_type invalid = fck_sparse_list<index_type, value_type>::index_info::invalid;
	static constexpr index_type free_mask = fck_sparse_list<index_type, value_type>::index_info::free_mask;

	bool has_free_slot = list->free_list_head != invalid;
	// Pop free list and re-use a slot in the sparse list
	if (has_free_slot)
	{
		index_type sparse_index = list->free_list_head & ~free_mask;
		index_type *sparse_lookup_element = fck_sparse_lookup_view(&list->sparse, sparse_index);

		list->free_list_head = *sparse_lookup_element;
		*sparse_lookup_element = list->dense.count;
		fck_dense_list_add(&list->dense, value);
		fck_dense_list_add(&list->owner, &sparse_index);
		if (list->free_list_head != invalid)
		{
			fck_sparse_lookup_set(&list->free_list_prev, list->free_list_head & ~free_mask, &invalid);
		}
		return sparse_index;
	}
	// better error? Maybe resize, but I am sooo lazy
	// Maybe assert with "Double the capacity, lol"
	return invalid;
}

template <typename index_type, typename value_type>
void fck_sparse_list_remove(fck_sparse_list<index_type, value_type> *list, typename fck_ignore_deduction<index_type>::type index)
{
	SDL_assert(list != nullptr);

	static constexpr index_type invalid = fck_sparse_list<index_type, value_type>::index_info::invalid;
	static constexpr index_type free_mask = fck_sparse_list<index_type, value_type>::index_info::free_mask;

	index_type dense_index = fck_sparse_lookup_get(&list->sparse, index);

	index_type last_owner = fck_dense_list_get(&list->owner, list->dense.count - 1);

	// Update structure
	fck_dense_list_remove(&list->dense, dense_index);
	fck_dense_list_remove(&list->owner, dense_index);
	fck_sparse_lookup_set(&list->sparse, last_owner, &dense_index);

	// Update free list - set next and prev
	fck_sparse_lookup_set(&list->sparse, index, &list->free_list_head);
	if (list->free_list_head != invalid)
	{
		index_type next_index = list->free_list_head & ~free_mask;
		fck_sparse_lookup_set(&list->free_list_prev, next_index, &index);
	}

	index = index | free_mask;
	list->free_list_head = index;
}

template <typename index_type, typename value_type>
fck_iterator<fck_item<index_type, value_type>> begin(fck_sparse_list<index_type, value_type> *list)
{
	fck_iterator<fck_item<index_type, value_type>> iterator;
	iterator.index = list->owner.data;
	iterator.value = list->dense.data;
	return iterator;
}

template <typename index_type, typename value_type>
fck_iterator<fck_item<index_type, value_type>> end(fck_sparse_list<index_type, value_type> *list)
{
	fck_iterator<fck_item<index_type, value_type>> iterator;
	iterator.index = list->owner.data + list->owner.count;
	iterator.value = list->dense.data + list->dense.count;
	return iterator;
}

#endif // FCK_SPARSE_LIST_INCLUDED