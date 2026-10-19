
#include <fck_os.h>

#include <fckc_assert.h>
#include <kll.h>
#include <kll_malloc.h>

#include <fck_dynarr.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *next_char_ignore_whitespace(const char *str)
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

static char *skip_space(char *str)
{
	while (*str)
	{
		if (!isspace(*str))
		{
			break;
		}
		str++;
	}
	return str;
}

static char *skip_until(char *str, char any)
{
	while (*str)
	{
		skip_space(str);

		// for (fckc_size_t index = 0; index < count; index++)
		{
			if (*str == any)
			{
				return str;
			}
		}

		str++;
	}
	return NULL;
}

static const char *from_args(const char *token, int argc, char **argv)
{
	int offset = strlen(token) - 1;
	for (int i = 0; i < argc; i++)
	{
		const char *path_token = strstr(argv[i], token);
		if (path_token == NULL)
		{
			continue;
		}
		const char *equals_sign = next_char_ignore_whitespace(path_token + offset);
		if (path_token != NULL && *equals_sign == '=')
		{
			return next_char_ignore_whitespace(equals_sign);
		}
	}
	return NULL;
}

static void normalise_path(char *buffer)
{
	while (*buffer)
	{
		if (*buffer == '\\')
		{
			*buffer = '/';
		}
		buffer++;
	}
}

typedef enum token_type
{
	token_ignore,
	token_identifier,
	token_semicolon,
	token_colon,

	// {}
	token_scope_open,
	token_scope_close,
	// () - EEEH, I do not like any name for this shit
	token_tuple_open,
	token_tuple_close,

	token_asterisk,

	token_struct,
	token_union,
	token_typedef,
	token_static,
	token_int,
	token_float,
	token_double,
	token_char,
	token_void,
	token_const,
	token_extern,
	token_register,
	token_auto,
	token_signed,
	token_unsigned,
	token_short,
	token_long,
	token_enum,
	token_sizeof,
	token_volatile,
	token_inline,
	token_restrict,

	token_pp_include,
	token_pp_define,
	token_pp_undef,
	token_pp_if,
	token_pp_elif,
	token_pp_else,
	token_pp_ifndef,
	token_pp_ifdef,
	token_pp_elifndef,
	token_pp_elifdef,
	token_pp_endif,
	token_pp_pragma,
	token_pp_error,
	token_pp_line,
	token_pp_warning,
	token_pp_embed,
} token_type;

typedef struct token_keyword_result
{
	const char *text;
	token_type  type;
} token_keyword_result;

#define token_keyword_custom(_text, _type) {.text = (#_text), .type = (_type)}
#define token_keyword(_text)               {.text = (#_text), .type = (token_##_text)}

