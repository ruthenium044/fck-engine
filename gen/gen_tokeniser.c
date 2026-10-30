
#include "gen_tokeniser.h"

#include <kll.h>
#include <kll_malloc.h>

#include <fck_dynarr.h>
#include <fck_set.h>

#include <fckc_math.h>
#include <limits.h>

#include <fck_hash.h>

#include <ctype.h>
#include <string.h>

#define source_text_state_transition_to(condition, state, target)                                                                          \
	case (condition):                                                                                                                      \
		(state) = (target);                                                                                                                \
		break

#define source_text_state_any_to(state, target)                                                                                            \
	default:                                                                                                                               \
		(state) = (target);                                                                                                                \
		break

#define conditional_switch for (int fckc_concat(i, __LINE__) = 1; fckc_concat(i, __LINE__) != 0; fckc_concat(i, __LINE__) = 0)

#define token_symbol_hash(c) (((c) ^ ((c) >> 3)) & 31)

#define token_hashed_symbol_case_break(num, holder, type)                                                                                  \
	case (token_symbol_hash(num)):                                                                                                         \
		(holder) = (type);                                                                                                                 \
		break

#define token_hashed_symbol_case(num, holder, type)                                                                                        \
	case (token_symbol_hash(num)):                                                                                                         \
		(holder) = (type);

#define token_symbol_assign_if(condition, result, target)                                                                                  \
	if ((condition))                                                                                                                       \
	{                                                                                                                                      \
		(result) = (target);                                                                                                               \
		break;                                                                                                                             \
	}

typedef enum source_text_state
{
	source_text_code,
	source_text_slash,
	source_text_star,
	source_text_line_comment,
	source_text_block_comment,
	source_text_string,
	source_text_string_escaped,
	source_text_char,
	source_text_char_escaped,

	source_text_ignore,
	source_text_backslash,
} source_text_state;

typedef struct token_keyword_entry
{
	const char        *raw;
	fckc_size_t        len;
	int                hash;
	token_keyword_type type;
} token_keyword_entry;
// Unsure about the size, but meh

static token_keyword_entry keyword_table[128];

static int tokeniser_up_and_running = 0;

// TODO: Export
static const char *source_text_get(const source_text *text)
{
	if (text->len <= sizeof(text->as.direct))
	{
		return text->as.direct;
	}
	return text->as.indirect;
}

static source_text source_text_from_string_length(const char *text, fckc_size_t len)
{
	source_text result;
	result.len = len;
	if (len <= sizeof(result.as.direct))
	{
		memcpy(result.as.direct, text, len);
	}
	else
	{
		result.as.indirect = text;
	}
	return result;
}

static source_text source_text_from_string_string(const char *begin, const char *end)
{
	const fckc_size_t len = end - begin;
	return source_text_from_string_length(begin, len);
}

static xtoken token_create_common(const source_text *text, token_common_type type)
{
	const xtoken result = {
		.category  = token_category_common,
		.as.common = type,
		.text      = *text,
	};
	return result;
}

static xtoken token_create_keyword(const source_text *text, token_keyword_type type)
{
	const xtoken result = {
		.category   = token_category_keyword,
		.as.keyword = type,
		.text       = *text,
	};
	return result;
}

static xtoken token_create_constant(const source_text *text, token_constant_type type)
{
	const xtoken result = {
		.category    = token_category_constant,
		.as.constant = type,
		.text        = *text,
	};
	return result;
}

static xtoken token_create_preprocessor(const source_text *text, token_preprocessor_type type)
{
	const xtoken result = {
		.category        = token_category_preprocessor,
		.as.preprocessor = type,
		.text            = *text,
	};
	return result;
}

static xtoken token_create_punctuator(const source_text *text, token_punctuator_type type)
{
	const xtoken result = {
		.category      = token_category_punctuator,
		.as.punctuator = type,
		.text          = *text,
	};
	return result;
}

