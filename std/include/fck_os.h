#ifndef FCK_OS_H
#define FCK_OS_H

#include <fckc_inttypes.h>

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

typedef struct fck_string_conversion_api
{
	// Maybe the names and the architecture of this will feel a bit... lazy
	long long (*ll)(const char *restrict str, char **restrict end, int base);
	unsigned long long (*ull)(const char *restrict str, char **restrict end, int base);
	double (*d)(const char *restrict str, char **restrict end);
} fck_string_conversion_api;

typedef struct fck_string_api
{
	fck_unsafe_string_api *unsafe;
	fck_string_conversion_api *to;

	int (*cmp)(const char *lhs, const char *rhs, fckc_size_t maxlen);
	char *(*dup)(const char *str, fckc_size_t maxlen);
	fckc_size_t (*len)(const char *str, fckc_size_t maxlen);
} fck_string_api;

typedef struct fck_memory_api
{
	// malloc, realloc and free come from KLL!!!
	void *(*cpy)(void *dst, void const *src, fckc_size_t size);
	void *(*set)(void *bytes, int value, fckc_size_t size);
} fck_memory_api;

typedef struct fck_io_api
{
	int (*snprintf)(char *restrict s, size_t n, const char *restrict format, ...);
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
extern fck_std_api *std;

// extern fck_os_api *os;

#endif // !FCK_OS_H