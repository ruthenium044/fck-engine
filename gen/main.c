
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

	token_logical_and,
	token_logical_or,
	token_equal_equal,
	token_not_equal,
	token_less_equal,
	token_greater_equal,
	token_shift_left,
	token_shift_right,
	token_increment,
	token_decrement,

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
	text       value;
	token_type type;
} token;

typedef struct element
{
	// Might not need this later!
	token      origin;
	token_type type;
	fckc_i32   is_unsigned;
	union {
		fckc_i64 i64;
		fckc_u64 u64;
	};
} element;

typedef struct constant
{
	text   name;
	token *params;
	token *tokens;
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

static const char *token_type_string(token_type type)
{
#define CASE(x)                                                                                                                            \
	case x:                                                                                                                                \
		return #x
	switch (type)
	{
		CASE(token_ignore);
		CASE(token_end_of_file);
		CASE(token_newline);
		CASE(token_identifier);
		CASE(token_number);
		CASE(token_string_literal);
		CASE(token_char_literal);
		CASE(token_header_name);
		CASE(token_semicolon);
		CASE(token_colon);
		CASE(token_scope_open);
		CASE(token_scope_close);
		CASE(token_paren_open);
		CASE(token_paren_close);
		CASE(token_bracket_open);
		CASE(token_bracket_close);
		CASE(token_asterisk);
		CASE(token_plus);
		CASE(token_minus);
		CASE(token_slash);
		CASE(token_percent);
		CASE(token_dot);
		CASE(token_comma);
		CASE(token_ampersand);
		CASE(token_pipe);
		CASE(token_caret);
		CASE(token_tilde);
		CASE(token_exclamation);
		CASE(token_question);
		CASE(token_bigger);
		CASE(token_smaller);
		CASE(token_equals);
		CASE(token_logical_and);
		CASE(token_logical_or);
		CASE(token_equal_equal);
		CASE(token_not_equal);
		CASE(token_less_equal);
		CASE(token_greater_equal);
		CASE(token_shift_left);
		CASE(token_shift_right);
		CASE(token_increment);
		CASE(token_decrement);
		CASE(token_struct);
		CASE(token_union);
		CASE(token_typedef);
		CASE(token_static);
		CASE(token_int);
		CASE(token_float);
		CASE(token_double);
		CASE(token_char);
		CASE(token_void);
		CASE(token_const);
		CASE(token_extern);
		CASE(token_register);
		CASE(token_auto);
		CASE(token_signed);
		CASE(token_unsigned);
		CASE(token_short);
		CASE(token_long);
		CASE(token_enum);
		CASE(token_sizeof);
		CASE(token_volatile);
		CASE(token_inline);
		CASE(token_restrict);
		CASE(token_if);
		CASE(token_else);
		CASE(token_switch);
		CASE(token_case);
		CASE(token_default);
		CASE(token_while);
		CASE(token_do);
		CASE(token_for);
		CASE(token_goto);
		CASE(token_continue);
		CASE(token_break);
		CASE(token_return);
		CASE(token_pp_include);
		CASE(token_pp_define);
		CASE(token_pp_defined);
		CASE(token_pp_undef);
		CASE(token_pp_if);
		CASE(token_pp_elif);
		CASE(token_pp_ifndef);
		CASE(token_pp_ifdef);
		CASE(token_pp_elifndef);
		CASE(token_pp_elifdef);
		CASE(token_pp_else);
		CASE(token_pp_endif);
		CASE(token_pp_pragma);
		CASE(token_pp_error);
		CASE(token_pp_line);
		CASE(token_pp_warning);
		CASE(token_pp_embed);
	default:
		return "<unknown>";
	}
#undef CASE
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
	fckc_u64   hash;
	text       value;
	token_type type;
} token_keyword_result;

#define token_keyword_custom(_text, _type) {.value.raw = (#_text), .value.len = sizeof(#_text) - 1, .type = (_type)}
#define token_keyword(_text)               {.value.raw = (#_text), .value.len = sizeof(#_text) - 1, .type = (token_##_text)}

static token_keyword_result keyword_table[64];

static void register_keyword(const char *str, token_type type)
{
	fckc_size_t len  = strlen(str);
	fckc_u64    hash = fck_hash(str, len);
	fckc_u64    slot = hash % fck_arraysize(keyword_table);

	while (keyword_table[slot].hash != 0)
	{
		slot = (slot + 1) % fck_arraysize(keyword_table);
	}

	keyword_table[slot].value.raw = str;
	keyword_table[slot].value.len = len;
	keyword_table[slot].hash      = hash;
	keyword_table[slot].type      = type;
}

static const token_keyword_result *find_keyword(fckc_u64 hash)
{
	fckc_u64 slot = hash % fck_arraysize(keyword_table);
	while (keyword_table[slot].hash != hash)
	{
		if (keyword_table[slot].hash == 0)
		{
			return NULL;
		}
		slot = (slot + 1) % fck_arraysize(keyword_table);
	}
	return &keyword_table[slot];
}

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

static fckc_size_t symbol(fckc_size_t index, char *source, fckc_size_t total, token **tokens)
{
	char *current = source + index;
	for (fckc_size_t offset = 0; offset < 2; offset++)
	{
		// Very simple:
		//	2 - 0 -> 2
		//	2 - 1 -> 1
		const int size = 2 - offset;

		int symbol = 0;
		for (int accumulator = 0; accumulator < size; accumulator++)
		{
			symbol = (symbol << CHAR_BIT) | (unsigned char)current[accumulator];
		}

		token_type type = token_ignore;

		switch (symbol)
		{
		case ';':
			type = token_semicolon;
			break;
		case ':':
			type = token_colon;
			break;
		case '{':
			type = token_scope_open;
			break;
		case '}':
			type = token_scope_close;
			break;
		case '(':
			type = token_paren_open;
			break;
		case ')':
			type = token_paren_close;
			break;
		case '[':
			type = token_bracket_open;
			break;
		case ']':
			type = token_bracket_close;
			break;
		case '*':
			type = token_asterisk;
			break;
		case '+':
			type = token_plus;
			break;
		case '-':
			type = token_minus;
			break;
		case '/':
			type = token_slash;
			break;
		case '%':
			type = token_percent;
			break;
		case '.':
			type = token_dot;
			break;
		case ',':
			type = token_comma;
			break;
		case '&':
			type = token_ampersand;
			break;
		case '&&':
			type = token_logical_and;
			break;
		case '|':
			type = token_pipe;
			break;
		case '||':
			type = token_logical_or;
			break;
		case '^':
			type = token_caret;
			break;
		case '~':
			type = token_tilde;
			break;
		case '!':
			type = token_exclamation;
			break;
		case '?':
			type = token_question;
			break;
		case '>':
			type = token_bigger;
			break;
		case '<':
			type = token_smaller;
			break;
		case '=':
			type = token_equals;
			break;
		case '==':
			type = token_equal_equal;
			break;
		case '!=':
			type = token_not_equal;
			break;
		case '<=':
			type = token_less_equal;
			break;
		case '>=':
			type = token_greater_equal;
			break;
		case '<<':
			type = token_shift_left;
			break;
		case '>>':
			type = token_shift_right;
			break;
		case '\n':
			type = token_newline;
			break;
		}

		if (type != token_ignore)
		{
			tokens_push(tokens, type, current, size);
			return index + (size - 1);
		}
	}
	return index;
}

static fckc_size_t strip(fckc_size_t index, char *source, fckc_size_t total, char front, char back, token_type type, token **tokens)
{
	fck_assert(front == source[index]);

	const fckc_size_t checkpoint = index + 1; // Past marker
	for (index = checkpoint; index < total; index++)
	{
		if (source[index] == back)
		{
			const fckc_size_t len = index - checkpoint;
			tokens_push(tokens, type, source + checkpoint, len);
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
			return strip(index, source, total, '\"', '\"', token_header_name, tokens);
		case '\'':
			return strip(index, source, total, '\'', '\'', token_header_name, tokens);
		case '<':
			return strip(index, source, total, '<', '>', token_header_name, tokens);
		case '\n':
			return total;
		}
	}
	return index;
}

static token_type classify_keyword(fckc_u64 hash)
{
	// Use the hash modulo slot directly in a regular runtime switch/if chain,
	// or use a small lookup table:
	const token_keyword_result *result = find_keyword(hash);
	if (result)
	{
		return result->type;
	}
	return token_ignore;
}

static fckc_size_t number(fckc_size_t index, char *source, fckc_size_t total, token **tokens)
{
	fck_assert(isdigit(source[index]));

	const fckc_size_t checkpoint = index;
	for (index = index + 1; index < total; index++)
	{
		char c = source[index];
		if ((c == 'e' || c == 'E' || c == 'p' || c == 'P') && index + 1 < total)
		{
			char next = source[index + 1];
			if (next == '+' || next == '-')
			{
				index++;
			}
		}
		else if (!isalnum((unsigned char)c) && c != '.')
		{
			break;
		}
	}

	const fckc_size_t size = index - checkpoint;
	tokens_push(tokens, token_number, source + checkpoint, size);

	return index - 1;
}

static fckc_size_t identifier_or_keyword(fckc_size_t index, char *source, fckc_size_t total, token **tokens)
{
	fckc_size_t checkpoint = index;
	for (; index < total; index++)
	{
		char c = source[index];
		// Standard C identifier check (alphanumeric + underscore)
		if (!isalnum(c) && c != '_')
		{
			break;
		}
	}
	char             *raw  = source + checkpoint;
	const fckc_size_t len  = to_int(index - checkpoint);
	const fckc_u64    hash = fck_hash(raw, to_int(index - checkpoint)); // Match your fck_hash initial seed
	const token_type  type = classify_keyword(hash);

	if (type != token_ignore)
	{
		// We know it is a keyword
		tokens_push(tokens, type, raw, len);
	}
	else
	{
		// We just accept it as an identifier
		tokens_push(tokens, token_identifier, raw, len);
	}
	// One step back since we have to look at the terminator
	return index - 1;
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
						// We handle header_name immediately - So in this case include can expect a
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
	token *tokens = fck_dynarr_new(token, kll->system, 64);

	for (fckc_size_t index = 0; index < total; index++)
	{
		char *current = source + index;
		switch (*current)
		{
		case '\n':
			tokens_push(&tokens, token_newline, current, 1);
			break;
		case '\"':
			index = strip(index, source, total, '\"', '\"', token_string_literal, &tokens);
			break;
		case '\'':
			index = strip(index, source, total, '\'', '\'', token_char_literal, &tokens);
			break;
		case '#':
			index = preprocessor(index, source, total, &tokens);
			break;
		default:
			if (isalpha(*current) || *current == '_')
			{
				index = identifier_or_keyword(index, source, total, &tokens);
			}
			else if (isdigit(*current))
			{
				index = number(index, source, total, &tokens);
			}
			else
			{
				index = symbol(index, source, total, &tokens);
			}
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
				os->io->log("Nested %.*s <anonymous>", current->value.len, current->value.raw);
				index = handle_definition(tokens, next_idx + 1);
				break;
			}
			if (next->type == token_identifier)
			{
				const fckc_size_t scope_open_idx = next_idx + 1;
				const token      *scope_open     = tokens + scope_open_idx;
				if (scope_open->type == token_scope_open)
				{
					os->io->log("Nested %.*s %.*s", current->value.len, current->value.raw, next->value.len, next->value.raw);
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

static token *parameter_list(kll_arena *arena, const token *begin, const token *end)
{
	fckc_size_t count = end - begin;
	if (count == 0)
	{
		return NULL;
	}
	// I think + 1 * 2 should be enough to aboid retriggering a growth... We will see later! :)
	// Meh.
	token *tokens = fck_set_new(token, arena, (count + 1) * 2);

	for (const token *current = begin; current != end; current++)
	{
		switch (current->type)
		{
		case token_comma:
			break;
		case token_identifier: {
			const fckc_u64 hash = text_hash(current->value);
			fck_set_add(tokens, hash, *current);
			break;
		}
		default:
			fck_assert(0);
			break;
		}
	}
	return tokens;
}

static token *token_list(kll_arena *arena, const token *begin, const token *end)
{
	fckc_size_t count = end - begin;
	if (count == 0)
	{
		return NULL;
	}
	token           *tokens = fck_dynarr_new(token, arena, count);
	fck_dynarr_info *info   = fck_dynarr_get_info(tokens);

	info->count = count;
	memcpy(tokens, begin, count * sizeof(*tokens));
	return tokens;
}

static fckc_size_t constants_find(constant *constants, text name)
{
	const fck_hash_int hash   = text_hash(name);
	const fckc_size_t  result = fck_set_find(constants, hash);
	return result;
}

static void constants_add(constant **constants, text name, token *tokens, token *params)
{
	const constant c = {.name = name, .tokens = tokens, .params = params};
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
	const token      *end              = tokens + end_of_statement;

	token       *p          = NULL;
	const token *maybe_open = identifier + 1;
	if (maybe_open->type == token_paren_open)
	{
		const token* list = maybe_open + 1;
		for (const token *current = list; current != end; current++)
		{
			if (current->type == token_paren_close)
			{	
				p = parameter_list(arena, list, current);
				break;
			}
		}
	}

	token *o = token_list(arena, statement, end);

	constants_add(constants, identifier->value, o, p);

	return end_of_statement;
}

static fckc_size_t include(const token *tokens, fckc_size_t index)
{
	const token *include = tokens + index;
	const token *current = tokens + index + 1;
	switch (current->type)
	{
	case token_smaller:
		current = tokens + index + 2;
		if (current->type == token_identifier)
		{
			return index + 2;
		}
		break;
	case token_string_literal:
		// If we do not see smaller, we know it HAS to be a string literal, in any other case, meh! :)
		return index + 1;
	}
	return index + 1;
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

static void print_elements(/* dynarr */ const element *elements)
{
	os->io->log("PRINT ELEMENTS:");
	const fckc_size_t count = fck_dynarr_count(elements);
	for (fckc_size_t index = 0; index < count; index++)
	{
		const element *current = elements + index;
		if (current->type == token_number)
		{
			if (current->is_unsigned)
			{
				os->io->log("%llu (origin: %.*s)", (int)current->u64, current->origin.value.len, current->origin.value.raw);
			}
			else
			{
				os->io->log("%llu (origin: %.*s)", (int)current->u64, current->origin.value.len, current->origin.value.raw);
			}
		}
		else
		{
			os->io->log("%s (origin: %.*s)", token_type_string(current->type), current->origin.value.len, current->origin.value.raw);
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
	// print_tokens(tokens);

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

static fckc_size_t maybe_repeat_branch(const token *tokens, fckc_size_t index)
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

typedef struct evaluation_stack
{
	fckc_u64 taken;
	element *temp; // TODO: rename to expanded?
	int      depth;
} evaluation_stack;

static void evaluation_stack_push(evaluation_stack *stack, int condition)
{
	const int current = stack->depth;
	stack->taken      = (stack->taken & ~(1ULL << current)) | ((!!condition) << current);
	stack->depth      = current + 1;
}

static int evaluation_stack_peek(evaluation_stack *stack)
{
	if (stack->depth == 0)
	{
		return 0;
	}
	const int current = stack->depth - 1;
	return (stack->taken >> current) & 1ULL;
}

static void evaluation_stack_pop(evaluation_stack *stack)
{
	const int current = stack->depth;
	stack->depth      = current - 1;
}

static token *fill(kll_arena *arena, constant **constants, evaluation_stack *eval, token *dst, const token *src, /* set */ header *headers);

static element token_to_element(const token *current)
{
	char buf[64];
	fck_assert(current->type == token_number || current->type == token_char_literal);

	element e = {0};
	e.origin  = *current;
	e.type    = token_number;

	switch (current->type)
	{
	case token_char_literal: {
		const char *raw = current->value.raw;
		fckc_size_t len = current->value.len;
		for (fckc_size_t index = 0; index < len; index++)
		{
			unsigned char c = (unsigned char)raw[index];
			if (c == '\\' && index + 1 < len)
			{
				index = index + 1;
				switch (raw[index])
				{
				case 'n':
					c = '\n';
					break;
				case 't':
					c = '\t';
					break;
				case 'r':
					c = '\r';
					break;
				case '0':
					c = '\0';
					break;
				case '\\':
					c = '\\';
					break;
				case '\'':
					c = '\'';
					break;
				default:
					c = (unsigned char)raw[index];
					break;
				}
			}
			e.i64 = (e.i64 << 8) | c;
		}
		return e;
	case token_number: {
		char buffer[128];

		const fckc_size_t len = current->value.len < sizeof(buffer) - 1 ? current->value.len : sizeof(buffer) - 1;
		memcpy(buffer, current->value.raw, len);
		buffer[len] = '\0';

		for (fckc_size_t i = 0; i < len; i++)
		{
			if (buffer[i] == 'u' || buffer[i] == 'U')
			{
				e.is_unsigned = 1;
				break;
			}
		}

		if (buffer[0] == '0' && (buffer[1] == 'b' || buffer[1] == 'B'))
		{
			e.u64 = strtoull(buffer + 2, NULL, 2);
		}
		else
		{
			e.u64 = strtoull(buffer, NULL, 0);
		}

		if (!e.is_unsigned && e.u64 > 0x7FFFFFFFFFFFFFFFULL)
		{
			e.is_unsigned = 1;
		}
		return e;
	}
	}
	default:
		return (element){0};
	}
}

static void elements_add_fake_symbol(evaluation_stack *eval, token_type type)
{
	element e = {.type = type};
	fck_dynarr_add(eval->temp, e);
}

static void elements_add_symbol(evaluation_stack *eval, const token *origin)
{
	element e = {.origin = *origin, .type = origin->type};
	fck_dynarr_add(eval->temp, e);
}

static void elements_add_u64(evaluation_stack *eval, const token *origin, fckc_i64 value)
{
	element e = {.origin = *origin, .u64 = value, .is_unsigned = 1, .type = token_number};
	fck_dynarr_add(eval->temp, e);
}

static void elements_add_i64(evaluation_stack *eval, const token *origin, fckc_i64 value)
{
	element e = {.origin = *origin, .u64 = value, .is_unsigned = 0, .type = token_number};
	fck_dynarr_add(eval->temp, e);
}

static int token_op_precedence(token_type type)
{
	switch (type)
	{
	case token_question:
		return 1;
	case token_logical_or:
		return 2;
	case token_logical_and:
		return 3;
	case token_pipe:
		return 4;
	case token_caret:
		return 5;
	case token_ampersand:
		return 6;
	case token_equal_equal:
	case token_not_equal:
		return 7;
	case token_smaller:
	case token_bigger:
	case token_less_equal:
	case token_greater_equal:
		return 8;
	case token_shift_left:
	case token_shift_right:
		return 9;
	case token_plus:
	case token_minus:
		return 10;
	case token_asterisk:
	case token_slash:
	case token_percent:
		return 11;
	default:
		return 0;
	}
}

static void expand_condition(kll_arena *arena, constant **constants, evaluation_stack *eval, const token *src, fckc_size_t start,
                             fckc_size_t end)
{
	int is_checking_define = 0;
	int define_scope_depth = 0;

	for (fckc_size_t index = start; index < end; index++)
	{
		const token *current = src + index;

		switch (current->type)
		{
		case token_paren_open:
			if (is_checking_define)
			{
				define_scope_depth = define_scope_depth + 1;
			}
			else
			{
				elements_add_symbol(eval, current);
			}
			break;
		case token_paren_close:
			if (define_scope_depth > 0)
			{
				define_scope_depth = define_scope_depth - 1;
				if (define_scope_depth == 0)
				{
					is_checking_define = 0;
				}
			}
			else
			{
				elements_add_symbol(eval, current);
			}
			break;
		case token_identifier:
			if (is_checking_define)
			{
				const fckc_size_t result = constants_find(*constants, current->value);
				elements_add_i64(eval, current, result ? 1 : 0);

				if (define_scope_depth == 0)
				{
					is_checking_define = 0;
				}
			}
			else
			{
				const fckc_size_t result = constants_find(*constants, current->value);
				if (result)
				{
					constant *macro = *constants + result - 1;
					if (macro->tokens)
					{
						const fckc_size_t count = fck_dynarr_count(macro->tokens);
						expand_condition(arena, constants, eval, macro->tokens, 0, count);
					}
				}
				else
				{
					element e = {.type = token_number, .is_unsigned = 0, .i64 = 0};
					elements_add_i64(eval, current, 0);
				}
			}
			break;
		case token_pp_defined: {
			is_checking_define = 1;
			break;
		}
		case token_number:
		case token_char_literal: {
			element e = token_to_element(current);
			fck_dynarr_add(eval->temp, e);
			break;
		}
		default:
			// Anything that bleeds into here, I do not care! :)
			// TODO: Maybe later
			elements_add_symbol(eval, current);
			break;
		}
	}
}

static int evluate_as_boolean(element e)
{
	return e.is_unsigned ? (e.u64 != 0) : (e.i64 != 0);
}

static fckc_u64 evluate_as_u64(element e)
{
	if (e.is_unsigned)
	{
		return e.u64;
	}
	else
	{
		return to_u64(e.i64);
	}
}

static fckc_i64 evluate_as_i64(element e)
{
	if (!e.is_unsigned)
	{
		return e.i64;
	}
	else
	{
		return to_i64(e.u64);
	}
}

static element evaluate_unary(token_type op, element operand)
{
	switch (op)
	{
	case token_plus:
		return operand;

	case token_minus:
		operand.u64 = -operand.u64;
		return operand;

	case token_tilde:
		operand.u64 = ~operand.u64;
		return operand;

	case token_exclamation:
		operand.u64         = !evluate_as_boolean(operand);
		operand.is_unsigned = 0;
		return operand;

	default:
		fck_assert(0);
		return (element){0};
	}
}

static element evaluate_binary(token_type op, element lhs, element rhs)
{
	int     is_unsigned = lhs.is_unsigned || rhs.is_unsigned;
	element res         = {.type = token_number, .is_unsigned = is_unsigned};

	fckc_u64 bias = is_unsigned ? 0ULL : (1ULL << 63);
	fckc_u64 ca   = lhs.u64 ^ bias;
	fckc_u64 cb   = rhs.u64 ^ bias;

	switch (op)
	{
	case token_logical_and:
		res.u64         = evluate_as_boolean(lhs) && evluate_as_boolean(rhs);
		res.is_unsigned = 0;
		break;
	case token_logical_or:
		res.u64         = evluate_as_boolean(lhs) || evluate_as_boolean(rhs);
		res.is_unsigned = 0;
		break;
	case token_equal_equal:
		res.u64         = (ca == cb);
		res.is_unsigned = 0;
		break;
	case token_not_equal:
		res.u64         = (ca != cb);
		res.is_unsigned = 0;
		break;
	case token_smaller:
		res.u64         = (ca < cb);
		res.is_unsigned = 0;
		break;
	case token_less_equal:
		res.u64         = (ca <= cb);
		res.is_unsigned = 0;
		break;
	case token_bigger:
		res.u64         = (ca > cb);
		res.is_unsigned = 0;
		break;
	case token_greater_equal:
		res.u64         = (ca >= cb);
		res.is_unsigned = 0;
		break;
	case token_plus:
		res.u64 = lhs.u64 + rhs.u64;
		break;
	case token_minus:
		res.u64 = lhs.u64 - rhs.u64;
		break;
	case token_asterisk:
		res.u64 = lhs.u64 * rhs.u64;
		break;
	case token_ampersand:
		res.u64 = lhs.u64 & rhs.u64;
		break;
	case token_pipe:
		res.u64 = lhs.u64 | rhs.u64;
		break;
	case token_caret:
		res.u64 = lhs.u64 ^ rhs.u64;
		break;
	case token_shift_left:
		res.u64 = lhs.u64 << rhs.u64;
		break;
	case token_shift_right:
		res.u64 = is_unsigned ? (lhs.u64 >> rhs.u64) : to_u64(lhs.i64 >> rhs.i64);
		break;
	case token_slash:
		res.u64 = is_unsigned ? (lhs.u64 / rhs.u64) : to_u64(lhs.i64 / rhs.i64);
		break;
	case token_percent:
		res.u64 = is_unsigned ? (lhs.u64 % rhs.u64) : to_u64(lhs.i64 % rhs.i64);
		break;

	default:
		fck_assert(0);
		break;
	}

	return res;
}

static element evaluate_expression(fckc_size_t *index, element *elements, fckc_size_t count, int precedence);

static element evaluate_group(fckc_size_t *index, element *elements, fckc_size_t count)
{
	fck_assert(elements[*index].type == token_paren_open);

	*index = *index + 1;

	element result = evaluate_expression(index, elements, count, 0);

	fck_assert(*index < count);
	fck_assert(elements[*index].type == token_paren_close);

	*index = *index + 1;

	return result;
}

static element evaluate_ternary(fckc_size_t *index, element *elements, fckc_size_t count, element condition)
{
	fck_assert(*index < count);
	fck_assert(elements[*index].type == token_question);
	*index = *index + 1;

	element true_expr = evaluate_expression(index, elements, count, 0);

	fck_assert(*index < count);
	fck_assert(elements[*index].type == token_colon);
	*index = *index + 1;

	element false_expr = evaluate_expression(index, elements, count, 1);

	return evluate_as_boolean(condition) ? true_expr : false_expr;
}

static element evaluate_prefix(fckc_size_t *index, element *elements, fckc_size_t count)
{
	fck_assert(*index < count);
	element *current = elements + *index;

	switch (current->type)
	{
	case token_number:
		*index = *index + 1;
		return *current;
	case token_plus:
	case token_minus:
	case token_tilde:
	case token_exclamation: {
		token_type op   = current->type;
		*index          = *index + 1;
		element operand = evaluate_prefix(index, elements, count);
		return evaluate_unary(op, operand);
	case token_paren_open:
		return evaluate_group(index, elements, count);
	}
	default:
		fck_assert(0);
		return (element){0};
	}
}

// (!0 && (1 < 2) && (3 <= 3) && (4 > 2) && (5 >= 5) && (6 == 6) && (7 != 8)) || 0 ? (+10 + -2 * 3 - (8 / 4) + (9 % 5)) + ((~0 & 15) ^ (2 |
// (1 << 2)) >> 1) : 0
static element evaluate_expression(fckc_size_t *index, element *elements, fckc_size_t count, int precedence)
{
	element left = evaluate_prefix(index, elements, count);
	for (; *index < count; *index++)
	{
		element *current         = elements + *index;
		int      next_precedence = token_op_precedence(current->type);
		if (next_precedence == 0 || next_precedence < precedence)
		{
			break;
		}

		switch (current->type)
		{
		case token_question:
			left = evaluate_ternary(index, elements, count, left);
			break;
		case token_paren_open:
			evaluate_group(index, elements, count);
			break;
		case token_paren_close:
			fck_assert(0);
			break;
		default: {
			fckc_size_t   next  = *index + 1;
			const element right = evaluate_expression(&next, elements, count, next_precedence + 1);
			left                = evaluate_binary(current->type, left, right);
			*index              = next;
			break;
		}
		}
	}

	return left;
}

static int evaluate_condition(kll_arena *arena, constant **constants, evaluation_stack *eval, const token *src, fckc_size_t *index)
{
	(void)arena;
	fck_dynarr_clear(eval->temp);

	const fckc_size_t start = *index;
	const fckc_size_t end   = goto_newline(src, start);

	// We wrap our elements in open/close so we can treat everything with more generic algorithms! :)
	elements_add_fake_symbol(eval, token_paren_open);
	expand_condition(arena, constants, eval, src, start, end);
	elements_add_fake_symbol(eval, token_paren_close);

	fckc_size_t       idx    = 0;
	const fckc_size_t count  = fck_dynarr_count(eval->temp);
	element           result = evaluate_expression(&idx, eval->temp, count, 0);

	// Shit should be expanded
	// Let's do evaluation...

	print_elements(eval->temp);

	*index = end;
	return 0;
}

static token *fill(kll_arena *arena, constant **constants, evaluation_stack *eval, token *dst, const token *src, /* set */ header *headers)
{
	const fckc_size_t count = fck_dynarr_count(src);
	for (fckc_size_t index = 0; index < count; index++)
	{
		const token *current = src + index;
		switch (current->type)
		{
		case token_identifier: {
			const fckc_size_t result = constants_find(*constants, current->value);
			if (result)
			{
				constant *macro = *constants + result - 1;
				if (macro->tokens)
				{
					dst = fill(arena, constants, eval, dst, macro->tokens, headers);
				}
			}
			else
			{
				fck_dynarr_add(dst, *current);
			}
			break;
		}
		case token_pp_include: {
			const token *identifier = src + include(src, index);
			if (identifier->type == token_header_name)
			{
				header *target = headers_find(headers, identifier->value);
				if (target)
				{
					dst = fill(arena, constants, eval, dst, target->tokens, headers);
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
		case token_pp_if: {
			index           = index + 1;
			const int taken = evaluate_condition(arena, constants, eval, src, &index);
			evaluation_stack_push(eval, taken);
			if (!taken)
			{
				index = skip_branch(src, index);
				index = maybe_repeat_branch(src, index);
			}
			break;
		}
		case token_pp_elif: {
			if (evaluation_stack_peek(eval))
			{
				index = skip_branch(src, index);
				break;
			}
			index           = index + 1;
			const int taken = evaluate_condition(arena, constants, eval, src, &index);
			evaluation_stack_push(eval, taken);
			if (!taken)
			{
				index = skip_branch(src, index);
				index = maybe_repeat_branch(src, index);
			}
			break;
		}
		case token_pp_else:
			if (evaluation_stack_peek(eval))
			{
				index = skip_branch(src, index);
				break;
			}
			break;
		case token_pp_endif:
			evaluation_stack_pop(eval);
			break;
		case token_pp_elifndef:
		case token_pp_ifndef: {
			if (evaluation_stack_peek(eval))
			{
				index = skip_branch(src, index);
				break;
			}
			const token      *identifier = current + 1;
			const fckc_size_t result     = constants_find(*constants, identifier->value);
			evaluation_stack_push(eval, result ? 0 : 1);
			if (result)
			{
				index = skip_branch(src, index);
				index = maybe_repeat_branch(src, index);
			}
			break;
		}
		// maybe_repeat_branch mechanics
		case token_pp_elifdef:
		case token_pp_ifdef: {
			if (evaluation_stack_peek(eval))
			{
				index = skip_branch(src, index);
				break;
			}
			const token      *identifier = current + 1;
			const fckc_size_t result     = constants_find(*constants, identifier->value);
			evaluation_stack_push(eval, result ? 1 : 0);
			if (!result)
			{
				index = skip_branch(src, index);
				index = maybe_repeat_branch(src, index);
			}
			break;
		}
		case token_pp_pragma:
			// BIG TODO! :D
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
	}
	return dst;
}

static token *substitute(kll_arena *arena, token *tokens, /* set */ header *headers)
{
	token *result = fck_dynarr_new(token, arena, 512);

	evaluation_stack eval = {
		.taken = 0,
		.depth = 0,
		.temp  = fck_dynarr_new(element, arena, 64),
	};
	constant *constants = fck_set_new(constant, arena, 128);

	constants_add(&constants, make_text("alignof"), NULL, NULL);

	result = fill(arena, &constants, &eval, result, tokens, headers);

	// fckc_size_t it = fck_set_begin(constants);
	// while (fck_set_next(constants, it))
	//{
	//	constant *current = constants + it;

	//	printf("%.*s: ", current->name.len, current->name.raw);
	//	if (current->tokens)
	//	{
	//		const fckc_size_t count = fck_dynarr_count(current->tokens);
	//		for (fckc_size_t index = 0; index < count; index++)
	//		{
	//			const token *token = current->tokens + index;
	//			printf("%.*s", token->value.len, token->value.raw);
	//		}
	//	}
	//	else
	//	{
	//		printf("defined");
	//	}
	//	printf("\n");
	//}

	// print_tokens(result);

	return result;
}

void init_keywords(void)
{
	register_keyword("struct", token_struct);
	register_keyword("union", token_union);
	register_keyword("typedef", token_typedef);
	register_keyword("static", token_static);
	register_keyword("int", token_int);
	register_keyword("float", token_float);
	register_keyword("double", token_double);
	register_keyword("char", token_char);
	register_keyword("void", token_void);
	register_keyword("const", token_const);
	register_keyword("extern", token_extern);
	register_keyword("register", token_register);
	register_keyword("auto", token_auto);
	register_keyword("signed", token_signed);
	register_keyword("unsigned", token_unsigned);
	register_keyword("short", token_short);
	register_keyword("long", token_long);
	register_keyword("enum", token_enum);
	register_keyword("sizeof", token_sizeof);
	register_keyword("volatile", token_volatile);
	register_keyword("inline", token_inline);
	register_keyword("restrict", token_restrict);

	register_keyword("defined", token_pp_defined);
	register_keyword("if", token_ignore);
	register_keyword("else", token_ignore);
	register_keyword("switch", token_ignore);
	register_keyword("case", token_ignore);
	register_keyword("default", token_ignore);
	register_keyword("while", token_ignore);
	register_keyword("do", token_ignore);
	register_keyword("for", token_ignore);
	register_keyword("goto", token_ignore);
	register_keyword("break", token_ignore);
	register_keyword("continue", token_ignore);
	register_keyword("return", token_ignore);
}

int main(int argc, char **argv)
{
	init_keywords();

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
	}

	return 0;
}