// sad_threads.h

#ifndef SAD_THREADS_H_INCLUDED
#define SAD_THREADS_H_INCLUDED

#include <fckc_inttypes.h>

#define SAD_THREAD_MEMORY 64
#define SAD_INFINITE 0xFFFFFFFF

typedef enum sad_result
{
	SAD_RESULT_SUCCESS = 0,
	SAD_RESULT_GENERIC_FAILURE = 1, // Every generic failure should become an explicit result at some point.
} sad_result;

typedef struct sad_affinity
{
	fckc_u64 data[2];
} sad_affinity;

typedef fckc_size_t sad_thread;

typedef int(sad_thread_main)(void *);

typedef struct sad_thread_arguments
{
	sad_thread_main *main;
	void *args;
} sad_thread_arguments;

sad_thread_arguments sad_thread_arguments_create(sad_thread_main *main, void *args);
sad_thread sad_thread_open(sad_thread_arguments *memory);
sad_result sad_thread_close(sad_thread thread);

sad_thread sad_thread_current(void);
sad_result sad_thread_detach(sad_thread thread);
sad_result sad_thread_join(sad_thread thread, fckc_u64 ms);
sad_result sad_thread_sleep(fckc_u64 ms);
sad_result sad_thread_yield(void);

int sad_thread_equal(sad_thread lhs, sad_thread rhs);
fckc_u64 sad_thread_id(sad_thread thread);
fckc_u64 sad_thread_current_id(void);

#endif // !SAD_THREADS_H_INCLUDED