static char *strip(char *current, char *eof, char front, char back, xtoken **tokens)
{
	fck_assert(front == *current);

	char *checkpoint = current + 1; // Past marker
	for (current = checkpoint; current != eof; current++)
	{
		if (*current == back)
		{
			const fckc_size_t len = current - checkpoint;

			return current;
		}
	}
	return NULL;
}

static char *included_header_name(char *current, char *eof, xtoken **tokens)
{
	for (current = current + 1; current != eof; current++)
	{
		char *end = NULL;
		switch (*current)
		{
		case '\"':
			end = strip(current, eof, '\"', '\"', tokens);
		case '\'':
			end = strip(current, eof, '\'', '\'', tokens);
		case '<':
			end = strip(current, eof, '<', '>', tokens);
		case '\n':
			return eof;
		}
		if (end)
		{
			char *begin = current + 1;

			const source_text text = source_text_from_string_string(begin, end);
			fck_dynarr_add(*tokens, token_create_preprocessor(&text, token_preprocessor_header_name));
			// Found header name!
			return end;
		}
	}
	return current;
}

static char *number(char **cursor, char *eof, xtoken **tokens)
{
	char *current = *cursor;
	if (!isdigit(*current))
	{
		return NULL;
	}
	char *checkpoint = current;

	int decimal = 0;
	for (current = current + 1; current != eof; current++)
	{
		char c = *current;
		if ((c == 'e' || c == 'E' || c == 'p' || c == 'P') && &current[1] != eof)
		{
			char next = current[1];
			if (next == '+' || next == '-')
			{
				current++;
			}
		}
		else if (!isalnum((unsigned char)c) && c != '.')
		{
			break;
		}
		decimal = decimal || c == '.';
	}

	const source_text text = source_text_from_string_string(checkpoint, current);

	const token_constant_type type = decimal ? token_constant_decimal : token_constant_integer;
	fck_dynarr_add(*tokens, token_create_constant(&text, type));
	*cursor = current - 1;
	return *cursor;
}

static char *preprocessor(char *current, char *eof, xtoken **tokens)
{
	fck_assert(*current == '#');

	for (current = current + 1; current != eof; current++)
	{
		// We except any letter after # - If not we just scan until the end, which is super duper fine lol
		char *begin = current;
		if (*begin >= 'a' && *begin <= 'z')
		{
			for (current = current + 1; current != eof; current++)
			{
				char *end = current;
				if (*end < 'a' || *end > 'z')
				{
					token_preprocessor_type type = token_preprocessor_invalid;
					const fckc_size_t       len  = end - begin;
					switch (len)
					{
					case 2:
						if (memcmp(begin, "if", len) == 0)
						{
							type = token_preprocessor_if;
						}
						break;
					case 4:
						if (memcmp(begin, "else", len) == 0)
						{
							type = token_preprocessor_else;
						}
						else if (memcmp(begin, "elif", len) == 0)
						{
							type = token_preprocessor_elif;
						}
						else if (memcmp(begin, "line", len) == 0)
						{
							type = token_preprocessor_line;
						}
						break;

					case 5:
						if (memcmp(begin, "error", len) == 0)
						{
							type = token_preprocessor_error;
						}
						else if (memcmp(begin, "ifdef", len) == 0)
						{
							type = token_preprocessor_ifdef;
						}
						else if (memcmp(begin, "endif", len) == 0)
						{
							type = token_preprocessor_endif;
						}
						else if (memcmp(begin, "undef", len) == 0)
						{
							type = token_preprocessor_undef;
						}
						else if (memcmp(begin, "embed", len) == 0)
						{
							type = token_preprocessor_embed;
						}
						break;

					case 6:
						if (memcmp(begin, "define", len) == 0)
						{
							type = token_preprocessor_define;
						}
						else if (memcmp(begin, "ifndef", len) == 0)
						{
							type = token_preprocessor_ifndef;
						}
						else if (memcmp(begin, "pragma", len) == 0)
						{
							type = token_preprocessor_pragma;
						}
						break;

					case 7:
						if (memcmp(begin, "include", len) == 0)
						{
							type = token_preprocessor_include;
						}
						// else if (memcmp(begin, "warning", len) == 0)
						//{
						//	type = token_preprocessor_warning;
						// }
						else if (memcmp(begin, "defined", len) == 0)
						{
							type = token_preprocessor_defined;
						}
						break;

					case 8:
						if (memcmp(begin, "elifdef", len) == 0)
						{
							type = token_preprocessor_elifdef;
						}
						break;

					case 9:
						if (memcmp(begin, "elifndef", len) == 0)
						{
							type = token_preprocessor_elifndef;
						}
						break;
					default:
						return eof;
					}
					if (type == token_preprocessor_invalid)
					{
						return eof;
					}

					const source_text text = source_text_from_string_length(begin, len);
					fck_dynarr_add(*tokens, token_create_preprocessor(&text, type));

					if (type == token_preprocessor_include)
					{
						current = included_header_name(current, eof, tokens);
					}
					return current;
				}
			}
		}
	}
	return eof;
}

