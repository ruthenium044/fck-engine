
#define FCK_STD_EXPORT
#include "fck_os.h"

#include <SDL3/SDL_clipboard.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_loadso.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_log.h>

static fck_char_api char_api = {
	.isdigit = SDL_isdigit,
	.isspace = SDL_isspace,
	.isgraph = SDL_isgraph,
	.isprint = SDL_isprint,
	.iscntrl = SDL_iscntrl,
};

static fck_unsafe_string_api unsafe_string_api = {
	.cmp = SDL_strcmp,
	.dup = SDL_strdup,
	.len = SDL_strlen,
};

char *fck_string_find_graphical(char *str)
{
	if (str == NULL || *str == '\0')
	{
		return NULL;
	}
	while (*str != '\0')
	{
		if (!SDL_isgraph(*str))
		{
			str = str + 1;
			continue;
		}
		return str;
	}
	return NULL;
}

char *fck_string_find_printable(char *str)
{
	if (str == NULL || *str == '\0')
	{
		return NULL;
	}
	while (*str != '\0')
	{
		if (!SDL_isprint(*str))
		{
			str = str + 1;
			continue;
		}
		return str;
	}
	return NULL;
}

char *fck_string_find_control(char *str)
{
	if (str == NULL || *str == '\0')
	{
		return NULL;
	}
	while (*str != '\0')
	{
		if (!SDL_iscntrl(*str))
		{
			str = str + 1;
			continue;
		}
		return str;
	}
	return NULL;
}

char *fck_string_find_string(char *str, const char *other)
{
	return SDL_strstr(str, other);
}

char *fck_string_find_char(char *str, int ch)
{
	return SDL_strchr(str, ch);
}

static fck_string_find_api string_find_api = {
	.string = fck_string_find_string,
	.graphical = fck_string_find_graphical,
	.printable = fck_string_find_printable,
	.chr = fck_string_find_char,
	.control = fck_string_find_control,
};

static fck_string_api string_api = {
	.unsafe = &unsafe_string_api, //
	.find = &string_find_api,     //
	.cmp = SDL_strncmp,           //
	.dup = SDL_strndup,           //
	.len = SDL_strnlen,           //
	.toll = SDL_strtoll,          //
	.toull = SDL_strtoull,        //
	.tod = SDL_strtod,            //
};

static fck_memory_api memory_api = {
	.cpy = SDL_memcpy,
	.set = SDL_memset,
};

static fck_io_api io_api = {
	.format = SDL_snprintf,
	.log = SDL_Log,
};

static int fck_shared_object_is_valid(fck_shared_object so)
{
	return so.handle != NULL;
}

#if defined(_WIN32) || defined(_WIN64)
// Technically windows does not care if we provide an extension or not
// It is quite forgiving in that sense. We leave it for completeness
#define FCK_SHARED_OBJECT_EXTENSION ".dll"
#elif defined(__APPLE__) && defined(__MACH__)
#define FCK_SHARED_OBJECT_EXTENSION ".dylib"
#elif defined(__unix__) || defined(__unix) || defined(__linux__)
#define FCK_SHARED_OBJECT_EXTENSION ".so"
#else
#error "Unsupported platform: unknown shared object extension"
#endif

static fck_shared_object fck_shared_object_load(const char *path)
{
	char real_path[256];
	int result = SDL_snprintf(real_path, sizeof(real_path), "%s%s", path, FCK_SHARED_OBJECT_EXTENSION);
	if (result < 0)
	{
		return (fck_shared_object){.handle = NULL};
	}
	SDL_SharedObject *so = SDL_LoadObject(real_path);
	return (fck_shared_object){.handle = (void *)so};
}
static void fck_shared_object_unload(fck_shared_object so)
{
	SDL_SharedObject *sdl_so = NULL;
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
	SDL_Window *window = SDL_CreateWindow(name, w, h, 0);
	return (fck_window){.handle = window};
}

