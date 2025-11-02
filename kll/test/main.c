#include <kll.h>

#include <kll_heap.h>
#include <kll_malloc.h>
#include <kll_temp.h>

int main(int argc, char **argv)
{
	kll_temp_allocator *allocator = kll_temp_allocator_create(kll_heap, 256);

	for (int i = 0; i < 256; i++)
	{
		fckc_u64 *num = (fckc_u64 *)kll_malloc(allocator, sizeof(fckc_u64));
	}
	kll_temp_reset(allocator);

	for (int i = 0; i < 256; i++)
	{
		fckc_u64 *num = (fckc_u64 *)kll_malloc(allocator, sizeof(fckc_u64));
	}
	kll_temp_reset(allocator);

	const char *hello = kll_temp_format(allocator, "%s %s - %llu", "Hello", "World", 69LLU);

	return 0;
}