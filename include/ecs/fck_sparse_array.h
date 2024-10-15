#ifndef FCK_SPARSE_ARRAY_INCLUDED
#define FCK_SPARSE_ARRAY_INCLUDED

#include "fck_dense_list.h"
#include "fck_item.h"
#include "fck_sparse_lookup.h"

template <typename index_type, typename value_type>
struct fck_sparse_array
{
	using index_info = fck_indexer_info<index_type>;
	static_assert(index_info::is_implemented, "Index type not implemented");

	// Should free_list_next simply be another lookup to keep it user friendly?
	// if free bit is flipped, it is a free_list_next
	fck_sparse_lookup<index_type, index_type> sparse;

	// We lazily make use of the deterministic nature of owner and dense
	// Let's just hope they stay deterministic
	fck_dense_list<index_type, index_type> owner;
	fck_dense_list<index_type, value_type> dense;
};

template <typename index_type, typename value_type>
void fck_sparse_array_alloc(fck_sparse_array<index_type, value_type> *array, typename fck_ignore_deduction<index_type>::type capacity)
{
	SDL_assert(array != nullptr);
	SDL_zerop(array);

	using index_info = typename fck_sparse_array<index_type, value_type>::index_info;

	fck_sparse_lookup_alloc(&array->sparse, capacity, index_info::invalid);

	fck_dense_list_alloc(&array->owner, capacity);
	fck_dense_list_alloc(&array->dense, capacity);
}

template <typename index_type, typename value_type>
void fck_sparse_array_free(fck_sparse_array<index_type, value_type> *list)
{
	SDL_assert(list != nullptr);
	SDL_zerop(list);

	fck_sparse_lookup_free(&list->sparse);

	fck_dense_list_free(&list->owner);
	fck_dense_list_free(&list->dense);
}

template <typename index_type, typename value_type>
bool fck_sparse_array_exists(fck_sparse_array<index_type, value_type> *list, typename fck_ignore_deduction<index_type>::type index)
{
	SDL_assert(list != nullptr);

	static constexpr index_type invalid = fck_sparse_array<index_type, value_type>::index_info::invalid;

	return fck_sparse_lookup_get(&list->sparse, index) != invalid;
}

template <typename index_type, typename value_type>
value_type *fck_sparse_array_view(fck_sparse_array<index_type, value_type> *list, typename fck_ignore_deduction<index_type>::type index)
{
	SDL_assert(list != nullptr);

	const index_type dense_index = fck_sparse_lookup_get(&list->sparse, index);

	static constexpr index_type invalid = fck_sparse_array<index_type, value_type>::index_info::invalid;
	if (dense_index != invalid)
	{
		return fck_dense_list_view(&list->dense, dense_index);
	}
	return nullptr;
}

template <typename index_type, typename value_type>
bool fck_sparse_array_try_view(fck_sparse_array<index_type, value_type> *list, typename fck_ignore_deduction<index_type>::type index,
                               value_type **out_value)
{
	SDL_assert(list != nullptr);

	const index_type dense_index = fck_sparse_lookup_get(&list->sparse, index);

	static constexpr index_type invalid = fck_sparse_array<index_type, value_type>::index_info::invalid;
	if (dense_index != invalid)
	{
		*out_value = fck_dense_list_view(&list->dense, dense_index);
		return true;
	}
	return false;
}

template <typename index_type, typename value_type>
value_type *fck_sparse_array_emplace(fck_sparse_array<index_type, value_type> *list, typename fck_ignore_deduction<index_type>::type index,
                                     value_type const *value)
{
	SDL_assert(list != nullptr);

	static constexpr index_type invalid = fck_sparse_array<index_type, value_type>::index_info::invalid;

	index_type *dense_index = fck_sparse_lookup_view(&list->sparse, index);

	if (*dense_index == invalid)
	{
		*dense_index = list->dense.count;
		fck_dense_list_add(&list->dense, value);
		fck_dense_list_add(&list->owner, &index);
		return fck_dense_list_view(&list->dense, *dense_index);
	}

	// Set dense data
	value_type *entry = fck_dense_list_view(&list->dense, *dense_index);
	*entry = *value;
	return entry;
}

template <typename index_type, typename value_type>
void fck_sparse_array_emplace_empty(fck_sparse_array<index_type, value_type> *list, typename fck_ignore_deduction<index_type>::type index)
{
	SDL_assert(list != nullptr);

	static constexpr index_type invalid = fck_sparse_array<index_type, value_type>::index_info::invalid;
	static constexpr index_type free_mask = fck_sparse_array<index_type, value_type>::index_info::free_mask;

	index_type *dense_index = fck_sparse_lookup_view(&list->sparse, index);

	if (*dense_index == invalid)
	{
		*dense_index = list->dense.count;
		fck_dense_list_add_empty(&list->dense);
		fck_dense_list_add(&list->owner, &index);
		return;
	}

	// Set dense data
	fck_dense_list_set_empty(&list->dense, *dense_index);
}

template <typename index_type, typename value_type>
void fck_sparse_array_remove(fck_sparse_array<index_type, value_type> *list, typename fck_ignore_deduction<index_type>::type index)
{
	SDL_assert(list != nullptr);

	static constexpr index_type invalid = fck_sparse_array<index_type, value_type>::index_info::invalid;

	index_type dense_index = fck_sparse_lookup_get(&list->sparse, index);

	index_type last_owner = fck_dense_list_get(&list->owner, list->dense.count - 1);

	// Update structure
	fck_dense_list_remove(&list->dense, dense_index);
	fck_dense_list_remove(&list->owner, dense_index);
	fck_sparse_lookup_set(&list->sparse, last_owner, &dense_index);
	fck_sparse_lookup_set(&list->sparse, index, &invalid);
}

template <typename index_type, typename value_type>
fck_iterator<fck_item<index_type, value_type>> begin(fck_sparse_array<index_type, value_type> *list)
{
	fck_iterator<fck_item<index_type, value_type>> iterator;
	iterator.index = list->owner.data;
	iterator.value = list->dense.data;
	return iterator;
}

template <typename index_type, typename value_type>
fck_iterator<fck_item<index_type, value_type>> end(fck_sparse_array<index_type, value_type> *list)
{
	fck_iterator<fck_item<index_type, value_type>> iterator;
	iterator.index = list->owner.data + list->owner.count;
	iterator.value = list->dense.data + list->dense.count;
	return iterator;
}

#endif // FCK_SPARSE_ARRAY_INCLUDED