#include "sad_atomic.h"

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(__clang__)
#include <intrin.h>
#define SAD_MSVC_AVAILABLE 1
#elif defined(__clang__) || defined(__GNUC__)
#define SAD_GNUC_AVAILABLE 1
#endif

fckc_u32 sad_u32_cas(sad_atomic_u32 volatile *atomic, fckc_u32 old_value, fckc_u32 new_value)
{
#if defined(SAD_MSVC_AVAILABLE)
	return (fckc_u32)_InterlockedCompareExchange(&atomic->value, new_value, old_value);
#elif defined(SAD_GNUC_AVAILABLE)
	return (fckc_u32)__sync_val_compare_and_swap(&atomic->value, old_value, new_value);
#endif
}
fckc_u32 sad_u32_load(sad_atomic_u32 volatile *atomic)
{
#if defined(SAD_MSVC_AVAILABLE)
	return (fckc_u32)_InterlockedOr(&atomic->value, (long)0);
#elif defined(SAD_GNUC_AVAILABLE)
	return (fckc_u32)__sync_fetch_and_or(&atomic->value, 0);
#endif
}
fckc_u32 sad_u32_store(sad_atomic_u32 volatile *atomic, fckc_u32 value)
{
#if defined(SAD_MSVC_AVAILABLE)
	return (fckc_u32)_InterlockedExchange(&atomic->value, (long)value);
#elif defined(SAD_GNUC_AVAILABLE)
	return (fckc_u32)__sync_lock_test_and_set(&atomic->value, value);
#endif
}
fckc_u32 sad_u32_load_and_add(sad_atomic_u32 volatile *atomic, fckc_u32 value)
{
#if defined(SAD_MSVC_AVAILABLE)
	return (fckc_u32)_InterlockedExchangeAdd(&atomic->value, (long)value);
#elif defined(SAD_GNUC_AVAILABLE)
	return (fckc_u32)__sync_fetch_and_add(&atomic->value, value);
#endif
}
fckc_u32 sad_u32_add_and_load(sad_atomic_u32 volatile *atomic, fckc_u32 value)
{
#if defined(SAD_MSVC_AVAILABLE)
	return (fckc_u32)_InterlockedExchangeAdd(&atomic->value, (long)value) + value;
#elif defined(SAD_GNUC_AVAILABLE)
	return (fckc_u32)__sync_add_and_fetch(&atomic->value, value);
#endif
}
fckc_u32 sad_u32_load_and_sub(sad_atomic_u32 volatile *atomic, fckc_u32 value)
{
#if defined(SAD_MSVC_AVAILABLE)
	return (fckc_u32)_InterlockedExchangeAdd(&atomic->value, -(long)value);
#elif defined(SAD_GNUC_AVAILABLE)
	return (fckc_u32)__sync_fetch_and_sub(&atomic->value, value);
#endif
}
fckc_u32 sad_u32_sub_and_load(sad_atomic_u32 volatile *atomic, fckc_u32 value)
{
#if defined(SAD_MSVC_AVAILABLE)
	return (fckc_u32)_InterlockedExchangeAdd(&atomic->value, -(long)value) - value;
#elif defined(SAD_GNUC_AVAILABLE)
	return (fckc_u32)__sync_sub_and_fetch(&atomic->value, value);
#endif
}

void *sad_pointer_cas(void *volatile *ptr, void *old_value, void *new_value)
{
#if defined(SAD_MSVC_AVAILABLE)
	return (void *)_InterlockedCompareExchangePointer(ptr, new_value, old_value);
#elif defined(SAD_GNUC_AVAILABLE)
	return (void *)__sync_val_compare_and_swap(ptr, old_value, new_value);
#endif
}

void *sad_pointer_load(void *volatile *ptr)
{
#if defined(SAD_MSVC_AVAILABLE)
	return (void *)_InterlockedCompareExchangePointer(ptr, NULL, NULL);
#elif defined(SAD_GNUC_AVAILABLE)
	return (void *)__sync_val_compare_and_swap(ptr, NULL, NULL);
#endif
}
void *sad_pointer_store(void *volatile *ptr, void *value)
{
#if defined(SAD_MSVC_AVAILABLE)
	return (void *)_InterlockedExchangePointer(ptr, value);
#elif defined(SAD_GNUC_AVAILABLE)
	return (void *)__sync_lock_test_and_set(ptr, value);
#endif
}
