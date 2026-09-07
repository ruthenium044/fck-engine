#ifndef FCK_OS_H_INCLUDED
#define FCK_OS_H_INCLUDED

#include <fckc_apidef.h>
#include <fckc_inttypes.h>

// TODO: Still no clue how this shit behaves with multiple windows
// well, well, well

#if defined(FCK_OS_EXPORT)
#define FCK_OS_API FCK_EXPORT_API
#else
#define FCK_OS_API FCK_IMPORT_API
#endif

// TODO: This is meh, format in temp allocator?
typedef struct fck_io_api
{
	void (*log)(const char *format, ...);
} fck_io_api;

// This is ok
typedef struct fck_shared_object
{
	void *handle;
} fck_shared_object;

typedef struct fck_shared_object_api
{
	fck_shared_object (*load)(const char *path);
	void (*unload)(fck_shared_object so);
	int (*is_ok)(fck_shared_object so);
	void *(*symbol)(fck_shared_object so, const char *name);
} fck_shared_object_api;

// This is ok
typedef struct fck_window
{
	void *handle;
} fck_window;

typedef struct fck_window_configuration
{
	float title_bar_height;
	float resize_line_width;
	float menu_area_width;
	float button_area_width;
} fck_window_configuration;

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
	void *(*native)(fck_window window, const char *name);

	// Hmm, I do not hate the style of this
	// win->configure(window, NULL) -> Does not set
	// win->configure(window, &config) -> Sets
	const fck_window_configuration *(*configuration)(fck_window window, const fck_window_configuration *config);
	const char *(*title)(fck_window window, const char *title);

	int (*minimise)(fck_window window);

	// Wonky, but ok
	int (*text_input_start)(fck_window window);
	int (*text_input_stop)(fck_window window);
	int (*text_input_active)(fck_window window);
} fck_window_api;

// This is ok
typedef struct fck_clipboard
{
	char *text;
} fck_clipboard;

// TODO: This is a bit weird, I want to avoid clipboard.text
typedef struct fck_clipboard_api
{
	int (*set)(const char *text);
	int (*has)(void);

	fck_clipboard (*receive)(void);
	int (*is_valid)(fck_clipboard);
	void (*close)(fck_clipboard);
} fck_clipboard_api;

// Meh
typedef struct fck_chrono_api
{
	fckc_u64 (*ms)(void);
	fckc_u64 (*ns)(void);
	// Maybe calling time (out of application) today
	// And now is the local application-related now...
	fckc_i64 (*now)(void);
	void (*sleep)(fckc_u64 ms);
} fck_chrono_api;

typedef struct fck_file
{
	void *handle;
} fck_file;

typedef enum fck_stream_seek_mode
{
	fck_stream_set,
	fck_stream_cur,
	fck_stream_end,
} fck_stream_seek_mode;

typedef enum fck_path_type
{
	fck_path_none,
	fck_path_file,
	fck_path_directory,
} fck_path_type;

typedef struct fck_path_info
{
	fck_alias(fck_path_type, fckc_u64) type;
	fckc_u64 size;
	fckc_i64 created;
	fckc_i64 modified;
	fckc_i64 accessed;
} fck_path_info;

// This is ok
typedef struct fck_filesystem_api
{
	// Classic file stuff
	fck_file (*open)(const char *path, const char *mode);
	void (*close)(fck_file);
	int (*is_valid)(fck_file);
	fckc_i64 (*size)(fck_file);
	fckc_i64 (*seek)(fck_file, fckc_i64 offset, fck_alias(fck_stream_seek_mode, fckc_u32) seek_mode);
	fckc_size_t (*read)(fck_file, void *ptr, fckc_size_t size);
	fckc_size_t (*write)(fck_file, const void *ptr, fckc_size_t size);
	fckc_i64 (*flush)(fck_file);

	int (*create_directory)(const char *path);

	// Path utilities - Maybe path api?
	int (*info)(const char *path, fck_path_info *info);

	int (*remove)(const char *path);
	// TODO: Remove this, use info instead
	fckc_i64 (*modified)(const char *path);
	const char *(*executable)(void);

} fck_file_system_api;

typedef struct fck_glob_api
{
	char *(*find)(const char *str, const char *substring);
	char *(*match)(const char *str, const char *pattern);
	fckc_size_t (*executable)(const char *pattern, char ***out_paths);
	fckc_size_t (*directory)(const char *path, const char *pattern, char ***out_paths);
	void (*free)(char **paths);
} fck_glob_api;

typedef enum fck_file_watcher_event_type
{
	fck_file_unknown,
	fck_file_created,
	fck_file_deleted,
	fck_file_modified,
} fck_file_watcher_event_type;

typedef struct fck_file_watcher_event
{
	fck_alias(fck_file_watcher_event_type, fckc_u32) type;
	char path[420]; // blaze it
	fckc_i64 time;
} fck_file_watcher_event;

typedef struct fck_file_watcher
{
	void *handle;
} fck_file_watcher;

typedef struct fck_file_watcher_api
{
	fck_file_watcher (*create)(const char *path);
	fckc_size_t (*changes)(fck_file_watcher watcher, fck_file_watcher_event *events, fckc_size_t capacity);
	// is_valid, exposed to allow users to answer the question:
	// "Ummm... why am I not getting any changes?"
	int (*is_valid)(fck_file_watcher watcher);
	void (*destroy)(fck_file_watcher watcher);
} fck_file_watcher_api;

// This is ok
typedef struct fck_os_api
{
	fck_io_api *io;
	fck_shared_object_api *so;
	fck_window_api *win;
	fck_file_system_api *fs;
	fck_glob_api *glob;
	// Special APIs are not getting a cool little abbrevation
	fck_clipboard_api *clipboard;
	fck_chrono_api *chrono;
	fck_file_watcher_api *fw;
} fck_os_api;

// Also ok. Maybe have inline loaders for each platform!
FCK_OS_API extern fck_os_api *os;

// extern fck_os_api *os;

#endif // !FCK_OS_H_INCLUDED