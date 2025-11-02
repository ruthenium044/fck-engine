// kll_heap.h
#ifndef FCK_KLL_HEAP_H_INCLUDED
#define FCK_KLL_HEAP_H_INCLUDED

#include <fckc_apidef.h>

#if defined(FCK_KLL_EXPORT)
#define FCK_KLL_API FCK_EXPORT_API
#else
#define FCK_KLL_API FCK_IMPORT_API
#endif

typedef struct kll_allocator kll_allocator;
FCK_KLL_API extern kll_allocator *kll_heap;

#endif // !FCK_KLL_HEAP_H_INCLUDED