#define FCK_OS_EXPORT
#include "fck_os.h"

#include <fckc_inttypes.h>

#include <SDL3/SDL_clipboard.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_loadso.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>

static int fck_shared_object_is_valid(fck_shared_object so)
{
	return so.handle != NULL;
}

#if defined(_WIN32) || defined(_WIN64)
// Technically windows does not care if we provide an extension or not
// It is quite forgiving in that sense. We leave it for completeness
#define FCK_SHARED_OBJECT_EXTENSION "dll"
#elif defined(__APPLE__) && defined(__MACH__)
#define FCK_SHARED_OBJECT_EXTENSION "dylib"
#elif defined(__unix__) || defined(__unix) || defined(__linux__)
#define FCK_SHARED_OBJECT_EXTENSION "so"
#else
#error "Unsupported platform: unknown shared object extension"
#endif

static fck_shared_object fck_shared_object_load(const char *path)
{
	// This is fucked, this is fucked, this is fucked, this is fucked
	char real_path[512];

	// Portable code stinks
	// const char *path_delim_backslash = SDL_strrchr(path, '\\');
	// const char *path_delim_slash = SDL_strrchr(path, '/');
	// const char *path_delim = path_delim_backslash > path_delim_slash ? path_delim_backslash : path_delim_slash;
	const char *extension_dot = SDL_strchr(path, '.');

	const int extension_found = extension_dot != NULL; //> path_delim;
	if (!extension_found)
	{
		const int result = SDL_snprintf(real_path, sizeof(real_path), "%s.%s", path, FCK_SHARED_OBJECT_EXTENSION);
		if (result < 0)
		{
			return (fck_shared_object){.handle = NULL};
		}
		path = real_path;
	}

	SDL_SharedObject *so = SDL_LoadObject(path);
	return (fck_shared_object){.handle = (void *)so};
}

static void fck_shared_object_unload(fck_shared_object so)
{
	SDL_UnloadObject((SDL_SharedObject *)so.handle);
}

static void *fck_shared_object_symbol(fck_shared_object so, const char *name)
{
	return (void *)SDL_LoadFunction((SDL_SharedObject *)so.handle, name);
}

static fck_shared_object_api so_api = {
	.load = fck_shared_object_load,
	.symbol = fck_shared_object_symbol,
	.unload = fck_shared_object_unload,
	.is_valid = fck_shared_object_is_valid,
};

typedef struct fck_sdl_window
{
	SDL_Window *value;
	// Making these smarter would be awesome
	fck_window_configuration config;
} fck_sdl_window;

static SDL_Window *to_sdl_window(fck_window window)
{
	fck_sdl_window *sdl = (fck_sdl_window *)window.handle;
	return (SDL_Window *)sdl->value;
}

static SDL_HitTestResult fck_custom_hit_test(SDL_Window *win, const SDL_Point *area, void *data)
{
	fck_sdl_window *sdl = (fck_sdl_window *)data;

	int w, h;
	SDL_GetWindowSize(win, &w, &h);

	const int title_bar_height = to_int(sdl->config.title_bar_height);
	const int resize_border = to_int(sdl->config.resize_line_width);
	const int button_zone = to_int(sdl->config.button_area_width);
	const int menu_zone = to_int(sdl->config.menu_area_width);

	if (area->y < resize_border)
	{
		if (area->x < resize_border)
		{
			return SDL_HITTEST_RESIZE_TOPLEFT;
		}
		if (area->x > w - resize_border)
		{
			return SDL_HITTEST_RESIZE_TOPRIGHT;
		}
		return SDL_HITTEST_RESIZE_TOP;
	}
	if (area->y > h - resize_border)
	{
		if (area->x < resize_border)
		{
			return SDL_HITTEST_RESIZE_BOTTOMLEFT;
		}
		if (area->x > w - resize_border)
		{
			return SDL_HITTEST_RESIZE_BOTTOMRIGHT;
		}
		return SDL_HITTEST_RESIZE_BOTTOM;
	}
	if (area->x < resize_border)
	{
		return SDL_HITTEST_RESIZE_LEFT;
	}
	if (area->x > w - resize_border)
	{
		return SDL_HITTEST_RESIZE_RIGHT;
	}
	if (area->y < title_bar_height && area->x < (w - button_zone) && area->x > menu_zone)
	{
		return SDL_HITTEST_DRAGGABLE;
	}

	return SDL_HITTEST_NORMAL;
}

