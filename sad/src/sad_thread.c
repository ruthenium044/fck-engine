#ifdef _WIN32
#include "sad_thread_win.inl"
#else
// YOLO LOL
#include "sad_thread_unix.inl"
#endif

const char *sad_result_to_string(sad_result result)
{
	switch (result)
	{
	case SAD_RESULT_SUCCESS:
		return "SAD RESULT SUCCESS";
	case SAD_RESULT_GENERIC_FAILURE:
		return "SAD RESULT GENERIC FAILURE";
	}
	return "SAD RESULT UNKNOWN";
}