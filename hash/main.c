
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fck_hash.h"

typedef struct fck_hash_simple_writer
{
	char *buffer;
	size_t position;
	size_t capacity;
} fck_hash_simple_writer;

typedef struct fck_hash_simple_reader
{
	char *buffer;
	size_t size;
	size_t capacity;
} fck_hash_simple_reader;

typedef struct fck_hash_for_each_file_context
{
	fck_hash_simple_writer writer;
	fck_hash_simple_reader reader;

	fck_hash_simple_writer full;
	fck_hash_simple_writer entries;
} fck_hash_for_each_file_context;

static fck_hash_simple_writer simple_writer_create()
{
	return (fck_hash_simple_writer){NULL, 0, 0};
}

static void simple_writer_reset(fck_hash_simple_writer *writer)
{
	writer->position = 0;
}

static void simple_writer_free(fck_hash_simple_writer *writer)
{
	if (writer->buffer != NULL)
	{
		free(writer->buffer);
	}
	writer->buffer = NULL;
	writer->position = 0;
	writer->capacity = 0;
}

static size_t next_power_of_two(size_t n)
{
	if (n == 0)
		return 1;
	n--;
	n |= n >> 1;
	n |= n >> 2;
	n |= n >> 4;
	n |= n >> 8;
	n |= n >> 16;
	if ((size_t)(~0LLU) > 0xFFFFFFFF)
	{
		n |= n >> 32; // needed for 64-bit size_t
	}
	return n + 1;
}

static void simple_writer_append_string(fck_hash_simple_writer *writer, const char *append_begin, const char *append_end)
{
	if (append_begin == NULL || append_end == NULL)
	{
		return;
	}
	if (append_begin >= append_end)
	{
		return;
	}

	size_t append_size = (size_t)(append_end - append_begin);

	size_t write_end = writer->position + append_size;
	if (write_end >= writer->capacity)
	{
		size_t new_capacity = next_power_of_two(write_end);
		char *new_buffer = (char *)malloc(new_capacity);
		if (writer->buffer != NULL)
		{
			memcpy(new_buffer, writer->buffer, writer->position);
			free(writer->buffer);
		}

		writer->buffer = new_buffer;
		writer->capacity = new_capacity;
	}

	memcpy(writer->buffer + writer->position, append_begin, append_size);
	writer->position = write_end;
}

static void simple_writer_append_char(fck_hash_simple_writer *writer, char c)
{
	const char lc[] = {c, '\0'};
	simple_writer_append_string(writer, lc, lc + 1);
}

static fck_hash_simple_reader simple_reader_create()
{
	return (fck_hash_simple_reader){NULL, 0, 0};
}

static void simple_reader_free(fck_hash_simple_reader *reader)
{
	if (reader->buffer != NULL)
	{
		free(reader->buffer);
	}
	reader->buffer = NULL;
	reader->size = 0;
	reader->capacity = 0;
}

static char *prev_char_ignore_whitespace(char *begin, char *str)
{
	if (str < begin)
	{
		return begin;
	}
	str = str - 1;
	while (str >= begin)
	{
		if (isspace(*str))
		{
			str = str - 1;
			continue;
		}
		return str;
	}
	return begin;
}

static char *next_char_ignore_whitespace(char *str)
{
	if (str == NULL || *str == '\0')
	{
		return str;
	}
	str = str + 1;
	while (*str != '\0')
	{
		if (isspace(*str))
		{
			str = str + 1;
			continue;
		}
		return str;
	}
	return str;
}

static char *next_char_until_non_alph_numeric(char *str)
{
	if (*str == '\0')
	{
		return str;
	}
	str = str + 1;
	while (*str != '\0')
	{
		if (isalnum(*str))
		{
			str = str + 1;
			continue;
		}
		return str;
	}
	return str;
}

static char *find_not_escaped_char_ignore_whitespace(char *str, char c)
{
	while (1)
	{
		char *start = str;
		str = next_char_ignore_whitespace(start);
		if (*str == '\0')
		{
			return str;
		}
		char *prev = prev_char_ignore_whitespace(start, str);
		if (*str == c && *prev != '\\')
		{
			return str;
		}
	}
}

