// kll_scratch.h
#ifndef FCK_KLL_SCRATCH_H_INCLUDED
#define FCK_KLL_SCRATCH_H_INCLUDED

#include <inttypes.h>

typedef struct kll_allocator kll_allocator;

kll_allocator *kll_scratch_create(void *memory, size_t size, size_t alignment);
kll_allocator *kll_scratch_reset(kll_allocator *allocator);

#endif // !FCK_KLL_SCRATCH_H_INCLUDED