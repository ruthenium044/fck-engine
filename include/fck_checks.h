#ifndef FCK_MACRO_CHECKS_INCLUDED
#define FCK_MACRO_CHECKS_INCLUDED

#include <SDL3/SDL_log.h>

// These are probably shit to debug lmao
#define CHECK_INFO(condition, message, ret)                                                                            \
	if ((condition))                                                                                                   \
	{                                                                                                                  \
		SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "%s:%d - %s", __FILE__, __LINE__, message);                          \
		ret;                                                                                                           \
	}
#define CHECK_WARNING(condition, message, ret)                                                                         \
	if ((condition))                                                                                                   \
	{                                                                                                                  \
		SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "%s:%d - %s", __FILE__, __LINE__, message);                          \
		ret;                                                                                                           \
	}
#define CHECK_ERROR(condition, message, ret)                                                                           \
	if ((condition))                                                                                                   \
	{                                                                                                                  \
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s:%d - %s", __FILE__, __LINE__, message);                         \
		ret;                                                                                                           \
	}
#define CHECK_CRITICAL(condition, message, ret)                                                                        \
	if ((condition))                                                                                                   \
	{                                                                                                                  \
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "%s:%d - %s", __FILE__, __LINE__, message);                      \
		ret;                                                                                                           \
	}

#endif // !FCK_MACRO_CHECKS_INCLUDED