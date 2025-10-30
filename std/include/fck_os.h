#ifndef FCK_OS_H
#define FCK_OS_H

#include <fckc_apidef.h>
#include <fckc_inttypes.h>

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

// Should be fck_format_api...
typedef struct fck_io_api
{
	int (*format)(char *s, size_t n, const char *format, ...);
} fck_io_api;

typedef struct fck_shared_object
{
	void *handle;
} fck_shared_object;

typedef struct fck_shared_object_api
{
	fck_shared_object (*load)(const char *path);
	void (*unload)(fck_shared_object so);
	int (*is_valid)(fck_shared_object so);
	void *(*symbol)(fck_shared_object so, const char *name);
} fck_shared_object_api;

typedef struct fck_window
{
	void *handle;
} fck_window;

typedef struct fck_window_api
{
	fck_window (*create)(const char *name, int w, int h);
	int (*is_valid)(fck_window);
	int (*text_input_start)(fck_window);
	int (*text_input_stop)(fck_window);
	int (*size_get)(fck_window, int *width, int *height);
	void (*destroy)(fck_window handle);
} fck_window_api;

typedef struct fck_clipboard
{
	char *text;
} fck_clipboard;

typedef struct fck_clipboard_api
{
	int (*set)(const char *text);
	int (*has)(void);

	fck_clipboard (*receive)(void);
	int (*is_valid)(fck_clipboard);
	void (*close)(fck_clipboard);
} fck_clipboard_api;

typedef struct fck_chrono_api
{
	fckc_u64 (*ms)(void);
} fck_chrono_api;

typedef struct fck_file
{
	void *handle;
} fck_file;

typedef enum fck_stream_seek_mode
{
	FCK_STREAM_SET,
	FCK_STREAM_CUR,
	FCK_STREAM_END,
} fck_stream_seek_mode;

typedef struct fck_filesystem_api
{
	fck_file (*open)(const char *path, const char *mode);
	void (*close)(fck_file);

	int (*is_valid)(fck_file);

	fckc_i64 (*size)(fck_file);
	fckc_i64 (*seek)(fck_file, fckc_i64 offset, fck_alias(fck_stream_seek_mode, fckc_u32) seek_mode);
	fckc_size_t (*read)(fck_file, void *ptr, fckc_size_t size);
	fckc_size_t (*write)(fck_file, const void *ptr, fckc_size_t size);
	fckc_u32 (*flush)(fck_file);
} fck_filesystem_api;

typedef struct fck_os_api
{
	fck_char_api *chr;
	fck_string_api *str;
	fck_memory_api *mem;
	fck_io_api *io;
	fck_shared_object_api *so;
	fck_window_api *win;
	fck_clipboard_api *clipboard;
	fck_chrono_api *chrono;
	fck_filesystem_api *fs;
} fck_os_api;

FCK_STD_API extern fck_os_api *os;

// extern fck_os_api *os;

#endif // !FCK_OS_H