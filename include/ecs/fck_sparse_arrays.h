#ifndef FCK_SPARSE_ARRAYS_INCLUDED
#define FCK_SPARSE_ARRAYS_INCLUDED

#include "ecs/fck_sparse_array.h"
#include "fck_tuple.h"

template <typename index_type, typename... types>
struct fck_sparse_arrays_tuple
{
	using type = fck_tuple<fck_sparse_array<index_type, types> *...>;
};

template <typename index_type, typename... types>
struct fck_sparse_arrays
{
	typename fck_sparse_arrays_tuple<index_type, types...>::type tuple;
};

template <typename index_type>
bool fck_sparse_arrays_tuple_is_valid_combine(fck_tuple<> *, typename fck_ignore_deduction<index_type>::type *)
{
	return true;
}

template <typename index_type, typename type, typename... types>
bool fck_sparse_arrays_tuple_is_valid_combine(
	fck_tuple<fck_sparse_array<index_type, type> *, fck_sparse_array<index_type, types> *...> *tuple,
	typename fck_ignore_deduction<index_type>::type *index)
{
	bool exists = fck_sparse_array_exists(tuple->value, *index);
	return exists && fck_sparse_arrays_tuple_is_valid_combine<index_type, types...>(
						 (fck_tuple<fck_sparse_array<index_type, types> *...> *)tuple, index);
}

template <typename index_type, typename... types>
bool fck_sparse_arrays_tuple_exists_combine(fck_tuple<fck_sparse_array<index_type, types> *...> *)
{
	return true;
}

template <typename index_type, typename type, typename... types>
bool fck_sparse_arrays_tuple_exists_combine(
	fck_tuple<fck_sparse_array<index_type, type> *, fck_sparse_array<index_type, types> *...> *tuple)
{

	bool exists = tuple->value != nullptr;
	return exists &&
	       fck_sparse_arrays_tuple_exists_combine<index_type, types...>((fck_tuple<fck_sparse_array<index_type, types> *...> *)tuple);
}

template <typename index_type, typename... types>
bool fck_sparse_arrays_is_valid(fck_sparse_arrays<index_type, types...> *array, typename fck_ignore_deduction<index_type>::type index)
{
	return fck_sparse_arrays_tuple_is_valid_combine<index_type, types...>(&array->tuple, &index);
}

template <typename functor, typename index_type, typename value_type, typename... types>
void fck_sparse_arrays_apply(fck_sparse_arrays<index_type, value_type, types...> *arrays, functor func)
{
	if (!fck_sparse_arrays_tuple_exists_combine(&arrays->tuple))
	{
		// Nothing to iterate! Set is empty! :)
		return;
	}

	fck_sparse_array<index_type, value_type> *array = arrays->tuple.value;
	for (fck_item<index_type, value_type> item : array)
	{
		index_type *index = item.index;
		value_type *value = item.value;

		// Since we receive a dense index for current column existence is implied
		using next_tuple = typename fck_sparse_arrays_tuple<index_type, types...>::type;
		if (fck_sparse_arrays_tuple_is_valid_combine<index_type>((next_tuple *)&arrays->tuple, index))
		{
			// Let's keep the design simple - by default iterations are read/write
			// That means the users ALWAYS get pointers and pointers of pointers, etc
			func(value, (fck_sparse_array_view(fck_get<fck_sparse_array<index_type, types> *>(&arrays->tuple), *index))...);
		}
	}
}

template <typename functor, typename index_type, typename value_type, typename... types>
void fck_sparse_arrays_apply_with_entity(fck_sparse_arrays<index_type, value_type, types...> *arrays, functor func)
{

	if (!fck_sparse_arrays_tuple_exists_combine(&arrays->tuple))
	{
		// Nothing to iterate! Set is empty! :)
		return;
	}
	// TODO: Select the one with the smallest count
	fck_sparse_array<index_type, value_type> *array = arrays->tuple.value;
	for (fck_item<index_type, value_type> item : array)
	{
		index_type *index = item.index;
		value_type *value = item.value;

		// Since we receive a dense index for current column existence is implied
		using next_tuple = typename fck_sparse_arrays_tuple<index_type, types...>::type;
		if (fck_sparse_arrays_tuple_is_valid_combine<index_type>((next_tuple *)&arrays->tuple, index))
		{
			// Let's keep the design simple - by default iterations are read/write
			// That means the users ALWAYS get pointers and pointers of pointers, etc
			func(*index, value, (fck_sparse_array_view(fck_get<fck_sparse_array<index_type, types> *>(&arrays->tuple), *index))...);
		}
	}
}

#endif // FCK_SPARSE_ARRAYS_INCLUDED