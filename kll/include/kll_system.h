// kll_system.h
#ifndef FCK_KLL_SYSTEM_H_INCLUDED
#define FCK_KLL_SYSTEM_H_INCLUDED

#include <fckc_apidef.h>

#if defined(FCK_KLL_EXPORT)
#define FCK_KLL_API FCK_EXPORT_API
#else
#define FCK_KLL_API FCK_IMPORT_API
#endif

struct kll_allocator;
FCK_KLL_API extern struct kll_allocator *kll_system;

#endif // !FCK_KLL_SYSTEM_H_INCLUDED