static void process_text(fck_hash_simple_writer *writer, fck_hash_simple_reader *reader, fck_hash_simple_writer *entries)
{
	const char static_hash_token[] = "FCK_STATIC_HASH";

	char *current = reader->buffer;
	while (1)
	{
		char *begin = current;
		char *static_hash_token_begin = current = strstr(current, static_hash_token);
		if (static_hash_token_begin == NULL)
		{
			current = begin;
			break;
		}

		char *static_hash_token_end = current = static_hash_token_begin + sizeof(static_hash_token) - 1;
		char *parenthesis_open = current = next_char_ignore_whitespace(current - 1);
		char *quotation_mark_open = current = next_char_ignore_whitespace(current);
		if (*parenthesis_open == '(' && *quotation_mark_open == '"')
		{
			// Idk about multiline tbh, we can do that later...
			char *quotation_mark_close = current = find_not_escaped_char_ignore_whitespace(current, '"');
			char *parenthesis_close_or_comma = current = next_char_ignore_whitespace(current);
			if (*quotation_mark_close == '"')
			{
				char *emplace_position = NULL;
				if (*parenthesis_close_or_comma == ',')
				{
					emplace_position = current = next_char_ignore_whitespace(current);
				}
				else if (*parenthesis_close_or_comma == ')')
				{
					emplace_position = parenthesis_close_or_comma;
				}
				if (emplace_position != NULL)
				{
					simple_writer_append_string(writer, begin, emplace_position);
					char hash_buffer[64]; // 16 hexadecimals and a bit of decoration
					unsigned long long h = fck_hash(quotation_mark_open + 1, quotation_mark_close);
					int offset = 0;
					if (*parenthesis_close_or_comma != ',')
					{
						offset += sprintf_s(hash_buffer, sizeof(hash_buffer), ", 0x%016llX", h);

						simple_writer_append_string(writer, hash_buffer, hash_buffer + offset);
					}
					else
					{
						offset += sprintf_s(hash_buffer, sizeof(hash_buffer), "0x%016llX", h);

						simple_writer_append_string(writer, hash_buffer, hash_buffer + offset);

						current = next_char_until_non_alph_numeric(current);
					}

					const char *str_begin = quotation_mark_open + 1;
					const char *str_end = quotation_mark_close;
					int size = str_end - str_begin;
					offset = sprintf_s(hash_buffer, sizeof(hash_buffer), "0x%016llX", h);
					simple_writer_append_string(entries, str_begin, str_end);
					simple_writer_append_char(entries, ':');
					simple_writer_append_string(entries, hash_buffer, hash_buffer + offset);
					simple_writer_append_char(entries, '\n');

					begin = current;
				}
			}
		}
		current = current + 1;
		simple_writer_append_string(writer, begin, current);
	}
	simple_writer_append_string(writer, current, reader->buffer + reader->size);
}

static void process_file(fck_hash_simple_writer *writer, fck_hash_simple_reader *reader, const char *file_path,
                         fck_hash_simple_writer *entries)
{
	HANDLE file = CreateFile(file_path,
	                         GENERIC_READ | GENERIC_WRITE, //
	                         0,                            //
	                         NULL,                         //
	                         OPEN_EXISTING,                //
	                         FILE_ATTRIBUTE_NORMAL,        //
	                         NULL);                        //

	size_t new_size = GetFileSize(file, NULL);
	reader->size = new_size;
	if (new_size > reader->capacity)
	{
		char *new_buffer = (char *)malloc(new_size + 1);
		if (reader->buffer != NULL)
		{
			// No need to copy over
			free(reader->buffer);
		}
		reader->capacity = new_size;
		reader->buffer = new_buffer;
	}

	if (reader->buffer == NULL)
	{
		return;
	}

	int result = ReadFile(file, reader->buffer, reader->size, NULL, NULL);
	reader->buffer[reader->size] = '\0';

	if (result)
	{
		simple_writer_reset(writer);
		process_text(writer, reader, entries);
	}
	SetFilePointer(file, 0, NULL, FILE_BEGIN);

	// -1, added a null terminator just in case
	result = WriteFile(file, writer->buffer, writer->position - 1, NULL, NULL);

	CloseHandle(file);
}

static void for_each_entry(fck_hash_for_each_file_context *context, const char *entry_directory, FILE_ID_EXTD_DIR_INFO *fileInfo);

