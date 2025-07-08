#include "sad_thread.h"

//static sad_thread to_user_handle(HANDLE handle)
//{
//	return thread;
//}
//
//static HANDLE to_win_handle(sad_thread thread)
//{
//	return handle;
//}

//static DWORD WINAPI real_main(LPVOID args)
//{
//	sad_thread_memory* memory = (sad_thread_memory*)args;
//	return (DWORD)memory->main(memory->args);
//};

sad_thread sad_thread_current(void)
{
	return 0xDEADDEAD;
}

sad_thread sad_thread_open(sad_thread_arguments* memory, sad_thread_main main, void* args)
{
	return 0xDEADDEAD;
}

int sad_thread_close(sad_thread thread)
{
	return 0xDEADDEAD;
}

int sad_thread_detach(sad_thread thread)
{
	return 0xDEADDEAD;
}

int sad_thread_equal(sad_thread lhs, sad_thread rhs)
{
	return 0xDEADDEAD;
}

int sad_thread_join(sad_thread thread, fckc_u64 ms)
{
	return 0xDEADDEAD;
}

int sad_thread_sleep(fckc_u64 ms)
{
	return 0xDEADDEAD;
}

int sad_thread_yield(void)
{
	return 0xDEADDEAD;
}

fckc_u64 sad_thread_id(sad_thread thread)
{
	return 0xDEADDEAD;
}

fckc_u64 sad_thread_current_id(void)
{
	return 0xDEADDEAD;
}

int sad_thread_force_on_core(sad_thread thread, int core_index)
{
	return 0xDEADDEAD;
}