#ifndef FCK_TUPLE_INCLUDED
#define FCK_TUPLE_INCLUDED

#include "fck_template_utility.h"

template <typename... Ts>
struct fck_tuple
{
};

template <typename T>
struct fck_tuple<T>
{
	using type = T;
	T value;
};

template <typename T, typename... Ts>
struct fck_tuple<T, Ts...> : fck_tuple<Ts>...
{
	using type = T;
	T value;
};

template <typename result_type, typename... types>
result_type fck_get(fck_tuple<types...> const *tuple)
{
	static_assert(sizeof...(types) > 0, "Type is not part of tuple!");
	return {};
}

template <typename result_type, typename... types>
result_type fck_get(fck_tuple<types...> const *tuple, fck_true_type)
{
	return tuple->value;
}

template <typename result_type, typename type, typename... types>
result_type fck_get(fck_tuple<type, types...> const *tuple, fck_false_type)
{
	return fck_get<result_type, types...>((fck_tuple<types...> const *)tuple);
}

template <typename result_type, typename type, typename... types>
result_type fck_get(fck_tuple<type, types...> const *tuple)
{
	static constexpr bool is_result = fck_is_same<result_type, type>::value;
	return fck_get<result_type, type, types...>(tuple, typename fck_conditional<is_result, fck_true_type, fck_false_type>::type{});
}

template <typename result_type, typename... types>
result_type *fck_view(fck_tuple<types...> *tuple)
{
	static_assert(sizeof...(types) > 0, "Type is not part of tuple!");
	return nullptr;
}

template <typename result_type, typename... types>
result_type *fck_view(fck_tuple<types...> *tuple, fck_true_type)
{
	return &tuple->value;
}

template <typename result_type, typename type, typename... types>
result_type *fck_view(fck_tuple<type, types...> *tuple, fck_false_type)
{
	return fck_view<result_type, types...>((fck_tuple<types...> *)tuple);
}

template <typename result_type, typename type, typename... types>
result_type *fck_view(fck_tuple<type, types...> *tuple)
{
	static constexpr bool is_result = fck_is_same<result_type, type>::value;
	return fck_view<result_type, type, types...>(tuple, typename fck_conditional<is_result, fck_true_type, fck_false_type>::type{});
}

#endif // FCK_TUPLE_INCLUDED