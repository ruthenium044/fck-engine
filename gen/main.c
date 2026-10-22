
#include <fck_os.h>

#include <fckc_assert.h>
#include <kll.h>
#include <kll_format.h>
#include <kll_malloc.h>

#include <fck_dynarr.h>
#include <fck_set.h>

#include <fck_hash.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum token_type
{
	token_ignore,
	token_end_of_file,
	token_newline,
	token_identifier,
	token_number,
	token_string_literal,
	token_char_literal,

	token_header_name,

	token_semicolon,
	token_colon,

	// {}
	token_scope_open,
	token_scope_close,
	// ()
	token_paren_open,
	token_paren_close,
	// []
	token_bracket_open,
	token_bracket_close,

	token_asterisk,
	token_plus,
	token_minus,
	token_slash,
	token_percent,
	token_dot,
	token_comma,
	token_ampersand,
	token_pipe,
	token_caret,
	token_tilde,
	token_exclamation,
	token_question,
	token_bigger,
	token_smaller,
	token_equals,

	// Keywords
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
	token_if,
	token_else,
	token_switch,
	token_case,
	token_default,
	token_while,
	token_do,
	token_for,
	token_goto,
	token_continue,
	token_break,
	token_return,

	// Preprocessor Directives
	token_pp_include,
	token_pp_define,
	token_pp_defined,
	token_pp_undef,
	token_pp_if,
	token_pp_elif,
	token_pp_ifndef,
	token_pp_ifdef,
	token_pp_elifndef,
	token_pp_elifdef,
	token_pp_else,
	token_pp_endif,
	token_pp_pragma,
	token_pp_error,
	token_pp_line,
	token_pp_warning,
	token_pp_embed,
} token_type;

typedef struct text
{
	const char *raw;
	fckc_size_t len;
} text;

#define make_text(str) (text){"" str "", sizeof(str) - 1}

struct token;
typedef struct token
{
	text        value;
	token_type  type;
	fckc_size_t next;
} token;

typedef struct constant
{
	text name;
	text value;
} constant;

typedef struct header
{
	const char *name;
	const char *path;

	token *tokens;
} header;

typedef struct compilation
{
	const char *name;
	const char *path;

	token *tokens;
} compilation;

static fck_hash_int text_hash(text lhs)
{
	return fck_hash(lhs.raw, lhs.len);
}

static int text_same(const text *lhs, const text *rhs)
{
	if (lhs->len != rhs->len)
	{
		return 0;
	}

	return memcmp(lhs->raw, rhs->raw, lhs->len) == 0;
}

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

typedef struct token_keyword_result
{
	text       value;
	token_type type;
} token_keyword_result;

#define token_keyword_custom(_text, _type) {.value.raw = (#_text), .value.len = sizeof(#_text) - 1, .type = (_type)}
#define token_keyword(_text)               {.value.raw = (#_text), .value.len = sizeof(#_text) - 1, .type = (token_##_text)}

