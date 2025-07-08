#include <sad_thread.h>
#include <sad_atomic.h>
#include <sad_pointer_queue.h>

static int thread_main(void *args)
{
	sad_thread_sleep(10000);

	return 0;
}

// #define SDL_CompilerBarrier()   __asm__ __volatile__ ("" : : : "memory")
int main(int argc, char **argv)
{
	sad_thread_arguments memory = sad_thread_arguments_create(thread_main, NULL);
	sad_thread thread = sad_thread_open(&memory);

	sad_release();

	sad_thread_sleep(2000);
	sad_thread_join(thread, SAD_INFINITE);


	return 0;
}