static void for_each_entry_in_directory(fck_hash_for_each_file_context *context, const char *entry_directory)
{
	if (entry_directory == NULL)
	{
		return;
	}
	// We need to first get a handle to the directory we want to list files in.
	HANDLE dirHandle = CreateFileA(entry_directory, FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, OPEN_EXISTING,
	                               FILE_FLAG_BACKUP_SEMANTICS, 0);
	if (dirHandle == INVALID_HANDLE_VALUE)
	{
		return;
	}

	unsigned char buffer[1024];
	ZeroMemory(&buffer, sizeof(buffer));

	if (!GetFileInformationByHandleEx(dirHandle, FileIdExtdDirectoryRestartInfo, buffer, sizeof(buffer)))
	{
		return;
	};

	FILE_ID_EXTD_DIR_INFO *fileInfo = (FILE_ID_EXTD_DIR_INFO *)buffer;
	while (1)
	{
		for_each_entry(context, entry_directory, fileInfo);

		if (fileInfo->NextEntryOffset != 0)
		{
			fileInfo = (FILE_ID_EXTD_DIR_INFO *)((char *)fileInfo + fileInfo->NextEntryOffset);
		}
		else
		{
			// Check whether there are more files to fetch.
			if (!GetFileInformationByHandleEx(dirHandle, FileIdExtdDirectoryInfo, buffer, sizeof(buffer)))
			{
				const DWORD error = GetLastError();
				if (error == ERROR_NO_MORE_FILES)
				{
					break;
				}
			}
			fileInfo = (FILE_ID_EXTD_DIR_INFO *)buffer;
		}
	}
	CloseHandle(dirHandle);
}

static void for_each_file(fck_hash_for_each_file_context *context, const char *file_path, int file_path_size);

static void for_each_entry(fck_hash_for_each_file_context *context, const char *entry_directory, FILE_ID_EXTD_DIR_INFO *fileInfo)
{
	if (wcscmp(fileInfo->FileName, L".") != 0 && wcscmp(fileInfo->FileName, L"..") != 0 && fileInfo->FileName[0] != L'.')
	{
		if (wcscmp(fileInfo->FileName, L"build") != 0)
		{
			char new_path_buffer[MAX_PATH];
			int offset = sprintf(new_path_buffer, "%s", entry_directory);

			size_t wchar_count = fileInfo->FileNameLength / 2; // We accept and assume all file names are ascii
			size_t converted_chars = 0;
			int result = wcstombs_s(&converted_chars, new_path_buffer + offset, MAX_PATH - offset, fileInfo->FileName, wchar_count);
			if (result == 0)
			{
				offset = offset + converted_chars - 1; // \0 is part of converted chars count...
				if (fileInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					offset = sprintf(new_path_buffer + offset, "\\");
					if (offset >= 1)
					{
						for_each_entry_in_directory(context, new_path_buffer);
					}
				}
				else
				{
					for_each_file(context, new_path_buffer, offset);
				}
			}
		}
	}
}

static void for_each_file(fck_hash_for_each_file_context *context, const char *file_path, int file_path_size)
{
	if (file_path_size < 4)
	{
		return;
	}
	const char *suffix = file_path + (file_path_size - 4);
	if (strstr(suffix, ".c") == NULL && strstr(suffix, ".h") == NULL)
	{
		return;
	}

	process_file(&context->writer, &context->reader, file_path, &context->entries);
	simple_writer_append_string(&context->full, file_path, file_path + file_path_size);
	simple_writer_append_char(&context->full, '\n');
}

static char *path_from_args(int argc, char **argv)
{
	for (int i = 0; i < argc; i++)
	{
		char *path_token = strstr(argv[i], "path");
		if (path_token == NULL)
		{
			continue;
		}
		char *equals_sign = next_char_ignore_whitespace(path_token + 3);
		if (path_token != NULL && *equals_sign == '=')
		{
			return next_char_ignore_whitespace(equals_sign);
		}
	}
	return NULL;
}

#include <time.h>

int main(int argc, char **argv)
{

	char *path = path_from_args(argc, argv);
	if (path == NULL)
	{
		path = "C:\\Users\\jukai\\Documents\\Engine\\";
	}

	printf("%s%s\n", "Running static hash pre build step in directory: ", path);

	clock_t start = clock();

	fck_hash_for_each_file_context context;
	context.writer = simple_writer_create();
	context.reader = simple_reader_create();
	context.full = simple_writer_create();
	context.entries = simple_writer_create();

	for_each_entry_in_directory(&context, path);

	const clock_t end = clock();
	const clock_t delta = end - start;

	const double elapsed = (double)delta / CLOCKS_PER_SEC;
	printf("%s [%fs]\n", "Finishing static hash pre build step", elapsed);

	simple_writer_append_char(&context.full, '\0');
	//printf("%s", context.full.buffer);

	// Optional, the OS will take care of it
	simple_writer_free(&context.entries);
	simple_writer_free(&context.full);
	simple_reader_free(&context.reader);
	simple_writer_free(&context.writer);

	return 0;
}