#define FCK_OS_EXPORT
#include "fck_os.h"

#include <SDL3/SDL_clipboard.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_loadso.h>
#include <SDL3/SDL_log.h>
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

	int extension_found = extension_dot != NULL; //> path_delim;
	if (!extension_found)
	{
		int result = SDL_snprintf(real_path, sizeof(real_path), "%s.%s", path, FCK_SHARED_OBJECT_EXTENSION);
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

static fck_window fck_window_api_create(const char *name, int w, int h)
{
	SDL_Window *window = SDL_CreateWindow(name, w, h, SDL_WINDOW_RESIZABLE);
	return (fck_window){.handle = window};
}

static void fck_window_api_destroy(fck_window window)
{
	SDL_DestroyWindow((SDL_Window *)window.handle);
}

static int fck_window_api_is_valid(fck_window window)
{
	return window.handle != NULL;
}

static int fck_window_api_resize(fck_window window, int width, int height)
{
	return (int)SDL_SetWindowSize((SDL_Window *)window.handle, width, height);
}

static int fck_window_api_text_input_start(fck_window window)
{
	return (int)SDL_StartTextInput((SDL_Window *)window.handle);
}

static int fck_window_api_text_input_stop(fck_window window)
{
	return (int)SDL_StopTextInput((SDL_Window *)window.handle);
}

static int fck_window_api_size(fck_window window, int *width, int *height)
{
	return (int)SDL_GetWindowSize((SDL_Window *)window.handle, width, height);
}

static void *fck_window_native(fck_window window, const char *name)
{
	if (!strcmp(name, "sdl.window"))
	{ // Maybe this will be useful one day
		return window.handle;
	}
	if (!strcmp(name, "win32.window"))
	{
		SDL_PropertiesID properties = SDL_GetWindowProperties((SDL_Window *)window.handle);
		return SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
	}
	if (!strcmp(name, "win32.instance"))
	{
		SDL_PropertiesID properties = SDL_GetWindowProperties((SDL_Window *)window.handle);
		return SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WIN32_INSTANCE_POINTER, NULL);
	}
	if (!strcmp(name, "macos.window"))
	{
		SDL_PropertiesID properties = SDL_GetWindowProperties((SDL_Window *)window.handle);
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
	if(SDL_GetPathInfo(path, &info)) {
		return info.modify_time;
	}
	return 0;
}

static int fck_filesystem_remove(const char* path)
{
	if (SDL_RemovePath(path)) {
		return 1;
	}
	return 0;
}

static fckc_size_t fck_glob_local(const char *path, const char *pattern, char ***out_paths)
{
	int count = 0;
	char buffer[4096];
	int result = SDL_snprintf(buffer, sizeof(buffer), "%s%s", SDL_GetCurrentDirectory(), path);
	if (result <= 0)
	{
		return to_size_t(0);
	}

	*out_paths = SDL_GlobDirectory(SDL_GetCurrentDirectory(), pattern, 0, &count);
	return to_size_t(count);
}

char *fck_glob_find(const char *str, const char *substring)
{
	char *result = SDL_strstr(str, substring);
	return result;
}

char *fck_glob_match(const char *str, const char *pattern)
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
	.local = fck_glob_local,
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
	.text_input_start = fck_window_api_text_input_start,
	.text_input_stop = fck_window_api_text_input_stop,
};

static fck_chrono_api chrono_api = {
	.ms = SDL_GetTicks,
};

static fck_io_api io_api = {
	.log = SDL_Log,
};

static fck_os_api std_api = {
	.io = &io_api,
	.so = &so_api,
	.win = &window_api,
	.clipboard = &clipboard_api,
	.chrono = &chrono_api,
	.fs = &file_system_api,
	.glob = &glob_api,
};

fck_os_api *os = &std_api;