static char *symbol(char **cursor, char *eof, xtoken **tokens)
{
	// TODO: Range check, but we can also just add a little buffer at the end of source for some convenience :)
	// stand-alone only symbols
	token_punctuator_type type     = token_punctuator_invalid;
	token_punctuator_type compound = token_punctuator_invalid;

	char *current = *cursor;
	switch (token_symbol_hash(current[0]))
	{
		{
			// single-character possibilities
			token_hashed_symbol_case_break('[', type, token_punctuator_lbracket);
			token_hashed_symbol_case_break(']', type, token_punctuator_rbracket);
			token_hashed_symbol_case_break('(', type, token_punctuator_lparen);
			token_hashed_symbol_case_break(')', type, token_punctuator_rparen);
			token_hashed_symbol_case_break('{', type, token_punctuator_lbrace);
			token_hashed_symbol_case_break('}', type, token_punctuator_rbrace);
			token_hashed_symbol_case_break('?', type, token_punctuator_question);
			token_hashed_symbol_case_break(';', type, token_punctuator_semicolon);
			token_hashed_symbol_case_break(':', type, token_punctuator_colon);
			token_hashed_symbol_case_break(',', type, token_punctuator_comma);
			token_hashed_symbol_case_break('~', type, token_punctuator_tilde);
		}

		{
			// multi-character possibilities
			token_hashed_symbol_case('.', type, token_punctuator_dot);
			token_symbol_assign_if(current[1] == '.', compound, token_punctuator_ellipsis);
			break;
			token_hashed_symbol_case('+', type, token_punctuator_plus);
			token_symbol_assign_if(current[1] == '+', compound, token_punctuator_plus_plus);
			token_symbol_assign_if(current[1] == '=', compound, token_punctuator_plus_equal);
			break;
			token_hashed_symbol_case('-', type, token_punctuator_minus);
			token_symbol_assign_if(current[1] == '-', compound, token_punctuator_minus_minus);
			token_symbol_assign_if(current[1] == '=', compound, token_punctuator_minus_equal);
			token_symbol_assign_if(current[1] == '>', compound, token_punctuator_arrow);
			break;
			token_hashed_symbol_case('%', type, token_punctuator_percent);
			token_symbol_assign_if(current[1] == '=', compound, token_punctuator_percent_equal);
			break;
			token_hashed_symbol_case('*', type, token_punctuator_star);
			token_symbol_assign_if(current[1] == '=', compound, token_punctuator_star_equal);
			break;
			token_hashed_symbol_case('/', type, token_punctuator_slash);
			token_symbol_assign_if(current[1] == '=', compound, token_punctuator_slash_equal);
			break;
			token_hashed_symbol_case('^', type, token_punctuator_caret);
			token_symbol_assign_if(current[1] == '=', compound, token_punctuator_caret_equal);
			break;
			token_hashed_symbol_case('&', type, token_punctuator_ampersand);
			token_symbol_assign_if(current[1] == '&', compound, token_punctuator_ampersand_ampersand);
			token_symbol_assign_if(current[1] == '=', compound, token_punctuator_ampersand_equal);
			break;
			token_hashed_symbol_case('|', type, token_punctuator_pipe);
			token_symbol_assign_if(current[1] == '|', compound, token_punctuator_pipe_pipe);
			token_symbol_assign_if(current[1] == '=', compound, token_punctuator_pipe_equal);
			break;
			token_hashed_symbol_case('!', type, token_punctuator_exclaim);
			token_symbol_assign_if(current[1] == '=', compound, token_punctuator_exclaim_equal);
			break;
			token_hashed_symbol_case('<', type, token_punctuator_less);
			token_symbol_assign_if(current[1] == '<', compound, token_punctuator_less_less);
			token_symbol_assign_if(current[1] == '=', compound, token_punctuator_less_equal);
			break;
			token_hashed_symbol_case('>', type, token_punctuator_greater);
			token_symbol_assign_if(current[1] == '>', compound, token_punctuator_greater_greater);
			token_symbol_assign_if(current[1] == '=', compound, token_punctuator_greater_equal);
			break;
			token_hashed_symbol_case('=', type, token_punctuator_equal);
			token_symbol_assign_if(current[1] == '=', compound, token_punctuator_equal_equal);
			break;
			token_hashed_symbol_case('#', type, token_punctuator_hash);
			token_symbol_assign_if(current[1] == '#', compound, token_punctuator_hash_hash);
			break;
		}
	}
	if (type == token_punctuator_invalid)
	{
		if (*current != '\n')
		{
			return NULL;
		}
		// '\n' does not fit well into the switch/case table mod 31
		// To compensate for that, we just defer the check for it
		// since a line delimer usually only appears once per line,
		// putting it down there should be ok! :)
		type = token_punctuator_newline;
	}

	if (compound == token_punctuator_invalid)
	{
		const source_text text = source_text_from_string_length(current, 1);
		fck_dynarr_add(*tokens, token_create_punctuator(&text, type));
		*cursor = current;
		return *cursor;
	}

	type = token_punctuator_invalid;

	switch (compound)
	{
	case token_punctuator_ellipsis: {
		token_symbol_assign_if(current[2] == '.', type, token_punctuator_ellipsis);

		const source_text text = source_text_from_string_length(current, 1);
		fck_dynarr_add(*tokens, token_create_punctuator(&text, token_punctuator_dot));
		return current;
	}
	case token_punctuator_greater_greater:
		token_symbol_assign_if(current[2] == '=', type, token_punctuator_greater_greater_equal);
		break;
	case token_punctuator_less_less:
		token_symbol_assign_if(current[2] == '=', type, token_punctuator_less_less_equal);
		break;
	}

	if (type == token_punctuator_invalid)
	{
		const source_text text = source_text_from_string_length(current, 2);
		fck_dynarr_add(*tokens, token_create_punctuator(&text, compound));
		*cursor = current + 1;
		return *cursor;
	}

	const source_text text = source_text_from_string_length(current, 3);
	fck_dynarr_add(*tokens, token_create_punctuator(&text, type));
	*cursor = current + 2;
	return *cursor;
}