static const fck_window_configuration *fck_window_api_configuration(fck_window window, const fck_window_configuration *config)
{
	fck_sdl_window *sdl = (fck_sdl_window *)window.handle;
	if (config)
	{
		sdl->config = *config;
	}
	return &sdl->config;
}

static fck_window fck_window_api_create(const char *name, int w, int h)
{
	fck_sdl_window *sdl = (fck_sdl_window *)SDL_malloc(sizeof(*sdl));
	sdl->value = SDL_CreateWindow(name, w, h, SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS);
	sdl->config.title_bar_height = 30.0f;
	sdl->config.resize_line_width = 8.0f;
	sdl->config.button_area_width = 30.0f;
	SDL_SetWindowHitTest(sdl->value, fck_custom_hit_test, sdl);
	return (fck_window){.handle = sdl};
}

const char *fck_window_api_title(fck_window window, const char *title)
{
	if (title)
	{
		SDL_SetWindowTitle(to_sdl_window(window), title);
	}
	return SDL_GetWindowTitle(to_sdl_window(window));
}

static void fck_window_api_destroy(fck_window window)
{
	SDL_DestroyWindow(to_sdl_window(window));
	SDL_free(window.handle);
}

static int fck_window_api_is_valid(fck_window window)
{
	return window.handle != NULL;
}

static int fck_window_api_resize(fck_window window, int width, int height)
{
	return (int)SDL_SetWindowSize(to_sdl_window(window), width, height);
}

static int fck_window_api_text_input_start(fck_window window)
{
	return (int)SDL_StartTextInput(to_sdl_window(window));
}

static int fck_window_api_text_input_stop(fck_window window)
{
	return (int)SDL_StopTextInput(to_sdl_window(window));
}

static int fck_window_api_size(fck_window window, int *width, int *height)
{
	return (int)SDL_GetWindowSize(to_sdl_window(window), width, height);
}

static void *fck_window_native(fck_window window, const char *name)
{
	if (!SDL_strcmp(name, "sdl.window"))
	{
		return window.handle;
	}
	if (!SDL_strcmp(name, "win32.window"))
	{
		const SDL_PropertiesID properties = SDL_GetWindowProperties(to_sdl_window(window));
		return SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
	}
	if (!SDL_strcmp(name, "win32.instance"))
	{
		const SDL_PropertiesID properties = SDL_GetWindowProperties(to_sdl_window(window));
		return SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WIN32_INSTANCE_POINTER, NULL);
	}
	if (!SDL_strcmp(name, "macos.window"))
	{
		const SDL_PropertiesID properties = SDL_GetWindowProperties(to_sdl_window(window));
		return SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, NULL);
	}

	return NULL;
}

static int fck_clipboard_api_set(const char *text)
{
	return (int)SDL_SetClipboardText(text);
}

static int fck_clipboard_api_has(void)
{
	return (int)SDL_HasClipboardText();
}

static fck_clipboard fck_clipboard_api_receive(void)
{
	return (fck_clipboard){.text = SDL_GetClipboardText()};
}

static void fck_clipboard_api_close(fck_clipboard clipboard)
{
	SDL_free(clipboard.text);
}

static int fck_clipboard_api_is_valid(fck_clipboard clipboard)
{
	if (clipboard.text == NULL)
	{
		return 0;
	}
	return SDL_strcmp("", clipboard.text);
}

static fck_file fck_filesystem_open(const char *path, const char *mode)
{
	SDL_IOStream *stream = SDL_IOFromFile(path, mode);
	return (fck_file){.handle = (void *)stream};
}

