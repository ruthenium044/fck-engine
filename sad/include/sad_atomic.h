// sad_atomic.h

#ifndef SAD_ATOMIC_H_INCLUDED
#define SAD_ATOMIC_H_INCLUDED

#include <fckc_inttypes.h>

typedef struct sad_atomic_u32
{
	fckc_u32 value;
} sad_atomic_u32;

// Maybe call sad_acquire and sad_release something else
// sad_load_fence, sad_store_fence? idk
#if defined(_MSC_VER) && (_MSC_VER > 1200) && !defined(__clang__)
void _ReadBarrier(void);
void _WriteBarrier(void);
void _ReadWriteBarrier(void);
#pragma intrinsic(_ReadBarrier)
#pragma intrinsic(_WriteBarrier)
#pragma intrinsic(_ReadWriteBarrier)
#define sad_release() _WriteBarrier()
#define sad_acquire() _ReadBarrier()
#define sad_fence() _ReadWriteBarrier()
#elif defined(__GNUC__) && (defined(__powerpc__) || defined(__ppc__))
#define sad_release() __asm__ __volatile__("lwsync" : : : "memory")
#define sad_acquire() __asm__ __volatile__("isync" : : : "memory")
#define sad_fence()  __asm__ __volatile__("sync" : : : "memory")
#elif defined(__GNUC__) && (defined(__aarch64__) || defined(__arm__))
#define sad_release() __asm__ __volatile__("dmb ishst" : : : "memory")
#define sad_acquire() __asm__ __volatile__("dmb ishld" : : : "memory")
#define sad_fence() __asm__ __volatile__("dmb ish" : : : "memory")
#else
#define sad_release() __asm__ __volatile__("" : : : "memory")
#define sad_acquire() __asm__ __volatile__("" : : : "memory")
#define sad_fence() __asm__ __volatile__("" : : : "memory")
#endif

// prefer:
// sad_loadp, sad_load32, sad_load16... 

fckc_u32 sad_u32_cas(sad_atomic_u32 volatile* atomic, fckc_u32 old_value, fckc_u32 new_value);
fckc_u32 sad_u32_load(sad_atomic_u32 volatile* atomic);
fckc_u32 sad_u32_store(sad_atomic_u32 volatile* atomic, fckc_u32 value);

fckc_u32 sad_u32_load_and_add(sad_atomic_u32 volatile* atomic, fckc_u32 value);
fckc_u32 sad_u32_add_and_load(sad_atomic_u32 volatile* atomic, fckc_u32 value);
fckc_u32 sad_u32_load_and_sub(sad_atomic_u32 volatile* atomic, fckc_u32 value);
fckc_u32 sad_u32_sub_and_load(sad_atomic_u32 volatile* atomic, fckc_u32 value);


void* sad_pointer_cas(void* volatile* ptr, void* old_value, void* new_value);
void* sad_pointer_load(void* volatile* ptr);
void* sad_pointer_store(void* volatile* ptr, void* value);
#endif // !SAD_ATOMIC_H_INCLUDED