static token_keyword_result pp_searches[] = {
	token_keyword_custom(#include, token_pp_include), token_keyword_custom(#define, token_pp_define),
	token_keyword_custom(#undef, token_pp_undef),     token_keyword_custom(#if, token_pp_if),
	token_keyword_custom(#elif, token_pp_elif),       token_keyword_custom(#else, token_pp_else),
	token_keyword_custom(#ifndef, token_pp_ifndef),   token_keyword_custom(#ifdef, token_pp_ifdef),
	token_keyword_custom(#elifdef, token_pp_elifdef), token_keyword_custom(#elifndef, token_pp_elifndef),
	token_keyword_custom(#endif, token_pp_endif),     token_keyword_custom(#pragma, token_pp_pragma),
	token_keyword_custom(#error, token_pp_error),     token_keyword_custom(#line, token_pp_line),
	token_keyword_custom(#warning, token_pp_warning),
};

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

	token_keyword_custom(defined, token_pp_defined),

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
};

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

static void replace_backslash(char *source)
{
	char *current = source;
	while (*current)
	{
		current = strchr(current, '\\');
		if (current == NULL)
		{
			break;
		}
		char *next = current + 1;
		if (*next == '\n')
		{
			*current = ' ';
			*next    = ' ';
		}
		if (*next == '\r')
		{
			char *last = next + 1;
			if (*last == '\n')
			{
				*current = ' ';
				*last    = ' ';
				*next    = ' ';
			}
		}
		current = current + 1;
	}
}

static fckc_size_t tokens_push(token **tokens, token_type type, char *raw, fckc_size_t len)
{
	if (raw && len > 0)
	{
		const token tok = {
			.type  = type,
			.value = (text){raw, len},
		};

		fck_dynarr_add(*tokens, tok);
		const fckc_size_t last = fck_dynarr_count(*tokens) - 1;
		return last + 1;
	}
	return 0;
}

static token_type char_to_token_type(char c)
{
	switch (c)
	{
	case ';':
		return token_semicolon;
	case ':':
		return token_colon;
	case '{':
		return token_scope_open;
	case '}':
		return token_scope_close;
	case '(':
		return token_paren_open;
	case ')':
		return token_paren_close;
	case '[':
		return token_bracket_open;
	case ']':
		return token_bracket_close;
	case '*':
		return token_asterisk;
	case '+':
		return token_plus;
	case '-':
		return token_minus;
	case '/':
		return token_slash;
	case '%':
		return token_percent;
	case '.':
		return token_dot;
	case ',':
		return token_comma;
	case '&':
		return token_ampersand;
	case '|':
		return token_pipe;
	case '^':
		return token_caret;
	case '~':
		return token_tilde;
	case '!':
		return token_exclamation;
	case '?':
		return token_question;
	case '>':
		return token_bigger;
	case '<':
		return token_smaller;
	case '=':
		return token_equals;
	case '\n':
		return token_newline;
	default:
		return token_ignore;
	}
}

static fckc_size_t literal(fckc_size_t index, char *source, fckc_size_t total, token **tokens)
{
	const char c = source[index];
	fck_assert(c == '\'' || c == '\"');

	const fckc_size_t checkpoint = index + 1; // Past marker
	for (index = checkpoint; index < total; index++)
	{
		if (source[index] == c)
		{
			const token_type  type = c == '\'' ? token_char_literal : token_string_literal;
			// End-exclusive math ;-)
			const fckc_size_t len  = index - checkpoint;
			tokens_push(tokens, type, source + checkpoint, len);
			break;
		}
	}
	return index;
}

static fckc_size_t header_name(fckc_size_t index, char *source, fckc_size_t total, token **tokens)
{
	const char c = source[index];
	fck_assert(c == '<');

	const fckc_size_t checkpoint = index + 1; // Past marker
	for (index = checkpoint; index < total; index++)
	{
		if (source[index] == '>')
		{
			// End-exclusive math ;-)
			const fckc_size_t len = index - checkpoint;
			tokens_push(tokens, token_header_name, source + checkpoint, len);
			break;
		}
	}
	return index;
}

static fckc_size_t included_header_name(fckc_size_t index, char *source, fckc_size_t total, token **tokens)
{
	for (index = index + 1; index < total; index++)
	{
		char *current = source + index;
		switch (*current)
		{
		case '\"':
		case '\'':
			index = literal(index, source, total, tokens);
			return index;
		case '<':
			index = header_name(index, source, total, tokens);
			return index;
		case '\n':
			return total;
		}
	}
	return index;
}

static fckc_size_t preprocessor(fckc_size_t index, char *source, fckc_size_t total, token **tokens)
{
	const char c = source[index];
	fck_assert(c == '#');

	for (index = index + 1; index < total; index++)
	{
		// We except any letter after # - If not we just scan until the end, which is super duper fine lol
		char *begin = source + index;
		if (*begin >= 'a' && *begin <= 'z')
		{
			for (index = index + 1; index < total; index++)
			{
				char *end = source + index;
				if (*end < 'a' || *end > 'z')
				{
					token_type        type = token_ignore;
					const fckc_size_t len  = end - begin;
					switch (len)
					{
					case 2:
						if (memcmp(begin, "if", len) == 0)
						{
							type = token_pp_if;
						}
						break;
					case 4:
						if (memcmp(begin, "else", len) == 0)
						{
							type = token_pp_else;
						}
						else if (memcmp(begin, "elif", len) == 0)
						{
							type = token_pp_elif;
						}
						else if (memcmp(begin, "line", len) == 0)
						{
							type = token_pp_line;
						}
						break;

					case 5:
						if (memcmp(begin, "error", len) == 0)
						{
							type = token_pp_error;
						}
						else if (memcmp(begin, "ifdef", len) == 0)
						{
							type = token_pp_ifdef;
						}
						else if (memcmp(begin, "endif", len) == 0)
						{
							type = token_pp_endif;
						}
						else if (memcmp(begin, "undef", len) == 0)
						{
							type = token_pp_undef;
						}
						else if (memcmp(begin, "embed", len) == 0)
						{
							type = token_pp_embed;
						}
						break;

					case 6:
						if (memcmp(begin, "define", len) == 0)
						{
							type = token_pp_define;
						}
						else if (memcmp(begin, "ifndef", len) == 0)
						{
							type = token_pp_ifndef;
						}
						else if (memcmp(begin, "pragma", len) == 0)
						{
							type = token_pp_pragma;
						}
						break;

					case 7:
						if (memcmp(begin, "include", len) == 0)
						{
							type = token_pp_include;
						}
						else if (memcmp(begin, "warning", len) == 0)
						{
							type = token_pp_warning;
						}
						else if (memcmp(begin, "defined", len) == 0)
						{
							type = token_pp_defined;
						}
						break;

					case 8:
						if (memcmp(begin, "elifdef", len) == 0)
						{
							type = token_pp_elifdef;
						}
						break;

					case 9:
						if (memcmp(begin, "elifndef", len) == 0)
						{
							type = token_pp_elifndef;
						}
						break;
					default:
						return total;
					}
					if (type == token_ignore)
					{
						return total;
					}

					tokens_push(tokens, type, begin, len);

					switch (type)
					{
					case token_ignore:
						return total;
					case token_pp_include:
						return included_header_name(index, source, total, tokens);
						break;
					}
					return index;
				}
			}
		}
	}
	return total;
}

static token *prepare(char *source, fckc_size_t total)
{
	token *tokens     = fck_dynarr_new(token, kll->system, 64);
	char  *checkpoint = NULL;

	for (fckc_size_t index = 0; index < total; index++)
	{
		char *current = source + index;
		switch (*current)
		{
		case '\"':
		case '\'':
			index = literal(index, source, total, &tokens);
			break;
		case '#':
			index = preprocessor(index, source, total, &tokens);
			break;
		}
	}
	tokens_push(&tokens, token_end_of_file, NULL, 0);
	return tokens;
}

static int is_function_pointer(const token *tokens, fckc_size_t begin, fckc_size_t semicolon)
{
	int count = 0;
	int stack = 0;
	for (fckc_size_t index = begin; index < semicolon; index++)
	{
		const token *current = tokens + index;
		if (current->type == token_paren_open)
		{
			stack = stack + 1;
			continue;
		}
		if (current->type == token_paren_close)
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
		if (current->type != token_newline)
		{
			printf("\t%.*s", (int)current->value.len, current->value.raw);
		}
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
				os->io->log("Nested %s <anonymous>", current->value);
				index = handle_definition(tokens, next_idx + 1);
				break;
			}
			if (next->type == token_identifier)
			{
				const fckc_size_t scope_open_idx = next_idx + 1;
				const token      *scope_open     = tokens + scope_open_idx;
				if (scope_open->type == token_scope_open)
				{
					os->io->log("Nested %s %s", current->value, next->value);
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

static fckc_size_t goto_newline(const token *tokens, fckc_size_t index)
{
	const fckc_size_t count = fck_dynarr_count(tokens);
	for (; index < count; index++)
	{
		const token *current = tokens + index;
		if (current->type == token_newline || current->type == token_end_of_file)
		{
			break;
		}
	}
	return index;
}

static text stringify(kll_arena *arena, const token *begin, const token *end)
{
	fckc_size_t total = 0;
	for (const token *current = begin; current != end; current++)
	{
		total = total + current->value.len;
	}
	if (total == 0)
	{
		return make_text("");
	}

	char *memory = (char *)kll_malloc(arena, total + 1);
	char *at     = memory;
	for (const token *current = begin; current != end; current++)
	{
		memcpy(at, current->value.raw, current->value.len);
		at = at + current->value.len;
	}
	memory[total] = '\0';
	return (text){.raw = memory, .len = total};
}

static fckc_size_t constants_find(constant *constants, text name)
{
	const fck_hash_int hash   = text_hash(name);
	const fckc_size_t  result = fck_set_find(constants, hash);
	return result;
}

static void constants_add(constant **constants, text name, text value)
{
	const constant c = {.name = name, .value = value};
	fck_set_add(*constants, text_hash(name), c);
}

static fckc_size_t define(kll_arena *arena, /* dynarr */ const token *tokens, /* set */ constant **constants, fckc_size_t index)
{
	const fckc_size_t count = fck_dynarr_count(tokens);
	const fckc_size_t begin = index;

	const token *include = tokens + index;

	for (; index < count; index++)
	{
		const token *current = tokens + index;
		if (current->type == token_newline)
		{
			// Ok we are done
			break;
		}
	}
	const token      *identifier       = include + 1;
	const token      *statement        = identifier + 1;
	const fckc_size_t end_of_statement = index;

	text raw = stringify(arena, statement, tokens + end_of_statement);

	constants_add(constants, identifier->value, raw);

	return end_of_statement;
}

static fckc_size_t include(token *tokens, fckc_size_t index)
{
	token       *include = tokens + index;
	const token *current = tokens + index + 1;
	switch (current->type)
	{
	case token_smaller:
		current = tokens + index + 2;
		if (current->type == token_identifier)
		{
			include->next = index + 2;
			return index + 2;
		}
		break;
	case token_string_literal:
		// If we do not see smaller, we know it HAS to be a string literal, in any other case, meh! :)
		return index + 1;
	}
	return index + 1;
}

static void classify(kll_arena *arena, token *tokens)
{
	// All of this shit. I mean all of it,
	// can break just because some knob likes macros
	const fckc_size_t count = fck_dynarr_count(tokens);

	for (fckc_size_t index = 0; index < count; index++)
	{
		token *current = tokens + index;
		switch (current->type)
		{
		case token_pp_include: {
			index = include(tokens, index);
			index = goto_newline(tokens, index);
			break;
		}
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
					index = handle_definition(tokens, scope_open_idx + 1);
				}
			}
			break;
		}
		}
	}
}

static token *tokenise(kll_arena *arena, const char *path)
{
	os->io->log("Processing... %s", path);

	fck_file file = os->fs->open(path, "r");
	fck_assert(os->fs->is_valid(file));

	const fckc_i64    size   = os->fs->size(file);
	// const fckc_size_t total  = size;
	char             *buffer = (char *)kll_malloc(kll->system, size + 1);
	const fckc_size_t result = os->fs->read(file, (void *)(buffer), size); // Prefix newline
	fck_assert(to_i64(result) == size);

	// Streamline the algorithms a little
	// buffer[0]        = '\n';
	// buffer[size + 0] = '\n';
	buffer[size] = '\0';

	remove_comments(buffer, buffer + size);
	replace_backslash(buffer);

	token *tokens = prepare(buffer, size);
	classify(arena, tokens);

	return tokens;
}

static const char *skip_until_last(const char *str, char c)
{
	const char *checkpoint = str;
	while (*str)
	{
		if (*str == c)
		{
			checkpoint = str + 1;
		}
		str++;
	}
	return checkpoint;
}

static const char *file_extension(const char *str)
{
	const char *extension = strrchr(str, '.');
	if (extension)
	{
		return extension + 1;
	}
	return "";
}

static header *headers_find(header *headers, text name)
{
	const fckc_size_t result = fck_set_find(headers, text_hash(name));
	if (result)
	{
		return headers + result - 1;
	}
	return NULL;
}

static void setup_system(kll_arena *arena, const char *system, /*set*/ header **headers)
{
	char      **paths  = NULL;
	fckc_size_t result = os->glob->directory(system, "*/*.h", &paths);

	for (fckc_size_t index = 0; index < result; index++)
	{
		char *path = paths[index];
		normalise_path(path);

		const char *full_path = kll_format(arena, "%s/%s", system, path);
		const char *name      = skip_until_last(full_path, '/');
		header      target    = {.path = full_path, .name = name};
		fck_set_add(*headers, fck_hash(name, strlen(name)), target);
	}

	os->glob->free(paths);
}

static void setup_user_directory(kll_arena *arena, const char *source, /*set*/ header **headers, /* dynarr */ compilation **compilations)
{
	char buffer[1024];

	char      **paths  = NULL;
	fckc_size_t result = os->glob->directory(source, NULL, &paths);

	for (fckc_size_t index = 0; index < result; index++)
	{
		char *path = paths[index];
		normalise_path(path);
		(void)snprintf(buffer, sizeof(buffer), "%s/%s", source, path);

		fck_path_info info;
		if (os->fs->info(buffer, &info))
		{
			if (info.type == fck_path_file)
			{
				const char *extension = file_extension(path);

				if (strcmp(extension, "h") == 0 || strcmp(extension, "inl") == 0)
				{
					const char *full_path = kll_format(arena, "%s", buffer);
					const char *name      = skip_until_last(full_path, '/');
					header     *result    = &fck_set_at(*headers, fck_hash(name, strlen(name)));

					result->path = full_path;
					result->name = name;
				}
				if (strcmp(extension, "c") == 0)
				{
					const char *full_path = kll_format(arena, "%s", buffer);
					const char *name      = skip_until_last(full_path, '/');
					compilation target    = {.path = full_path, .name = name};
					fck_dynarr_add(*compilations, target);
				}
			}
		}
	}

	os->glob->free(paths);
}

static void setup_user(kll_arena *arena, const char *source, /*set*/ header **headers, /* dynarr */ compilation **compilations)
{
	char buffer[1024];

	char      **directories;
	fckc_size_t directories_count = os->glob->directory(source, "*", &directories);
	for (fckc_size_t index = 0; index < directories_count; index++)
	{
		char *directory = directories[index];
		if (strcmp(directory, "build") == 0)
		{
			continue;
		}

		normalise_path(directory);
		(void)snprintf(buffer, sizeof(buffer), "%s/%s", source, directory);

		fck_path_info info;
		if (os->fs->info(buffer, &info))
		{
			if (info.type == fck_path_directory)
			{
				setup_user_directory(arena, buffer, headers, compilations);
			}
		}
	}

	os->glob->free(directories);
}

static fckc_size_t repeat_branch(const token *tokens, fckc_size_t index)
{
	const token *current = tokens + index;
	switch (current->type)
	{
	case token_pp_elif:
	case token_pp_elifndef:
	case token_pp_elifdef:
		// We need to go before the pp token to repeat it since these branches need
		// More evaluation
		return index - 1;
	case token_pp_else:
		// Explicitly here to illustrate that the else branch on skip will be treated
		// like any other branch that is used!
		return index;
	}
	return index;
}

static fckc_size_t skip_branch(const token *tokens, fckc_size_t index)
{
	int               depth = 0;
	const fckc_size_t count = fck_dynarr_count(tokens);

	for (; index < count; index++)
	{
		const token *current = tokens + index;
		switch (current->type)
		{
		case token_pp_if:
		case token_pp_ifndef:
		case token_pp_ifdef:
			depth++;
			break;

		case token_pp_endif:
			depth--;
			if (depth < 0)
			{
				return index;
			}
			break;

		case token_pp_elif:
		case token_pp_else:
		case token_pp_elifndef:
		case token_pp_elifdef:
			if (depth == 0)
			{
				return index;
			}
			break;
		}
	}
	return index;
}

static token *fill(kll_arena *arena, constant **constants, token *dst, const token *src, /* set */ header *headers)
{
	const fckc_size_t count = fck_dynarr_count(src);
	for (fckc_size_t index = 0; index < count; index++)
	{

		const token *current = src + index;
		switch (current->type)
		{
		case token_pp_include: {
			const token *identifier = src + current->next;
			if (identifier->type == token_identifier)
			{
				header *target = headers_find(headers, identifier->value);
				if (target)
				{
					dst = fill(arena, constants, dst, target->tokens, headers);
				}
				index = goto_newline(src, index);
			}
			break;
		}
		case token_pp_define:
			index = define(arena, src, constants, index);
			index = goto_newline(src, index);
			break;
		case token_pp_undef: {
			const token *identifier = current + 1;
			if (identifier->type == token_identifier)
			{
				fck_set_remove(*constants, text_hash(identifier->value));
			}
			index = goto_newline(src, index);
			break;
		}
		case token_pp_if:
			// TODO
			break;
		case token_pp_elif:
			// TODO
			break;
		case token_pp_ifndef: {
			const token      *identifier = current + 1;
			const fckc_size_t result     = constants_find(*constants, identifier->value);
			if (result)
			{
				index = skip_branch(src, index);
			}
			index = repeat_branch(src, index);
			break;
		}
		case token_pp_ifdef: {
			const token      *identifier = current + 1;
			const fckc_size_t result     = constants_find(*constants, identifier->value);
			if (!result)
			{
				index = skip_branch(src, index);
			}
			index = repeat_branch(src, index);
			break;
		}
		case token_pp_elifndef:
			break;
		case token_pp_elifdef:
			break;
		case token_pp_else:
			break;
		case token_pp_endif:
			break;
		case token_pp_pragma:
			break;
		case token_pp_error:
			break;
		case token_pp_line:
			break;
		case token_pp_warning:
			break;
		case token_pp_embed:
			break;

		default:
			fck_dynarr_add(dst, *current);
			break;
		}

		fck_dynarr_add(dst, *current);
	}
	return dst;
}

static void print_tokens(/* dynarr */ token *tokens)
{
	os->io->log("PRINT TOKENS:");
	const fckc_size_t count = fck_dynarr_count(tokens);
	for (fckc_size_t index = 0; index < count; index++)
	{
		token *current = tokens + index;
		printf("%.*s", (int)current->value.len, current->value.raw);
	}
}

static token *substitute(kll_arena *arena, token *tokens, /* set */ header *headers)
{
	token *result = fck_dynarr_new(token, arena, 512);

	constant *constants = fck_set_new(constant, arena, 128);
	constants_add(&constants, make_text("alignof"), make_text(""));

	result = fill(arena, &constants, result, tokens, headers);

	print_tokens(result);

	return result;

	// const fckc_size_t count = fck_dynarr_count(tokens);
	// for (fckc_size_t index = 0; index < count; index++)
	//{

	//	token *current = tokens + index;

	//	switch (current->type)
	//	{
	//	case token_pp_include: {
	//		token *identifier = tokens + current->next;
	//		if (identifier->type == token_identifier)
	//		{
	//			header *target = headers_find(headers, identifier->raw);
	//			if (target)
	//			{
	//				result = fill(constants, result, target->tokens, headers);
	//			}
	//			index = identifier - tokens;
	//		}
	//		break;
	//	}
	//	case token_pp_define:
	//		index = define(arena, tokens, &constants, index);
	//		break;
	//	case token_pp_undef: {
	//		const token *identifier = tokens + 1;
	//		if (identifier->type == token_identifier)
	//		{
	//			fck_set_remove(constants, fck_hash(identifier->raw, strlen(identifier->raw)));
	//		}
	//		break;
	//	}
	//	case token_pp_if:
	//		// TODO
	//		break;
	//	case token_pp_elif:
	//		// TODO
	//		break;
	//	case token_pp_ifndef:
	//		// TODO
	//		break;
	//	case token_pp_ifdef:
	//		break;
	//	case token_pp_elifndef:
	//		break;
	//	case token_pp_elifdef:
	//		break;
	//	case token_pp_else:
	//		break;
	//	case token_pp_endif:
	//		break;
	//	case token_pp_pragma:
	//		break;
	//	case token_pp_error:
	//		break;
	//	case token_pp_line:
	//		break;
	//	case token_pp_warning:
	//		break;
	//	case token_pp_embed:
	//		break;

	//	default:
	//		fck_dynarr_add(result, *current);
	//		break;
	//	}
	//}
	// print_tokens(result);
	// return result;
}

int main(int argc, char **argv)
{
	char buffer[4048];

	os->io->log("Generate...");

#if defined(fck_gen_system_dir)
	const char *system = fck_gen_system_dir;
#else
	const char *system = from_args("system", argc, argv);
	if (system == NULL)
	{
		return -1;
	}
#endif
#if defined(fck_gen_target_path)
	const char *path = fck_gen_target_path;
#else
	const char *path = from_args("path", argc, argv);
	if (path == NULL)
	{
		return -1;
	}
#endif
	os->io->log(system);
	os->io->log(path);

	kll_arena   *arena        = kll->arena->create(kll->system, 4096);
	header      *headers      = fck_set_new(header, arena, 512);
	compilation *compilations = fck_dynarr_new(compilation, arena, 512);

	setup_user(arena, path, &headers, &compilations);

	fckc_size_t it = fck_set_begin(headers);
	while (fck_set_next(headers, it))
	{
		header *value = headers + it;
		value->tokens = tokenise(arena, value->path);
	}

	const fckc_size_t count = fck_dynarr_count(compilations);
	for (fckc_size_t index = 0; index < count; index++)
	{
		compilation *value = compilations + index;
		value->tokens      = tokenise(arena, value->path);
		value->tokens      = substitute(arena, value->tokens, headers);
		break;
	}

	return 0;
}