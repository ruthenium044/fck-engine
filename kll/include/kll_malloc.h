// kll_malloc.h
#ifndef FCK_KLL_MALLOC_H_INCLUDED
#define FCK_KLL_MALLOC_H_INCLUDED

#define kll_malloc(allocator, size) allocator->vt.realloc((allocator)->context, NULL, (size), (__LINE__), (__FILE__))
#define kll_realloc(allocator, ptr, size) allocator->vt.realloc((allocator)->context, (ptr), (size), (__LINE__), (__FILE__))
#define kll_free(allocator, ptr) allocator->vt.realloc((allocator)->context, (ptr), 0, (__LINE__), (__FILE__))

#endif // !FCK_KLL_MALLOC_H_INCLUDED