void fck_window_api_destroy(fck_window window)
{
	SDL_DestroyWindow((SDL_Window *)window.handle);
}

int fck_window_api_is_valid(fck_window window)
{
	return window.handle != NULL;
}

int fck_window_api_text_input_start(fck_window window)
{
	return (int)SDL_StartTextInput((SDL_Window *)window.handle);
}

int fck_window_api_text_input_stop(fck_window window)
{
	return (int)SDL_StopTextInput((SDL_Window *)window.handle);
}

int fck_window_api_text_size_get(fck_window window, int *width, int *height)
{
	return (int)SDL_GetWindowSize((SDL_Window *)window.handle, width, height);
}

int fck_clipboard_api_set(const char *text)
{
	return (int)SDL_SetClipboardText(text);
}

int fck_clipboard_api_has(void)
{
	return (int)SDL_HasClipboardText();
}

fck_clipboard fck_clipboard_api_receive(void)
{
	return (fck_clipboard){.text = SDL_GetClipboardText()};
}

void fck_clipboard_api_close(fck_clipboard clipboard)
{
	SDL_free(clipboard.text);
}

int fck_clipboard_api_is_valid(fck_clipboard clipboard)
{
	if (clipboard.text == NULL)
	{
		return 0;
	}
	return SDL_strcmp("", clipboard.text);
}

fck_file fck_filesystem_open(const char *path, const char *mode)
{
	SDL_IOStream *stream = SDL_IOFromFile(path, mode);
	return (fck_file){.handle = (void *)stream};
}

void fck_filesystem_close(fck_file file)
{
	SDL_CloseIO((SDL_IOStream *)file.handle);
}

int fck_filesystem_is_valid(fck_file file)
{
	return file.handle != NULL;
}

fckc_i64 fck_filesystem_size(fck_file file)
{
	return SDL_GetIOSize((SDL_IOStream *)file.handle);
}

fckc_i64 fck_filesystem_seek(fck_file file, fckc_i64 offset, fckc_u32 seek_mode)
{
	SDL_IOWhence whence;
	switch ((fck_stream_seek_mode)seek_mode)
	{
	case FCK_STREAM_CUR:
		whence = SDL_IO_SEEK_CUR;
		break;
	case FCK_STREAM_END:
		whence = SDL_IO_SEEK_END;
		break;
	case FCK_STREAM_SET:
		whence = SDL_IO_SEEK_SET;
		break;
	default:
		return -1;
	}
	return (fckc_i64)SDL_SeekIO((SDL_IOStream *)file.handle, offset, whence);
}

fckc_size_t fck_filesystem_read(fck_file file, void *ptr, fckc_size_t size)
{
	return SDL_ReadIO((SDL_IOStream *)file.handle, ptr, size);
}

fckc_size_t fck_filesystem_write(fck_file file, const void *ptr, fckc_size_t size)
{
	return SDL_WriteIO((SDL_IOStream *)file.handle, ptr, size);
}

fckc_u32 fck_filesystem_flush(fck_file file)
{
	return SDL_FlushIO((SDL_IOStream *)file.handle);
}

static fck_filesystem_api file_system_api = {
	.open = fck_filesystem_open,
	.close = fck_filesystem_close,
	.is_valid = fck_filesystem_is_valid,
	.size = fck_filesystem_size,
	.seek = fck_filesystem_seek,
	.read = fck_filesystem_read,
	.write = fck_filesystem_write,
	.flush = fck_filesystem_flush,
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
	.text_input_start = fck_window_api_text_input_start,
	.text_input_stop = fck_window_api_text_input_stop,
	.size_get = fck_window_api_text_size_get,
};

static fck_chrono_api chrono_api = {
	.ms = SDL_GetTicks,
};

static fck_os_api std_api = {
	.chr = &char_api,
	.str = &string_api,
	.mem = &memory_api,
	.io = &io_api,
	.so = &so_api,
	.win = &window_api,
	.chrono = &chrono_api,
	.fs = &file_system_api,
};

fck_os_api *os = &std_api;
