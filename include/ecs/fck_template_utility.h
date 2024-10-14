#ifndef FCK_TEMPLATE_UTILITY_INCLUDED
#define FCK_TEMPLATE_UTILITY_INCLUDED

#include "fck_type_pack.h"
#include <SDL3/SDL_stdinc.h>

// TODO: this file needs some cleanup for sure
template <bool B, class T, class F>
struct fck_conditional
{
	using type = T;
};

template <class T, class F>
struct fck_conditional<false, T, F>
{
	using type = F;
};

template <bool condition, typename T = void>
struct fck_enable_if
{
};

template <class T>
struct fck_enable_if<true, T>
{
	typedef T type;
};

template <class T, T v>
struct fck_constant
{
	static constexpr T value = v;
	using value_type = T;
	using type = fck_constant; // using injected-class-name
};

typedef fck_constant<bool, true> fck_true_type;
typedef fck_constant<bool, false> fck_false_type;

template <class T, class U>
struct fck_is_same : fck_false_type
{
};

template <class T>
struct fck_is_same<T, T> : fck_true_type
{
};

template <typename T>
struct fck_ignore_deduction
{
	typedef T type;
};

// We can probably infer all this info since unsigned MAX is always
// invalid/tombstone and the last bit indicates if it is a free index
// TODO: Let's maybe do that
template <class T>
struct fck_indexer_info
{
	static constexpr bool is_implemented = false;
};
template <>
struct fck_indexer_info<uint8_t>
{
	using type = uint8_t;
	static constexpr bool is_implemented = true;
	static constexpr uint8_t invalid = (uint8_t)~0u;
	static constexpr uint8_t free_mask = 1u << 7;
};
template <>
struct fck_indexer_info<uint16_t>
{
	using type = uint16_t;
	static constexpr bool is_implemented = true;
	static constexpr uint16_t invalid = (uint16_t)~0u;
	static constexpr uint16_t free_mask = 1u << 15;
};
template <>
struct fck_indexer_info<uint32_t>
{
	using type = uint32_t;
	static constexpr bool is_implemented = true;
	static constexpr uint32_t invalid = ~0u;
	static constexpr uint32_t free_mask = 1u << 31;
};
template <>
struct fck_indexer_info<uint64_t>
{
	using type = uint64_t;
	static constexpr bool is_implemented = true;
	static constexpr uint64_t invalid = ~0ul;
	static constexpr uint64_t free_mask = 1ul << 63;
};

template <typename pointer_type>
struct fck_remove_pointer
{
	typedef pointer_type type;
};
template <typename pointer_type>
struct fck_remove_pointer<pointer_type *>
{
	typedef pointer_type type;
};
template <typename pointer_type>
struct fck_remove_pointer<pointer_type *const>
{
	typedef pointer_type type;
};
template <typename pointer_type>
struct fck_remove_pointer<pointer_type *volatile>
{
	typedef pointer_type type;
};
template <typename pointer_type>
struct fck_remove_pointer<pointer_type *const volatile>
{
	typedef pointer_type type;
};

template <class T>
struct fck_is_pointer : fck_false_type
{
};

template <class T>
struct fck_is_pointer<T *> : fck_true_type
{
};

template <class T>
struct fck_is_pointer<T *const> : fck_true_type
{
};

template <class T>
struct fck_is_pointer<T *volatile> : fck_true_type
{
};

template <class T>
struct fck_is_pointer<T *const volatile> : fck_true_type
{
};

template <typename pointer_type>
using fck_remove_pointer_type = typename fck_remove_pointer<pointer_type>::type;

template <typename function>
struct fck_function_traits : fck_function_traits<decltype(&function::operator())>
{
};

template <typename return_t, typename... args>
struct fck_function_traits<return_t (*)(args...)>
{
	using caller_type = void;
	using return_type = return_t;
	using arguments = fck_type_pack<args...>;
	using arguments_no_pointer = fck_type_pack<fck_remove_pointer_type<args>...>;
	static constexpr size_t argument_count = sizeof...(args);
};

template <typename caller, typename return_t, typename... args>
struct fck_function_traits<return_t (caller::*)(args...) const>
{
	using caller_type = caller;
	using return_type = return_t;
	using arguments = fck_type_pack<args...>;
	using arguments_no_pointer = fck_type_pack<fck_remove_pointer_type<args>...>;

	static constexpr size_t argument_count = sizeof...(args);
};

// Call this one in user code to seriouslz fuck shit up
template <size_t>
uint16_t fck_count_up_and_get()
{
	static uint16_t counter = 0;
	counter++;
	return counter - 1;
}

#endif // FCK_TEMPLATE_UTILITY_INCLUDED