static void fck_filesystem_close(fck_file file)
{
	SDL_CloseIO((SDL_IOStream *)file.handle);
}

static int fck_filesystem_is_valid(fck_file file)
{
	return file.handle != NULL;
}

static fckc_i64 fck_filesystem_size(fck_file file)
{
	return SDL_GetIOSize((SDL_IOStream *)file.handle);
}

static fckc_i64 fck_filesystem_seek(fck_file file, fckc_i64 offset, fckc_u32 seek_mode)
{
	SDL_IOWhence whence;
	switch ((fck_stream_seek_mode)seek_mode)
	{
	case fck_stream_cur:
		whence = SDL_IO_SEEK_CUR;
		break;
	case fck_stream_end:
		whence = SDL_IO_SEEK_END;
		break;
	case fck_stream_set:
		whence = SDL_IO_SEEK_SET;
		break;
	default:
		return -1;
	}
	return (fckc_i64)SDL_SeekIO((SDL_IOStream *)file.handle, offset, whence);
}

static fckc_size_t fck_filesystem_read(fck_file file, void *ptr, fckc_size_t size)
{
	return SDL_ReadIO((SDL_IOStream *)file.handle, ptr, size);
}

static fckc_size_t fck_filesystem_write(fck_file file, const void *ptr, fckc_size_t size)
{
	return SDL_WriteIO((SDL_IOStream *)file.handle, ptr, size);
}

static fckc_i64 fck_filesystem_flush(fck_file file)
{
	return (fckc_i64)SDL_FlushIO((SDL_IOStream *)file.handle);
}

static fckc_i64 fck_filesystem_modified(const char *path)
{
	SDL_PathInfo info;
	if (SDL_GetPathInfo(path, &info))
	{
		return info.modify_time;
	}
	return 0;
}

static int fck_filesystem_remove(const char *path)
{
	if (SDL_RemovePath(path))
	{
		return 1;
	}
	return 0;
}

static const char *fck_filesystem_local_path(void)
{
	return SDL_GetBasePath();
}

static fckc_size_t fck_glob_executable(const char *pattern, char ***out_paths)
{
	int count = 0;
	*out_paths = SDL_GlobDirectory(SDL_GetBasePath(), pattern, 0, &count);
	return to_size_t(count);
}

static fckc_size_t fck_glob_directory(const char *path, const char *pattern, char ***out_paths)
{
	int count = 0;

	*out_paths = SDL_GlobDirectory(path, pattern, 0, &count);
	return to_size_t(count);
}

static char *fck_glob_find(const char *str, const char *substring)
{
	char *result = (char *)SDL_strstr(str, substring);
	return result;
}

static char *fck_glob_match(const char *str, const char *pattern)
{
	const char *s = str;
	const char *p = pattern;
	const char *s_fallback = NULL;
	const char *p_fallback = NULL;
	const char *match = NULL;

	while (*s != '\0')
	{
		if (*p == *s || *p == '?')
		{
			// Characters match, or '?' matches any single character. Advance both.
			s++;
			p++;
		}
		else if (*p == '*')
		{
			if (match == NULL)
			{
				match = s;
			}
			// Found a '*'. Memory the current positions for potential backtracking.
			p_fallback = p;     // Match 0 characters first, stay on '*'
			s_fallback = s + 1; // Next time, try matching this 's' character with '*'
			p++;
		}
		else if (p_fallback != NULL)
		{
			// Match failed, but we encountered a '*' earlier. Backtrack!
			p = p_fallback;
			s = s_fallback;
		}
		else
		{
			// Strict mismatch and no '*' to save us.
			return NULL;
		}
	}

	// Eat up any trailing wildcards in the pattern
	while (*p == '*')
	{
		p++;
	}

	// If the entire pattern was consumed, we have a successful match.
	// Per your signature requirement, we return the start of the matched string.
	return (*p == '\0') ? (char *)match : NULL;
}