static void register_keyword(const char *str, token_keyword_type type)
{
	fckc_size_t len  = strlen(str);
	fckc_u64    hash = fck_hash(str, len);
	fckc_u64    slot = hash % fck_arraysize(keyword_table);

	while (keyword_table[slot].hash != 0)
	{
		slot = (slot + 1) % fck_arraysize(keyword_table);
	}

	keyword_table[slot].raw  = str;
	keyword_table[slot].len  = len;
	keyword_table[slot].hash = hash;
	keyword_table[slot].type = type;
}

static const token_keyword_entry *find_keyword(fckc_u64 hash, const char *value, fckc_size_t len)
{
	fckc_u64 slot = hash % fck_arraysize(keyword_table);
	while (keyword_table[slot].hash != 0)
	{
		if (keyword_table[slot].hash == hash)
		{
			if (strncmp(keyword_table[slot].raw, value, len) == 0)
			{
				return &keyword_table[slot];
			}
		}
		slot = (slot + 1) % fck_arraysize(keyword_table);
	}
	return NULL;
}

void token_keywords_init(void)
{
	register_keyword("auto", token_keyword_auto);
	register_keyword("break", token_keyword_break);
	register_keyword("case", token_keyword_case);
	register_keyword("char", token_keyword_char);
	register_keyword("const", token_keyword_const);
	register_keyword("continue", token_keyword_continue);
	register_keyword("default", token_keyword_default);
	register_keyword("do", token_keyword_do);
	register_keyword("double", token_keyword_double);
	register_keyword("else", token_keyword_else);
	register_keyword("enum", token_keyword_enum);
	register_keyword("extern", token_keyword_extern);
	register_keyword("float", token_keyword_float);
	register_keyword("for", token_keyword_for);
	register_keyword("goto", token_keyword_goto);
	register_keyword("if", token_keyword_if);
	register_keyword("inline", token_keyword_inline);
	register_keyword("int", token_keyword_int);
	register_keyword("long", token_keyword_long);
	register_keyword("register", token_keyword_register);
	register_keyword("restrict", token_keyword_restrict);
	register_keyword("return", token_keyword_return);
	register_keyword("short", token_keyword_short);
	register_keyword("signed", token_keyword_signed);
	register_keyword("sizeof", token_keyword_sizeof);
	register_keyword("static", token_keyword_static);
	register_keyword("struct", token_keyword_struct);
	register_keyword("switch", token_keyword_switch);
	register_keyword("typedef", token_keyword_typedef);
	register_keyword("union", token_keyword_union);
	register_keyword("unsigned", token_keyword_unsigned);
	register_keyword("void", token_keyword_void);
	register_keyword("volatile", token_keyword_volatile);
	register_keyword("while", token_keyword_while);
	register_keyword("_Alignas", token_keyword_Alignas);
	register_keyword("_Alignof", token_keyword_Alignof);
	register_keyword("_Atomic", token_keyword_Atomic);
	register_keyword("_Bool", token_keyword_Bool);
	register_keyword("_Complex", token_keyword_Complex);
	register_keyword("_Generic", token_keyword_Generic);
	register_keyword("_Imaginary", token_keyword_Imaginary);
	register_keyword("_Noreturn", token_keyword_Noreturn);
	register_keyword("_Thread_local", token_keyword_Thread_local);
	register_keyword("_Static_assert", token_keyword_Static_assert);
}