static token_keyword_result searches[] = {
	token_keyword(struct),
	token_keyword(union),
	token_keyword(typedef),
	token_keyword(static),
	token_keyword(int),
	token_keyword(float),
	token_keyword(double),
	token_keyword(char),
	token_keyword(void),
	token_keyword(const),
	token_keyword(extern),
	token_keyword(register),
	token_keyword(auto),
	token_keyword(signed),
	token_keyword(unsigned),
	token_keyword(short),
	token_keyword(long),
	token_keyword(enum),
	token_keyword(sizeof),
	token_keyword(volatile),
	token_keyword(inline),
	token_keyword(restrict),

	token_keyword_custom(if, token_ignore),
	token_keyword_custom(else, token_ignore),
	token_keyword_custom(switch, token_ignore),
	token_keyword_custom(case, token_ignore),
	token_keyword_custom(default, token_ignore),
	token_keyword_custom(while, token_ignore),
	token_keyword_custom(do, token_ignore),
	token_keyword_custom(for, token_ignore),
	token_keyword_custom(goto, token_ignore),
	token_keyword_custom(break, token_ignore),
	token_keyword_custom(continue, token_ignore),
	token_keyword_custom(return, token_ignore),
	token_keyword_custom(#include, token_pp_include),
	token_keyword_custom(#define, token_pp_define),
	token_keyword_custom(#undef, token_pp_undef),
	token_keyword_custom(#if, token_pp_if),
	token_keyword_custom(#elif, token_pp_elif),
	token_keyword_custom(#else, token_pp_else),
	token_keyword_custom(#ifndef, token_pp_ifndef),
	token_keyword_custom(#ifdef, token_pp_ifdef),
	token_keyword_custom(#elifdef, token_pp_elifdef),
	token_keyword_custom(#elifndef, token_pp_elifndef),
	token_keyword_custom(#endif, token_pp_endif),
	token_keyword_custom(#pragma, token_pp_pragma),
	token_keyword_custom(#error, token_pp_error),
	token_keyword_custom(#line, token_pp_line),
	token_keyword_custom(#warning, token_pp_warning),
};

typedef struct token
{
	token_type  type;
	const char *raw;
} token;

static char *remove_comments2(char *source, char *eof)
{
	fck_assert(*source == '/');
	char *current = source + 1;

	if (*current == '\0')
	{
		// I am not even going to ask
		return current;
	}

	if (*current == '/')
	{
		char *end = strchr(current, '\n');
		end       = end ? end : eof;
		// No need to remove '\n'
		memset(source, ' ', to_size_t(end - source));
		return end;
	}
	if (*current == '*')
	{
		char *end = strstr(current, "*/");
		end       = end ? end : eof;
		// +2 Need to remove "*/"
		memset(source, ' ', to_size_t(end - source) + 2);
		return end;
	}
	return source;
}

static void remove_comments(char *source, char *eof)
{
	char *current = source;
	while (*current)
	{
		if (*current == '/')
		{
			current = remove_comments2(current, eof);
		}
		current = current + 1;
	}
}

static fckc_size_t tokens_push(token **tokens, token_type type, char *text)
{
	if (text && strlen(text) > 0)
	{
		const token tok = {
			.type = type,
			.raw  = text,
		};

		fck_dynarr_add(*tokens, tok);
		const fckc_size_t last = fck_dynarr_count(*tokens) - 1;
		return last + 1;
	}
	return 0;
}

static token *tokenise(char *source)
{
	token *tokens     = fck_dynarr_new(token, kll->system, 64);
	char  *checkpoint = NULL;

	const token_keyword_result *active[fck_arraysize(searches)];
	fckc_size_t                 active_count = 0;
	fckc_size_t                 pivot        = 0;

	char *current = source;
	while (*current)
	{
		char previous = *current;

		fckc_size_t tok = 0;
		switch (*current)
		{
		case '*':
			*current = '\0';
			tok      = tokens_push(&tokens, token_identifier, checkpoint);
			tokens_push(&tokens, token_asterisk, "*");
			break;
		case ';':
			*current = '\0';
			tok      = tokens_push(&tokens, token_identifier, checkpoint);
			tokens_push(&tokens, token_semicolon, ";");
			break;
		case ':':
			*current = '\0';
			tok      = tokens_push(&tokens, token_identifier, checkpoint);
			tokens_push(&tokens, token_colon, ":");
			break;
		case '\t':
		case '\n':
		case '\v':
		case '\f':
		case '\r':
		case ' ':
			*current = '\0';
			tok      = tokens_push(&tokens, token_identifier, checkpoint);
			break;
		case '{':
			*current = '\0';
			tok      = tokens_push(&tokens, token_identifier, checkpoint);
			tokens_push(&tokens, token_scope_open, "{");
			break;
		case '}':
			*current = '\0';
			tok      = tokens_push(&tokens, token_identifier, checkpoint);
			tokens_push(&tokens, token_scope_close, "}");
			break;
		case '(':
			*current = '\0';
			tok      = tokens_push(&tokens, token_identifier, checkpoint);
			tokens_push(&tokens, token_tuple_open, "(");
			break;
		case ')':
			*current = '\0';
			tok      = tokens_push(&tokens, token_identifier, checkpoint);
			tokens_push(&tokens, token_tuple_close, ")");
			break;
		}

		for (fckc_size_t index = 0; index < active_count; index++)
		{
			const token_keyword_result *search = active[index];
			if (*current != search->text[pivot])
			{
				const fckc_size_t last = active_count - 1;
				active[index]          = active[last];
				active[last]           = NULL;
				active_count           = last;
				// Repeat!
				index                  = index - 1;
				continue;
			}

			// Characters still the same
			// Both terminated
			if (*current == '\0')
			{
				fck_assert(tok);
				token *t = tokens + tok - 1;
				fck_assert(strcmp(t->raw, search->text) == 0);
				t->type = search->type;
			}
		}
		pivot = pivot + 1;

		{
			char *next = current + 1;
			if (*current == '\0')
			{
				checkpoint = next;
				pivot      = 0;

				// Refresh active searches
				for (fckc_size_t index = 0; index < fck_arraysize(searches); index++)
				{
					const token_keyword_result *search = searches + index;
					if (*next == *search->text)
					{
						active[active_count] = search;
						active_count         = active_count + 1;
					}
				}
			}
			current = next;
		}
	}
	return tokens;
}

static int is_function_pointer(const token *tokens, fckc_size_t begin, fckc_size_t semicolon)
{
	int count = 0;
	int stack = 0;
	for (fckc_size_t index = begin; index < semicolon; index++)
	{
		const token *current = tokens + index;
		if (current->type == token_tuple_open)
		{
			stack = stack + 1;
			continue;
		}
		if (current->type == token_tuple_close)
		{
			stack = stack - 1;
			if (stack == 0)
			{
				count = count + 1;
			}
			continue;
		}
	}
	return count;
}

static fckc_size_t goto_bitfield(const token *tokens, fckc_size_t begin, fckc_size_t semicolon)
{
	for (fckc_size_t index = begin; index < semicolon; index++)
	{
		const token *current = tokens + index;
		if (current->type == token_colon)
		{
			return index + 1;
		}
	}
	return 0;
}

static void handle_segment(const token *tokens, fckc_size_t begin, fckc_size_t semicolon)
{
	if (is_function_pointer(tokens, begin, semicolon))
	{
		return;
	}
	const token *end = tokens + semicolon;
	fck_assert(end->type == token_semicolon);
	const fckc_size_t distance = semicolon - begin;
	if (distance < 2)
	{
		// Absolute Minimum: type identifier;
		//					 ^		^		^
		//					 0		1		2
		// Fucking Count Von Count over here
		return;
	}

	const fckc_size_t bitfield = goto_bitfield(tokens, begin, semicolon);
	fckc_size_t       offset   = 0;
	if (bitfield)
	{
		const fckc_size_t bitfield_at = bitfield - 1;
		offset                        = semicolon - bitfield_at;
	}

	for (fckc_size_t index = offset; index < distance; index++)
	{
		const fckc_size_t inverted = distance - index - 1;
		const fckc_size_t at       = inverted + begin;
		const token      *current  = tokens + at;
		printf("\t%s", current->raw);
	}
	printf("\n");
}

static fckc_size_t handle_definition(const token *tokens, fckc_size_t index)
{
	fckc_size_t       checkpoint = index;
	const fckc_size_t count      = fck_dynarr_count(tokens);
	for (; index < count; index++)
	{
		const token *current = tokens + index;
		switch (current->type)
		{
		case token_union:
		case token_struct: {
			const fckc_size_t next_idx = index + 1;
			const token      *next     = tokens + next_idx;
			// The issue with all of this is, how much can be in between?
			// Maybe blacklisting a bunch of keywords and as soon as a file has them it gets excluded from reflection
			if (next->type == token_scope_open)
			{
				os->io->log("Nested %s <anonymous>", current->raw);
				index = handle_definition(tokens, next_idx + 1);
				break;
			}
			if (next->type == token_identifier)
			{
				const fckc_size_t scope_open_idx = next_idx + 1;
				const token      *scope_open     = tokens + scope_open_idx;
				if (scope_open->type == token_scope_open)
				{
					os->io->log("Nested %s %s", current->raw, next->raw);
					index = handle_definition(tokens, scope_open_idx + 1);
				}
			}
		}
		break;
		case token_semicolon:
			handle_segment(tokens, checkpoint, index);
			// New checkpoint...
			checkpoint = index + 1;
			break;
		case token_scope_open:
			handle_definition(tokens, index + 1);
			// Very much done
			return index + 1;
		case token_scope_close:
			// Very much done
			return index + 1;
		}
	}
	// Done but 100% broken
	fck_assert(0);
	return index;
}

static void classify(const token *tokens)
{
	// All of this shit. I mean all of it,
	// can break just because some knob likes macros
	const fckc_size_t count = fck_dynarr_count(tokens);

	for (fckc_size_t index = 0; index < count; index++)
	{
		const token *current = tokens + index;
		switch (current->type)
		{
		case token_union:
		case token_struct: {
			const fckc_size_t identifier_idx = index + 1;
			const token      *identifier     = tokens + identifier_idx;
			if (identifier->type == token_identifier)
			{
				const fckc_size_t scope_open_idx = identifier_idx + 1;
				const token      *scope_open     = tokens + scope_open_idx;
				if (scope_open->type == token_scope_open)
				{
					os->io->log("%s %s", current->raw, identifier->raw);
					index = handle_definition(tokens, scope_open_idx + 1);
				}
			}
			break;
		}
		}
	}
}

static void scan_file(const char *path)
{
	fck_file file = os->fs->open(path, "r");
	fck_assert(os->fs->is_valid(file));

	const fckc_i64    size   = os->fs->size(file);
	const fckc_size_t total  = size + 3;
	char             *buffer = (char *)kll_malloc(kll->system, total);
	const fckc_size_t result = os->fs->read(file, (void *)(buffer + 1), size); // Prefix newline
	fck_assert(to_i64(result) == size);

	// Streamline the algorithms a little
	buffer[0]        = '\n';
	buffer[size + 0] = '\n';
	buffer[size + 1] = '\0';

	remove_comments(buffer, buffer + total);
	token *tokens = tokenise(buffer);
	classify(tokens);
}

int main(int argc, char **argv)
{
	char buffer[4048];

	os->io->log("Generate...");

	const char *path = from_args("path", argc, argv);
	if (path == NULL)
	{
		path = "C:/Users/jukai/Documents/fck-engine";
	}
	else
	{
		return;
	}
	os->io->log(path);

	char            **paths       = NULL;
	const fckc_size_t paths_count = os->glob->directory(path, "*", &paths);
	for (fckc_size_t paths_index = 0; paths_index < paths_count; paths_index++)
	{
		const char *sub    = paths[paths_index];
		const int   offset = snprintf(buffer, sizeof(buffer), "%s/%s/", path, sub);

		fck_path_info info;
		if (os->fs->info(buffer, &info))
		{
			if (info.type == fck_path_directory)
			{
				if (*sub != '.' && strcmp(sub, "build") != 0)
				{
					char            **files       = NULL;
					const fckc_size_t files_count = os->glob->directory(buffer, "*/*.h", &files);
					for (fckc_size_t files_index = 0; files_index < files_count; files_index++)
					{
						char *file = files[files_index];
						normalise_path(file);
						(void)snprintf(buffer + offset, sizeof(buffer) - offset, "%s", file);
						// os->io->log(buffer);
						scan_file(buffer);
					}
					os->glob->free(files);
				}
			}
		}
	}
	os->glob->free(paths);

	return 0;
}