static void fck_glob_free(char **paths)
{
	SDL_free((void *)paths);
}

static fck_glob_api glob_api = {
	.find = fck_glob_find,
	.match = fck_glob_match,
	.executable = fck_glob_executable,
	.directory = fck_glob_directory,
	.free = fck_glob_free,
};

static fck_file_system_api file_system_api = {
	.open = fck_filesystem_open,
	.close = fck_filesystem_close,
	.is_valid = fck_filesystem_is_valid,
	.size = fck_filesystem_size,
	.seek = fck_filesystem_seek,
	.read = fck_filesystem_read,
	.write = fck_filesystem_write,
	.flush = fck_filesystem_flush,
	.modified = fck_filesystem_modified,
	.remove = fck_filesystem_remove,
	.executable = fck_filesystem_local_path,
};

static fck_clipboard_api clipboard_api = {
	.set = fck_clipboard_api_set,
	.has = fck_clipboard_api_has,
	.receive = fck_clipboard_api_receive,
	.close = fck_clipboard_api_close,
	.is_valid = fck_clipboard_api_is_valid,
};

static fck_window_api window_api = {
	.create = fck_window_api_create,
	.destroy = fck_window_api_destroy,
	.is_valid = fck_window_api_is_valid,
	.size = fck_window_api_size,
	.resize = fck_window_api_resize,
	.native = fck_window_native,
	.title = fck_window_api_title,
	.configuration = fck_window_api_configuration,
	.text_input_start = fck_window_api_text_input_start,
	.text_input_stop = fck_window_api_text_input_stop,
};

static fck_chrono_api chrono_api = {
	.ms = SDL_GetTicks,
};

static fck_io_api io_api = {
	.log = SDL_Log,
};

static fck_file_watcher fck_file_watcher_create(const char *path);
static fckc_size_t fck_file_watcher_changes(fck_file_watcher watcher, fck_file_watcher_event *events, fckc_size_t capacity);
static void fck_file_watcher_destroy(fck_file_watcher watcher);

static int fck_file_watcher_is_valid(fck_file_watcher watcher)
{
	return watcher.handle != NULL;
}

static fck_file_watcher_api file_watcher_api = {
	.changes = fck_file_watcher_changes,
	.create = fck_file_watcher_create,
	.destroy = fck_file_watcher_destroy,
	.is_valid = fck_file_watcher_is_valid,
};

static fck_os_api std_api = {
	.io = &io_api,
	.so = &so_api,
	.win = &window_api,
	.clipboard = &clipboard_api,
	.chrono = &chrono_api,
	.fs = &file_system_api,
	.glob = &glob_api,
	.fw = &file_watcher_api,
};

fck_os_api *os = &std_api;

#ifndef __WIN32__
#include <windows.h> // !NOLINT

#include <WinBase.h>
#include <WinNls.h>
#include <fileapi.h>
#include <handleapi.h>
#include <ioapiset.h>
#include <minwinbase.h>
#include <minwindef.h>
#include <stringapiset.h>
#include <synchapi.h>
#include <winnt.h>

typedef struct fck_file_watcher_win32
{
	HANDLE handle;
	OVERLAPPED overlapped;

	DWORD offset;
	DWORD pending;
	char buffer[65536];
} fck_file_watcher_win32;

fck_file_watcher fck_file_watcher_create(const char *path)
{
	fck_file_watcher_win32 *fs = (fck_file_watcher_win32 *)SDL_malloc(sizeof(*fs));
	const int access = GENERIC_READ | FILE_LIST_DIRECTORY;
	const int share = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
	const int flags = FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED;
	fs->handle = CreateFile(path, access, share, NULL, OPEN_EXISTING, flags, NULL);

	if (fs->handle == INVALID_HANDLE_VALUE)
	{
		return (fck_file_watcher){.handle = (void *)NULL};
	}

	fs->overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	const int notification_flags = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES |
	                               FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION;
	const fckc_size_t buffer_size = sizeof(fs->buffer);
	ReadDirectoryChangesW(fs->handle, (LPVOID)fs->buffer, (DWORD)buffer_size, TRUE, notification_flags, NULL, &fs->overlapped, NULL);
	// TODO: Handle rror
	fs->offset = 0;
	fs->pending = 0;
	return (fck_file_watcher){.handle = (void *)fs};
}