static char *identifier_or_keyword(char **current, char *eof, xtoken **tokens)
{
	char *checkpoint = *current;
	if (!isalnum(*checkpoint) && *checkpoint != '_')
	{
		return NULL;
	}
	char *end = checkpoint + 1;
	for (; end != eof; end++)
	{
		if (!isalnum(*end) && *end != '_')
		{
			break;
		}
	}
	const fckc_size_t len = to_size_t(end - checkpoint);

	const source_text text = source_text_from_string_length(checkpoint, len);
	const int         hash = fck_hash(checkpoint, len);

	const token_keyword_entry *entry = find_keyword(hash, checkpoint, len);
	if (entry)
	{
		fck_dynarr_add(*tokens, token_create_keyword(&text, entry->type));
	}
	else
	{
		fck_dynarr_add(*tokens, token_create_common(&text, token_common_identifier));
	}
	*current = end - 1;
	return *current;
}

static xtoken *generate_tokens_from_source(char *source, char *eof)
{
	source_text_state state = source_text_code;

	xtoken *result = fck_dynarr_new(xtoken, kll->system, 512);

	char *literal = source;
	for (char *current = source; current != eof; current = current + 1)
	{
		// TODO: Fix the way we go to next current!!
		source_text_state previous = state;
		switch (state)
		{
		case source_text_ignore: {
			switch (*current)
			{
				source_text_state_any_to(state, source_text_code);
			}
		}
		case source_text_code:
			switch (*current)
			{
				source_text_state_transition_to('/', state, source_text_slash);
				source_text_state_transition_to('"', state, source_text_string);
				source_text_state_transition_to('\'', state, source_text_char);
				source_text_state_transition_to('\\', state, source_text_backslash);
			}
			break;
		case source_text_backslash:
			switch (*current)
			{
				source_text_state_transition_to('\n', state, source_text_ignore);
				// Technically a compiler error
				source_text_state_any_to(state, source_text_code);
			}
			break;
		case source_text_slash:
			switch (*current)
			{
				source_text_state_transition_to('*', state, source_text_block_comment);
				source_text_state_transition_to('/', state, source_text_line_comment);
				source_text_state_transition_to('"', state, source_text_string);
				source_text_state_transition_to('\'', state, source_text_char);
				source_text_state_any_to(state, source_text_code);
			}
			break;
		case source_text_block_comment:
			switch (*current)
			{
				source_text_state_transition_to('*', state, source_text_star);
			}
			break;
		case source_text_star:
			switch (*current)
			{
				source_text_state_transition_to('/', state, source_text_ignore);
				source_text_state_transition_to('*', state, source_text_star);
				source_text_state_any_to(state, source_text_block_comment);
			}
			break;
		case source_text_line_comment:
			switch (*current)
			{
				source_text_state_transition_to('\n', state, source_text_ignore);
			}
			break;
		case source_text_string:
			switch (*current)
			{
				source_text_state_transition_to('\\', state, source_text_string_escaped);
				source_text_state_transition_to('"', state, source_text_code);
			}
			break;
		case source_text_string_escaped:
			switch (*current)
			{
				source_text_state_any_to(state, source_text_string);
			}
			break;
		case source_text_char:
			switch (*current)
			{
				source_text_state_transition_to('\\', state, source_text_char_escaped);
				source_text_state_transition_to('\'', state, source_text_code);
			}
			break;
		case source_text_char_escaped:
			switch (*current)
			{
				source_text_state_any_to(state, source_text_char);
			}
			break;
		}

		// We can completely ignore escaped string since that becomes part of the string
		// We can completely ignore escaped char since that becomes part of the char
		// We can completely ignore star since transitions into star only happen within comments
		// Obviously we can ignore comments
		// If we transitioned away from code, we know we are not in code anymore
		conditional_switch
		{
			// Very important edge case...
			if (previous == source_text_slash && state != source_text_block_comment && state != source_text_line_comment)
			{
				// dumb edge case cause slash can be division and comment-related
				// We know previous was a slash, we know that it cannot be a comment
				// We can have other stuff after the slash - for some reason
				// We just re-simulate the symbol logic and before that we retract current
				// What a fucking mess
				current    = current - 1;
				char *next = symbol(&current, eof, &result);
				fck_assert(next && "We should know better.");
				break;
			}

			// Hot path
			if (state == source_text_code)
			{
				conditional_switch
				{
					const source_text text = source_text_from_string_string(literal, current);
					if (previous == source_text_char)
					{
						fck_dynarr_add(result, token_create_constant(&text, token_constant_char));
						break;
					}

					if (previous == source_text_string)
					{
						fck_dynarr_add(result, token_create_constant(&text, token_constant_string));
						break;
					}

					// Let's hope we do not skip anything
					switch (*current)
					{
					case '#':
						current = preprocessor(current, eof, &result);
						break;
					default:
						if (identifier_or_keyword(&current, eof, &result))
						{
							if (!number(&current, eof, &result))
							{
								if (!symbol(&current, eof, &result))
								{
									break;
									// Assert?
								}
							}
						}
						break;
					}
				}
				break;
			}


			// Usual
			if (previous == source_text_code && state == source_text_string)
			{
				// Congrats, we got a string!
				literal = current;
				break;
			}

			// Usual
			if (previous == source_text_code && state == source_text_char)
			{
				// Congrats, we got a char!
				literal = current;
				break;
			}
		}
	}
	return result;
}

// TODO: export
fckc_size_t tokens_generate(char *source, xtoken **tokens)
{
	if (!tokeniser_up_and_running)
	{
		token_keywords_init();
		tokeniser_up_and_running = 1;
	}

	const fckc_size_t len = strlen(source);
	*tokens               = generate_tokens_from_source(source, source + len);
	return fck_dynarr_count(*tokens);
}
