#ifndef FCK_MACRO_CHECKS_INCLUDED
#define FCK_MACRO_CHECKS_INCLUDED

#include <SDL3/SDL_log.h>

// These are probably shit to debug lmao
#define CHECK_INFO_ARGS_1(condition, message, ret)                                                                     \
	if (!(condition))                                                                                                  \
	{                                                                                                                  \
		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "%s:%d - %s", __FILE__, __LINE__, message);                          \
		ret;                                                                                                           \
	}
#define CHECK_WARNING_ARGS_1(condition, message, ret)                                                                  \
	if (!(condition))                                                                                                  \
	{                                                                                                                  \
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "%s:%d - %s", __FILE__, __LINE__, message);                          \
		ret;                                                                                                           \
	}
#define CHECK_ERROR_ARGS_1(condition, message, ret)                                                                    \
	if (!(condition))                                                                                                  \
	{                                                                                                                  \
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s:%d - %s", __FILE__, __LINE__, message);                         \
		ret;                                                                                                           \
	}
#define CHECK_CRITICAL_ARGS_1(condition, message, ret)                                                                 \
	if (!(condition))                                                                                                  \
	{                                                                                                                  \
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "%s:%d - %s", __FILE__, __LINE__, message);                      \
		ret;                                                                                                           \
	}

#define CHECK_INFO_ARGS_0(condition, message)                                                                          \
	if (!(condition))                                                                                                  \
	{                                                                                                                  \
		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "%s:%d - %s", __FILE__, __LINE__, message);                          \
	}
#define CHECK_WARNING_ARGS_0(condition, message)                                                                       \
	if (!(condition))                                                                                                  \
	{                                                                                                                  \
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "%s:%d - %s", __FILE__, __LINE__, message);                          \
	}
#define CHECK_ERROR_ARGS_0(condition, message)                                                                         \
	if (!(condition))                                                                                                  \
	{                                                                                                                  \
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s:%d - %s", __FILE__, __LINE__, message);                         \
	}
#define CHECK_CRITICAL_ARGS_0(condition, message)                                                                      \
	if (!(condition))                                                                                                  \
	{                                                                                                                  \
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "%s:%d - %s", __FILE__, __LINE__, message);                      \
	}

#define CHECK_MACRO_CHOOSER(A, FUNC, ...) FUNC

#define CHECK_INFO(condition, message, ...)                                                                            \
	CHECK_MACRO_CHOOSER(__VA_ARGS__, CHECK_INFO_ARGS_1(condition, message, __VA_ARGS__),                               \
	                    CHECK_INFO_ARGS_0(condition, message))

#define CHECK_WARNING(condition, message, ...)                                                                         \
	CHECK_MACRO_CHOOSER(__VA_ARGS__, CHECK_WARNING_ARGS_1(condition, message, __VA_ARGS__),                            \
	                    CHECK_WARNING_ARGS_0(condition, message))

#define CHECK_ERROR(condition, message, ...)                                                                           \
	CHECK_MACRO_CHOOSER(__VA_ARGS__, CHECK_ERROR_ARGS_1(condition, message, __VA_ARGS__),                              \
	                    CHECK_ERROR_ARGS_0(condition, message))

#define CHECK_CRITICAL(condition, message, ...)                                                                        \
	CHECK_MACRO_CHOOSER(__VA_ARGS__, CHECK_CRITICAL_ARGS_1(condition, message, __VA_ARGS__),                           \
	                    CHECK_CRITICAL_ARGS_0(condition, message))

#endif // !FCK_MACRO_CHECKS_INCLUDED