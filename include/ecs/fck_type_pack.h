#ifndef FCK_TYPE_PACK_INCLUDED
#define FCK_TYPE_PACK_INCLUDED

template <typename... Ts>
struct fck_type_pack
{
};

template <typename T>
struct fck_type_pack<T>
{
	using type = T;
};

template <typename T, typename... Ts>
struct fck_type_pack<T, Ts...> : fck_type_pack<Ts>...
{
	using type = T;
};

// Also for tuples?
template <typename type>
struct fck_pop_front;

template <typename value_type, typename... types>
struct fck_pop_front<fck_type_pack<value_type, types...>>
{
	using result = fck_type_pack<types...>;
};

#endif // FCK_TYPE_PACK_INCLUDED