fckc_size_t fck_file_watcher_changes(fck_file_watcher watcher, fck_file_watcher_event *events, fckc_size_t capacity)
{
	fck_file_watcher_win32 *fs = (fck_file_watcher_win32 *)watcher.handle;
	if (fs == NULL)
	{
		return 0;
	}

	if (fs->offset == 0)
	{
		// result == true && fs->pending == 0 -> discarded cause internal buffer overflow
		const BOOL result = GetOverlappedResult(fs->handle, &fs->overlapped, &fs->pending, FALSE);
		if (!result)
		{
			// Maybe log error
			/*const DWORD error = GetLastError();
			if (error == ERROR_IO_INCOMPLETE)
			{
			    return 0;
			}*/
			return 0;
		}
	}

	fckc_size_t count = 0;
	while (fs->offset < fs->pending)
	{
		if (count == capacity)
		{
			return count;
		}

		fck_file_watcher_event *event = events + count;
		// Meh, maybe we get away with approximate time stamps provided by the one and only, me!
		event->time = os->chrono->ms();

		const FILE_NOTIFY_INFORMATION *notify_info = (FILE_NOTIFY_INFORMATION *)(fs->buffer + fs->offset);
		const int len = notify_info->FileNameLength / sizeof(WCHAR);
		const int filenamelen = WideCharToMultiByte(CP_ACP, 0, notify_info->FileName, len, event->path, sizeof(event->path), NULL, NULL);
		event->path[filenamelen] = '\0';

		(void)filenamelen;
		switch (notify_info->Action)
		{
		case FILE_ACTION_ADDED:
			event->type = fck_file_created;
			break;
		case FILE_ACTION_REMOVED:
			event->type = fck_file_deleted;
			break;
		case FILE_ACTION_MODIFIED:
			event->type = fck_file_modified;
			break;
		case FILE_ACTION_RENAMED_OLD_NAME:
			event->type = fck_file_deleted;
			break;
		case FILE_ACTION_RENAMED_NEW_NAME:
			event->type = fck_file_created;
			break;
		default:
			break;
		}
		count = count + 1;

		if (notify_info->NextEntryOffset == 0)
		{
			break;
		}
		fs->offset = fs->offset + notify_info->NextEntryOffset;
	}

	fs->pending = 0;
	fs->offset = 0;

	ResetEvent(fs->overlapped.hEvent);
	const int notification_flags = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES |
	                               FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION;
	const fckc_size_t buffer_size = sizeof(fs->buffer);
	ReadDirectoryChangesW(fs->handle, (LPVOID)fs->buffer, (DWORD)buffer_size, TRUE, notification_flags, NULL, &fs->overlapped, NULL);
	// TODO: Handle error
	return count;
}

void fck_file_watcher_destroy(fck_file_watcher watcher)
{
	if (!watcher.handle)
	{
		return;
	}
	fck_file_watcher_win32 *fs = (fck_file_watcher_win32 *)watcher.handle;
	CancelIo(fs->handle);
	CloseHandle(fs->overlapped.hEvent);
	CloseHandle(fs->handle);
	SDL_free(fs);
}
#else // !__WIN32__
// TODO: Uniiiiix
static fck_file_watcher fck_file_watcher_create(const char *path)
{
	return (fck_file_watcher){.handle = (void *)NULL};
}
static fckc_size_t fck_file_watcher_changes(fck_file_watcher watcher, fck_file_watcher_event *events, fckc_size_t capacity)
{
	(void)watcher;
	(void)events;
	(void)capacity;
	return 0;
}
static void fck_file_watcher_destroy(fck_file_watcher watcher)
{
	(void)watcher;
}
#endif
