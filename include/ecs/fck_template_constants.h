#ifndef FCK_TEMPLATE_CONSTANTS_INCLUDED
#define FCK_TEMPLATE_CONSTANTS_INCLUDED

template <class T, T v>
struct fck_constant
{
	static constexpr T value = v;
	using value_type = T;
	using type = fck_constant; // using injected-class-name
};

typedef fck_constant<bool, true> fck_true_type;
typedef fck_constant<bool, false> fck_false_type;

#endif // FCK_TEMPLATE_CONSTANTS_INCLUDED