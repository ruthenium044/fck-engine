
#include <Windows.h>
#include <stdio.h>
#include <string.h>

#define STATIC_HASH_NO_HASH_EXISTS(str) 0
#define STATIC_HASH_HASH_EXISTS(str, hash) hash
#define STATIC_HASH_CHOOSE(X, SELECT, ...) SELECT

#define STATIC_HASH(...) STATIC_HASH_CHOOSE(__VA_ARGS__, STATIC_HASH_NO_HASH_EXISTS(__VA_ARGS__), STATIC_HASH_HASH_EXISTS(__VA_ARGS__))

unsigned long long hash(char *str, char *end)
{
	unsigned long long hash = 5381;
	int c;

	while (str < end && *str)
	{
		char c = *str++;
		hash = ((hash << 5) + hash) + (unsigned char)c;
	}

	return hash;
}

typedef struct simple_writer
{
	char *buffer;
	size_t position;
	size_t capacity;
} simple_writer;

typedef struct simple_reader
{
	char *buffer;
	size_t size;
	size_t capacity;
} simple_reader;

simple_writer simple_writer_create()
{
	return (simple_writer){NULL, 0, 0};
}

void simple_writer_reset(simple_writer *writer)
{
	writer->position = 0;
}

void simple_writer_free(simple_writer *writer)
{
	if (writer->buffer != NULL)
	{
		free(writer->buffer);
	}
	writer->buffer = NULL;
	writer->position = 0;
	writer->capacity = 0;
}

size_t next_power_of_two(size_t n)
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

void simple_writer_append(simple_writer *writer, const char *append_begin, const char *append_end)
{
	if (append_begin == NULL || append_end == NULL)
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

simple_reader simple_reader_create()
{
	return (simple_reader){NULL, 0, 0};
}

void simple_reader_free(simple_reader *reader)
{
	if (reader->buffer != NULL)
	{
		free(reader->buffer);
	}
	reader->buffer = NULL;
	reader->size = 0;
	reader->capacity = 0;
}

char *prev_char_ignore_whitespace(char *begin, char *str)
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

char *next_char_ignore_whitespace(char *str)
{
	if (*str == '\0')
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

char *next_char_until_non_alph_numeric(char *str)
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

char *find_not_escaped_char_ignore_whitespace(char *str, char c)
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

void process_text(simple_writer *writer, simple_reader *reader)
{
	const char static_hash_token[] = "STATIC_HASH";

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
					simple_writer_append(writer, begin, emplace_position);
					char hash_buffer[64]; // 16 hexadecimals and a bit of decoration
					unsigned long long h = hash(quotation_mark_open + 1, quotation_mark_close);
					int offset = 0;
					if (*parenthesis_close_or_comma != ',')
					{
						offset += sprintf_s(hash_buffer, sizeof(hash_buffer), ", 0x%016llX", h);

						simple_writer_append(writer, hash_buffer, hash_buffer + offset);
					}
					else
					{
						offset += sprintf_s(hash_buffer, sizeof(hash_buffer), "0x%016llX", h);

						simple_writer_append(writer, hash_buffer, hash_buffer + offset);

						current = next_char_until_non_alph_numeric(current);
					}
					begin = current;
				}
			}
		}
		current = current + 1;
		simple_writer_append(writer, begin, current);
	}
	simple_writer_append(writer, current, reader->buffer + reader->size);
}

void process_file(simple_writer *writer, simple_reader *reader, const char *file_path)
{
	HANDLE file = CreateFile(file_path,
	                         GENERIC_READ | GENERIC_WRITE, //
	                         0,                            //
	                         NULL,                         //
	                         OPEN_EXISTING,                //
	                         FILE_ATTRIBUTE_NORMAL,        //
	                         NULL);                        //

	size_t new_size = GetFileSize(file, NULL) + 1;
	reader->size = new_size;
	if (new_size > reader->capacity)
	{
		char *new_buffer = (char *)malloc(new_size);
		if (reader->buffer != NULL)
		{
			// No need to copy over
			free(reader->buffer);
		}
		reader->capacity = new_size;
		reader->buffer = new_buffer;
		reader->buffer[new_size - 1] = '\0';
	}

	int result = ReadFile(file, reader->buffer, reader->size, NULL, NULL);

	if (result)
	{
		simple_writer_reset(writer);
		process_text(writer, reader);
	}
	SetFilePointer(file, 0, NULL, FILE_BEGIN);

	// -1, added a null terminator just in case
	result = WriteFile(file, writer->buffer, writer->position - 1, NULL, NULL);

	CloseHandle(file);
}

int main(int arg, char **argv)
{
	simple_writer writer = simple_writer_create();
	simple_reader reader = simple_reader_create();
	const unsigned long long hash = STATIC_HASH("Test", 0x000000017C8CDC45);

	process_file(&writer, &reader, "C:\\Users\\jukai\\Documents\\Engine\\hash\\main.c");

	simple_writer_free(&writer);
	simple_reader_free(&reader);

	return 0;
}