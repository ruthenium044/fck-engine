#include "sad_thread.h"

#include <Windows.h>
#include <fckc_math.h>

typedef struct sad_affinity_win
{
	KAFFINITY mask;
	WORD group;
	WORD padding[3];
} sad_affinity_win;

static void to_user_affinity(sad_affinity_win const *win, sad_affinity *affinity)
{
	memcpy(affinity, win, fck_min(sizeof(*affinity), sizeof(*win)));
}

static void to_win_affinity(sad_affinity const *affinity, sad_affinity_win *win)
{
	memcpy(win, affinity, fck_min(sizeof(*affinity), sizeof(*win)));
}

static sad_thread to_user_handle(HANDLE handle)
{
	sad_thread thread = 0;
	memcpy(&thread, &handle, fck_min(sizeof(thread), sizeof(handle)));
	return thread;
}

static HANDLE to_win_handle(sad_thread thread)
{
	HANDLE handle = NULL;
	memcpy(&handle, &thread, fck_min(sizeof(thread), sizeof(handle)));
	return handle;
}

static DWORD WINAPI real_main(LPVOID args)
{
	sad_thread_arguments *memory = (sad_thread_arguments *)args;
	return (DWORD)memory->main(memory->args);
};

sad_thread sad_thread_current(void)
{
	HANDLE handle = GetCurrentThread();
	return to_user_handle(handle);
}

sad_thread_arguments sad_thread_arguments_create(sad_thread_main *main, void *args)
{
	return (sad_thread_arguments){main, args};
}

sad_thread sad_thread_open(sad_thread_arguments *memory)
{
	DWORD thread_id; // Fuck this, use get_thread_id.. We can store it back in the thread_arguments...
	HANDLE handle = CreateThread(NULL, 0, real_main, (LPVOID)memory, 0, &thread_id);
	return to_user_handle(handle);
}

sad_result sad_thread_close(sad_thread thread)
{
	HANDLE handle = to_win_handle(thread);

	if (CloseHandle(handle))
	{
		return SAD_RESULT_SUCCESS;
	}
	return SAD_RESULT_GENERIC_FAILURE;
}

sad_result sad_thread_detach(sad_thread thread)
{
	return sad_thread_close(thread);
}

int sad_thread_equal(sad_thread lhs, sad_thread rhs)
{
	return sad_thread_id(lhs) == sad_thread_id(rhs);
}

sad_result sad_thread_join(sad_thread thread, fckc_u64 ms)
{
	HANDLE handle = to_win_handle(thread);
	DWORD result = WaitForSingleObject(handle, ms);
	// In any case, clean up the handle
	DWORD close_result = CloseHandle(handle);
	if (result != 0 || close_result == FALSE)
	{
		return SAD_RESULT_GENERIC_FAILURE;
	}
	return SAD_RESULT_SUCCESS;
}

sad_result sad_thread_sleep(fckc_u64 ms)
{
	Sleep((DWORD)ms);
	return SAD_RESULT_SUCCESS;
}

sad_result sad_thread_yield(void)
{
	return SwitchToThread() == FALSE ? SAD_RESULT_GENERIC_FAILURE : SAD_RESULT_SUCCESS;
}

fckc_u64 sad_thread_id(sad_thread thread)
{
	HANDLE handle = to_win_handle(thread);
	DWORD id = GetThreadId(handle);
	return (fckc_u64)id;
}

fckc_u64 sad_thread_current_id(void)
{
	return sad_thread_id(sad_thread_current());
}

// TODO: Stuff below
// fckc_u16 sad_thread_get_active_processor_count()
//{
//	return (fckc_u16)GetActiveProcessorGroupCount();
//}
//
// fckc_u16 sad_thread_get_processor_count(fckc_u16 group)
//{
//	return (fckc_size_t)GetActiveProcessorCount(group);
//}
//
// fckc_u64 sad_thread_create_full_mask(fckc_u16 group)
//{
//	fckc_u64 count = (fckc_u64)GetActiveProcessorCount(group);
//	fckc_u64 full_mask = ~((fckc_u64)0);
//	fckc_u64 difference = 64 - count;
//	fckc_u64 mask = full_mask >> difference;
//	return mask;
//}

void sad_thread_set_affinity(sad_thread thread, sad_affinity const *affinity)
{
	sad_affinity_win win_affinity;
	to_win_affinity(affinity, &win_affinity);

	GROUP_AFFINITY group_affinity;
	group_affinity.Group = win_affinity.group;
	group_affinity.Mask = win_affinity.mask;

	HANDLE handle = to_win_handle(thread);
	SetThreadGroupAffinity(handle, &group_affinity, NULL);
}
