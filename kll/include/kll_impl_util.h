// kll_impl_util.h
#ifndef FCK_KLL_IMPL_UTIL_H_INCLUDED
#define FCK_KLL_IMPL_UTIL_H_INCLUDED

#define kll_make_allocator(ctx, realloc_func)                                                                                              \
	{                                                                                                                                      \
		.context = (kll_context *)(ctx), .vt = {(kll_realloc_function *)(realloc_func) }                                                   \
	}

#endif // !FCK_KLL_IMPL_UTIL_H_INCLUDED