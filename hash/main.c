
#include <Windows.h>
#include <string.h>

#define STATIC_HASH_0(str) 0
#define STATIC_HASH_1(str, hash) hash
#define STATIC_HASH_CHOOSE(X, SELECT, ...) SELECT

#define STATIC_HASH(...) STATIC_HASH_CHOOSE(__VA_ARGS__, STATIC_HASH_0(__VA_ARGS__), STATIC_HASH_1(__VA_ARGS__))

char *next_char_ignore_whitespace(char *str)
{
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

int main(int arg, char **argv)
{
	const unsigned long long hash = STATIC_HASH("Type");

	HANDLE file = CreateFile("C:\\Users\\jukai\\Documents\\Engine\\hash\\main.c",
	                         GENERIC_READ | GENERIC_WRITE, //
	                         0,                            //
	                         NULL,                         //
	                         OPEN_EXISTING,                //
	                         FILE_ATTRIBUTE_NORMAL,        //
	                         NULL);                        //

	size_t size = GetFileSize(file, NULL);
	LPVOID buffer = VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);


	char static_hash_substr[] = "STATIC_HASH";
	int result = ReadFile(file, buffer, size, NULL, NULL);

	DWORD offset = 0;
	char *current = buffer;
	for (size_t index = 0; index < sizeof(buffer); index++)
	{
		SetFilePointer(file, offset, NULL, FILE_BEGIN);

		char *static_hash_begin = strstr(current, static_hash_substr);
		char *begin_arguments = static_hash_begin + sizeof(static_hash_substr) - 1;
		if (begin_arguments[0] != '(' || begin_arguments[1] != '"')
		{
			current = begin_arguments;
			continue;
		}

		char *end_of_line = strchr(begin_arguments, '\n');
		// let's already prepare the next iteration so we do not have to think about it anymore...
		current = end_of_line;

		char *end_of_str_argument = NULL;

		char *end_arguments = begin_arguments + 2;
		while (1)
		{
			end_arguments = strchr(end_arguments, '"');
			if (end_arguments > end_of_line)
			{
				// Something here is broken...
				break;
			}
			char *next_char_no_whitespace = next_char_ignore_whitespace(end_arguments);
			if (end_arguments[0] == '"' && *next_char_no_whitespace == ',')
			{
				if (end_of_str_argument != NULL)
				{
					break;
				}
				end_of_str_argument = next_char_no_whitespace;
			}
			if (end_arguments[0] == '"' && end_arguments[1] == ')' && end_arguments[2] == ';' &&
			    (end_arguments[3] == '\n' || (end_arguments[3] == '\r' && end_arguments[4] == '\n')))
			{
				break;
			}
			end_arguments = end_arguments + 1;
		}

		char* str_begin = begin_arguments + 2;
		size_t argument_size = (size_t)(end_arguments - str_begin);
		if (end_of_str_argument != NULL)
		{
		}
		else if ((size_t)(end_arguments - begin_arguments) > 4)
		{
			char* str_begin = begin_arguments + 2;
			size_t argument_offset = (size_t)((str_begin) - buffer);
			SetFilePointer(file, offset + argument_offset, NULL, FILE_BEGIN);
			char write_buffer[4096];
			strncpy_s(write_buffer, sizeof(write_buffer), str_begin, argument_size);
			
			char hash[] = "\", \"0xDEADBEEF";
			strncpy_s(write_buffer + argument_size, sizeof(write_buffer), hash, sizeof(hash));

			DWORD bytes_written;
			WriteFile(file, write_buffer, strlen(write_buffer), &bytes_written, NULL);
			CloseHandle(file);
			return 1;
		}
		return 0;
	}
}