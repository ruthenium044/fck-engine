#ifndef FCK_OS_H
#define FCK_OS_H

#include <fckc_apidef.h>
#include <fckc_inttypes.h>

// TODO: Still no clue how this shit behaves with multiple windows
// well, well, well

#if defined(FCK_STD_EXPORT)
#define FCK_STD_API FCK_EXPORT_API
#else
#define FCK_STD_API FCK_IMPORT_API
#endif

struct kll_allocator;
union fck_event;

// Should be fck_format_api...
typedef struct fck_io_api
{
	int (*format)(char *s, size_t n, const char *format, ...);
	void (*log)(const char *format, ...);
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
	int (*is_valid)(fck_window window);
	int (*size)(fck_window window, int *width, int *height);
	int (*position)(fck_window window, int *x, int *z);
	int (*resize)(fck_window window, int width, int height);
	void (*destroy)(fck_window window);

	// Returns platform native data, such as:
	// HWDN and HINSTANCE on windows
	// or NSWindow on MacOS
	void* (*native)(fck_window window, const char* name);

	int (*text_input_start)(fck_window window);
	int (*text_input_stop)(fck_window window);
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
	fckc_i64 (*flush)(fck_file);
} fck_filesystem_api;

typedef struct fck_event_channel
{
	void *handle;
} fck_event_channel;

typedef struct fck_event_channel_api
{
	// TODO: Make it possible so the user can not see the producing-side
	fck_event_channel (*create)(struct kll_allocator *alloocator, fckc_size_t capacity);
	void (*destroy)(fck_event_channel channel);

	// Pumps events into channel
	void (*pump)(fck_event_channel channel);

	// Returns the actual count of events placed in provided buffer
	fckc_size_t (*poll)(fck_event_channel channel, union fck_event *events, fckc_size_t capacity, fckc_size_t *count);
} fck_event_channel_api;

typedef struct fck_os_api
{
	fck_io_api *io;
	fck_shared_object_api *so;
	fck_window_api *win;
	fck_filesystem_api *fs;

	// Special APIs are not getting a cool little abbrevation
	fck_clipboard_api *clipboard;
	fck_chrono_api *chrono;
	fck_event_channel_api *event_channel;
} fck_os_api;

FCK_STD_API extern fck_os_api *os;

// extern fck_os_api *os;

#endif // !FCK_OS_H