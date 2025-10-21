#ifndef FCK_OS_H
#define FCK_OS_H

#include <fckc_inttypes.h>
#include <fckc_apidef.h>

#if defined(FCK_STD_EXPORT)
#define FCK_STD_API FCK_EXPORT_API
#else
#define FCK_STD_API FCK_IMPORT_API
#endif

typedef struct fck_char_api
{
	int (*isspace)(int ch);
	int (*isdigit)(int ch);
} fck_char_api;

typedef struct fck_unsafe_string_api
{
	int (*cmp)(const char *lhs, const char *rhs);
	char *(*dup)(const char *str);
	fckc_size_t (*len)(const char *str);
} fck_unsafe_string_api;

typedef struct fck_string_api
{
	fck_unsafe_string_api *unsafe;
	int (*cmp)(const char *lhs, const char *rhs, fckc_size_t maxlen);
	char *(*dup)(const char *str, fckc_size_t maxlen);
	fckc_size_t (*len)(const char *str, fckc_size_t maxlen);
	long long (*toll)(const char *str, char **end, int base);
	unsigned long long (*toull)(const char *str, char **end, int base);
	double (*tod)(const char *str, char **end);
} fck_string_api;

typedef struct fck_memory_api
{
	// malloc, realloc and free come from KLL!!!
	void *(*cpy)(void *dst, void const *src, fckc_size_t size);
	void *(*set)(void *bytes, int value, fckc_size_t size);
} fck_memory_api;

typedef struct fck_io_api
{
	int (*snprintf)(char *s, size_t n, const char *format, ...);
} fck_io_api;

typedef struct fck_std_api
{
	fck_char_api *character;
	fck_string_api *str;
	fck_memory_api *mem;
	fck_io_api *io;
} fck_std_api;

// typedef struct fck_os_api
//{
// } fck_os_api;

// Gotta see if this name clashes hehe
// Maybe just putting everything into OS and calling it os is nicer?
FCK_STD_API extern fck_std_api *std;

// extern fck_os_api *os;

#endif // !FCK_OS_H