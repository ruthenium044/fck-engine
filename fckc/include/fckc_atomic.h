// fckc_atomic.h

#ifndef FCKC_ATOMIC_H_INCLUDED
#define FCKC_ATOMIC_H_INCLUDED

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(__clang__)
#include <intrin.h>
#define SAD_MSVC_AVAILABLE 1
#elif defined(__clang__) || defined(__GNUC__)
#define SAD_GNUC_AVAILABLE 1
#endif

#include "fckc_inttypes.h"

typedef struct fckc_atomic_u32
{
	fckc_u32 value;
} fckc_atomic_u32;

// Maybe call fckc_acquire and fckc_release something else
// fckc_load_fence, fckc_store_fence? idk
#if defined(_MSC_VER) && (_MSC_VER > 1200) && !defined(__clang__)
void _ReadBarrier(void);
void _WriteBarrier(void);
void _ReadWriteBarrier(void);
#pragma intrinsic(_ReadBarrier)
#pragma intrinsic(_WriteBarrier)
#pragma intrinsic(_ReadWriteBarrier)
#define fckc_release() _WriteBarrier()
#define fckc_acquire() _ReadBarrier()
#define fckc_fence() _ReadWriteBarrier()
#elif defined(__GNUC__) && (defined(__powerpc__) || defined(__ppc__))
#define fckc_release() __asm__ __volatile__("lwsync" : : : "memory")
#define fckc_acquire() __asm__ __volatile__("isync" : : : "memory")
#define fckc_fence()  __asm__ __volatile__("sync" : : : "memory")
#elif defined(__GNUC__) && (defined(__aarch64__) || defined(__arm__))
#define fckc_release() __asm__ __volatile__("dmb ishst" : : : "memory")
#define fckc_acquire() __asm__ __volatile__("dmb ishld" : : : "memory")
#define fckc_fence() __asm__ __volatile__("dmb ish" : : : "memory")
#else
#define fckc_release() __asm__ __volatile__("" : : : "memory")
#define fckc_acquire() __asm__ __volatile__("" : : : "memory")
#define fckc_fence() __asm__ __volatile__("" : : : "memory")
#endif

// prefer:
// fckc_loadp, fckc_load32, fckc_load16... 

fckc_u32 fckc_u32_cas(fckc_atomic_u32 volatile* atomic, fckc_u32 old_value, fckc_u32 new_value);
fckc_u32 fckc_u32_load(fckc_atomic_u32 volatile* atomic);
fckc_u32 fckc_u32_store(fckc_atomic_u32 volatile* atomic, fckc_u32 value);

fckc_u32 fckc_u32_load_and_add(fckc_atomic_u32 volatile* atomic, fckc_u32 value);
fckc_u32 fckc_u32_add_and_load(fckc_atomic_u32 volatile* atomic, fckc_u32 value);
fckc_u32 fckc_u32_load_and_sub(fckc_atomic_u32 volatile* atomic, fckc_u32 value);
fckc_u32 fckc_u32_sub_and_load(fckc_atomic_u32 volatile* atomic, fckc_u32 value);


void* fckc_pointer_cas(void* volatile* ptr, void* old_value, void* new_value);
void* fckc_pointer_load(void* volatile* ptr);
void* fckc_pointer_store(void* volatile* ptr, void* value);


fckc_u32 fckc_u32_cas(fckc_atomic_u32 volatile* atomic, fckc_u32 old_value, fckc_u32 new_value)
{
#if defined(SAD_MSVC_AVAILABLE)
	return (fckc_u32)_InterlockedCompareExchange((long*)&atomic->value, new_value, old_value);
#elif defined(SAD_GNUC_AVAILABLE)
	return (fckc_u32)__sync_val_compare_and_swap(&atomic->value, old_value, new_value);
#endif
}
fckc_u32 fckc_u32_load(fckc_atomic_u32 volatile* atomic)
{
#if defined(SAD_MSVC_AVAILABLE)
	return (fckc_u32)_InterlockedOr((long*)&atomic->value, (long)0);
#elif defined(SAD_GNUC_AVAILABLE)
	return (fckc_u32)__sync_fetch_and_or(&atomic->value, 0);
#endif
}
fckc_u32 fckc_u32_store(fckc_atomic_u32 volatile* atomic, fckc_u32 value)
{
#if defined(SAD_MSVC_AVAILABLE)
	return (fckc_u32)_InterlockedExchange((long*)&atomic->value, (long)value);
#elif defined(SAD_GNUC_AVAILABLE)
	return (fckc_u32)__sync_lock_test_and_set(&atomic->value, value);
#endif
}
fckc_u32 fckc_u32_load_and_add(fckc_atomic_u32 volatile* atomic, fckc_u32 value)
{
#if defined(SAD_MSVC_AVAILABLE)
	return (fckc_u32)_InterlockedExchangeAdd((long*)&atomic->value, (long)value);
#elif defined(SAD_GNUC_AVAILABLE)
	return (fckc_u32)__sync_fetch_and_add(&atomic->value, value);
#endif
}
fckc_u32 fckc_u32_add_and_load(fckc_atomic_u32 volatile* atomic, fckc_u32 value)
{
#if defined(SAD_MSVC_AVAILABLE)
	return (fckc_u32)_InterlockedExchangeAdd((long*)&atomic->value, (long)value) + value;
#elif defined(SAD_GNUC_AVAILABLE)
	return (fckc_u32)__sync_add_and_fetch(&atomic->value, value);
#endif
}
fckc_u32 fckc_u32_load_and_sub(fckc_atomic_u32 volatile* atomic, fckc_u32 value)
{
#if defined(SAD_MSVC_AVAILABLE)
	return (fckc_u32)_InterlockedExchangeAdd((long*)&atomic->value, -(long)value);
#elif defined(SAD_GNUC_AVAILABLE)
	return (fckc_u32)__sync_fetch_and_sub(&atomic->value, value);
#endif
}
fckc_u32 fckc_u32_sub_and_load(fckc_atomic_u32 volatile* atomic, fckc_u32 value)
{
#if defined(SAD_MSVC_AVAILABLE)
	return (fckc_u32)_InterlockedExchangeAdd((long*)&atomic->value, -(long)value) - value;
#elif defined(SAD_GNUC_AVAILABLE)
	return (fckc_u32)__sync_sub_and_fetch(&atomic->value, value);
#endif
}

void* fckc_pointer_cas(void* volatile* ptr, void* old_value, void* new_value)
{
#if defined(SAD_MSVC_AVAILABLE)
	return (void*)_InterlockedCompareExchangePointer(ptr, new_value, old_value);
#elif defined(SAD_GNUC_AVAILABLE)
	return (void*)__sync_val_compare_and_swap(ptr, old_value, new_value);
#endif
}

void* fckc_pointer_load(void* volatile* ptr)
{
#if defined(SAD_MSVC_AVAILABLE)
	return (void*)_InterlockedCompareExchangePointer(ptr, NULL, NULL);
#elif defined(SAD_GNUC_AVAILABLE)
	return (void*)__sync_val_compare_and_swap(ptr, NULL, NULL);
#endif
}
void* fckc_pointer_store(void* volatile* ptr, void* value)
{
#if defined(SAD_MSVC_AVAILABLE)
	return (void*)_InterlockedExchangePointer(ptr, value);
#elif defined(SAD_GNUC_AVAILABLE)
	return (void*)__sync_lock_test_and_set(ptr, value);
#endif
}

#endif // !FCKC_ATOMIC_H_INCLUDED
