// kll_malloc.h
#ifndef FCK_KLL_MALLOC_H_INCLUDED
#define FCK_KLL_MALLOC_H_INCLUDED

// If issues, better casting!
#define kll_malloc(allocator, size) (allocator)->realloc((struct kll_allocator*)(allocator), NULL, (size), (__FILE__), (__LINE__))
#define kll_realloc(allocator, ptr, size) (allocator)->realloc((struct kll_allocator*)(allocator), (ptr), (size), (__FILE__), (__LINE__))
#define kll_free(allocator, ptr) (allocator)->realloc((struct kll_allocator*)(allocator), (ptr), 0, (__FILE__), (__LINE__))

#endif // !FCK_KLL_MALLOC_H_INCLUDED