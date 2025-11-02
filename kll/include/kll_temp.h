#ifndef FCK_KLL_TEMP_H_INCLUDED
#define FCK_KLL_TEMP_H_INCLUDED

#include <fckc_apidef.h>
#include <fckc_inttypes.h>

struct kll_temp_allocator;
struct kll_allocator;

FCK_IMPORT_API struct kll_temp_allocator *kll_temp_allocator_create(struct kll_allocator *parent, fckc_size_t capacity);
FCK_IMPORT_API void kll_temp_allocator_destroy(struct kll_temp_allocator *allocator);

// Functionality to bind to static memory would be cool
#define kll_temp_new(allocator, cap) kll_temp_allocator_create((allocator), (cap))
#define kll_temp_delete(allocator) kll_temp_allocator_destroy((allocator))
#define kll_temp_reset(allocator) (allocator)->temp.reset((allocator)->context)
#define kll_temp_format(allocator, fmt, ...) (allocator)->temp.format((allocator)->context, (fmt), ##__VA_ARGS__)

#endif // !FCK_KLL_TEMP_H_INCLUDED