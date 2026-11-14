
#include "gen_tokeniser.h"

#include <kll.h>
#include <kll_malloc.h>

#include <fck_dynarr.h>
#include <fck_set.h>

#include <fckc_math.h>
#include <limits.h>

#include <fck_hash.h>

#include <stdio.h>
#include <string.h>

#include <fckc_bits.h>
#include <fckc_simd.h>

#include <kll.h>
#include <kll_malloc.h>

#include <fck_os.h>

typedef enum chartype
{
	chartype_none  = 0,
	chartype_space = 1 << 0,                          // 9-13 | 32
	chartype_digit = 1 << 1,                          // 48–57
	chartype_alpha = 1 << 2,                          // 65-90 | 97-122
	chartype_alnum = chartype_digit | chartype_alpha, // 48–57 | 65-90 | 97-122
	chartype_punct = 1 << 4,                          // 33-47 | 58-64 | 91-96 | 123–126
} chartype;

typedef enum tokenise_state_type
{
	tokenise_state_none = 0,
	tokenise_state_identifier_or_keyword,
	tokenise_state_integer_or_decimal_number,
	tokenise_state_symbol,
} tokenise_state_type;

static fckc_i128 categorise_chars(fckc_i128 batch)
{
	// Maybe use char constants instead. Oh well
	const fckc_i128 space_exact = fck_simd_i128_set1_i8(32);
	const fckc_i128 space_min   = fck_simd_i128_set1_i8(8);
	const fckc_i128 space_max   = fck_simd_i128_set1_i8(14);

	const fckc_i128 digit_min = fck_simd_i128_set1_i8(47);
	const fckc_i128 digit_max = fck_simd_i128_set1_i8(58);

	const fckc_i128 alpha_min = fck_simd_i128_set1_i8(96);
	const fckc_i128 alpha_max = fck_simd_i128_set1_i8(123);

	const fckc_i128 mask_space = fck_simd_i128_set1_i8(chartype_space);
	const fckc_i128 mask_digit = fck_simd_i128_set1_i8(chartype_digit);
	const fckc_i128 mask_alpha = fck_simd_i128_set1_i8(chartype_alpha);
	const fckc_i128 mask_punct = fck_simd_i128_set1_i8(chartype_punct);

	const fckc_i128 is_space_char = fck_simd_i128_cmpeq_i8(batch, space_exact);
	const fckc_i128 is_space_ish  = fck_simd_i128_and(fck_simd_i128_cmpgt_i8(batch, space_min), fck_simd_i128_cmplt_i8(batch, space_max));
	const fckc_i128 is_space      = fck_simd_i128_or(is_space_char, is_space_ish);

	const fckc_i128 is_digit = fck_simd_i128_and(fck_simd_i128_cmpgt_i8(batch, digit_min), fck_simd_i128_cmplt_i8(batch, digit_max));

	const fckc_i128 as_lower = fck_simd_i128_or(batch, fck_simd_i128_set1_i8(0x20));
	const fckc_i128 is_alpha = fck_simd_i128_and(fck_simd_i128_cmpgt_i8(as_lower, alpha_min), fck_simd_i128_cmplt_i8(as_lower, alpha_max));

	const fckc_i128 is_greater_space = fck_simd_i128_cmpgt_i8(batch, space_exact);

	const fckc_i128 is_alnum = fck_simd_i128_or(is_digit, is_alpha);
	// This logic is WRONG
	const fckc_i128 is_punct = fck_simd_i128_andnot(is_greater_space, is_alnum);

	fckc_i128 result = fck_simd_i128_zero();
	result           = fck_simd_i128_or(result, fck_simd_i128_and(is_space, mask_space));
	result           = fck_simd_i128_or(result, fck_simd_i128_and(is_digit, mask_digit));
	result           = fck_simd_i128_or(result, fck_simd_i128_and(is_alpha, mask_alpha));
	result           = fck_simd_i128_or(result, fck_simd_i128_and(is_punct, mask_punct));

	return result;
}

static chartype isspace(char c)
{
	switch (c)
	{
	case '\t':
	case '\n':
	case '\v':
	case '\f':
	case '\r':
	case ' ':
		return chartype_space;
	}
	return chartype_none;
}

static chartype isalpha(char c)
{
	switch (c)
	{
	case 'A':
	case 'B':
	case 'C':
	case 'D':
	case 'E':
	case 'F':
	case 'G':
	case 'H':
	case 'I':
	case 'J':
	case 'K':
	case 'L':
	case 'M':
	case 'N':
	case 'O':
	case 'P':
	case 'Q':
	case 'R':
	case 'S':
	case 'T':
	case 'U':
	case 'V':
	case 'W':
	case 'X':
	case 'Y':
	case 'Z':
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
	case 'g':
	case 'h':
	case 'i':
	case 'j':
	case 'k':
	case 'l':
	case 'm':
	case 'n':
	case 'o':
	case 'p':
	case 'q':
	case 'r':
	case 's':
	case 't':
	case 'u':
	case 'v':
	case 'w':
	case 'x':
	case 'y':
	case 'z':
		return chartype_alpha;
	}
	return chartype_none;
}

static chartype isdigit(char c)
{
	switch (c)
	{
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		return chartype_digit;
	}
	return chartype_none;
}

static chartype isalnum(char c)
{
	return isalpha(c) | isdigit(c);
}

// Renaming these function macro defines would be golden
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
			break;
		case '\'':
			end = strip(current, eof, '\'', '\'', tokens);
			break;
		case '<':
			end = strip(current, eof, '<', '>', tokens);
			break;
		case '\n':
			return eof;
		}
		if (end)
		{
			char *begin = current + 1;

			const source_text text = source_text_from_string_string(begin, end);
			fck_dynarr_add(*tokens, token_create_preprocessor(&text, token_preprocessor_header_name));
			// Found header name!
			return end + 1;
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

static char *preprocessor_directive(char *begin, char *end, xtoken **tokens)
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
		return NULL;
	}
	if (type == token_preprocessor_invalid)
	{
		return NULL;
	}

	const source_text text = source_text_from_string_length(begin, len);
	fck_dynarr_add(*tokens, token_create_preprocessor(&text, type));

	return end;
}

static char *preprocessor(char **cursor, char *eof, xtoken **tokens)
{
	char *current = *cursor;
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
						return NULL;
					}
					if (type == token_preprocessor_invalid)
					{
						return NULL;
					}

					const source_text text = source_text_from_string_length(begin, len);
					fck_dynarr_add(*tokens, token_create_preprocessor(&text, type));

					if (type == token_preprocessor_include)
					{
						current = included_header_name(current, eof, tokens);
					}
					*cursor = current - 1;
					return *cursor;
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
	if (*current == '\n')
	{
		// Edge case for now... Just for now. I think I can do something better later
		const source_text text = source_text_from_string_length(current, 1);
		fck_dynarr_add(*tokens, token_create_punctuator(&text, token_punctuator_newline));
		*cursor = current;
		return *cursor;
	}

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
		return NULL;
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
			const token_keyword_entry *item = keyword_table + slot;
			if (item->len == len && memcmp(item->raw, value, len) == 0)
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
	if (!isalpha(*checkpoint) && *checkpoint != '_')
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
	const fckc_u64    hash = fck_hash(checkpoint, len);

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

static source_text_state source_text_state_update(char *current, source_text_state state)
{
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
		case ('\''):
			(state) = (source_text_code);
			break;
		}
		break;
	case source_text_char_escaped:
		switch (*current)
		{
		default:
			(state) = (source_text_char);
			break;
		}
		break;
	}
	return state;
}

typedef struct chartype_debug
{
	fckc_u8 values[16];
} chartype_debug;

static char *preprocessor_or_symbol(char *current, char *eof, xtoken **tokens)
{
	for (char *at = current; at < eof; at++)
	{
		char *begin = at;
		if (!symbol(&at, eof, tokens))
		{
			// Uuhh...
			return NULL;
		}
	}
	return eof;
}

static xtoken *generate_tokens_simd(char *source, char *eof)
{
	xtoken *result = fck_dynarr_new(xtoken, kll->system, 512);

	const fckc_i128 mask_space = fck_simd_i128_set1_i8(chartype_space);
	const fckc_i128 mask_digit = fck_simd_i128_set1_i8(chartype_digit);
	const fckc_i128 mask_alpha = fck_simd_i128_set1_i8(chartype_alpha);
	const fckc_i128 mask_punct = fck_simd_i128_set1_i8(chartype_punct);

	const fckc_i128 space_mask            = fck_simd_i128_set1_i8(' ');
	const fckc_i128 slash_mask            = fck_simd_i128_set1_i8('/');
	const fckc_i128 return_mask           = fck_simd_i128_set1_i8('\r');
	const fckc_i128 newline_mask          = fck_simd_i128_set1_i8('\n');
	const fckc_i128 double_quotation_mask = fck_simd_i128_set1_i8('"');
	const fckc_i128 single_qutation_mask  = fck_simd_i128_set1_i8('\'');
	const fckc_i128 star_mask             = fck_simd_i128_set1_i8('*');
	const fckc_i128 backslash_mask        = fck_simd_i128_set1_i8('\\');
	const fckc_i128 hash_mask             = fck_simd_i128_set1_i8('#');
	const fckc_i128 underscore_mask       = fck_simd_i128_set1_i8('_');
	const fckc_i128 dot_mask              = fck_simd_i128_set1_i8('.');

	struct
	{
		fckc_u8 backlash         : 1;
		fckc_u8 single_quotation : 1;
		fckc_u8 double_quotation : 1;
		fckc_u8 line_comment     : 1;
		fckc_u8 block_comment    : 1;

		fckc_u8 identifier : 1;
		fckc_u8 digit      : 1;
		fckc_u8 punctuator : 1;
	} carry = {0};

	// struct
	//{
	//	fckc_u8 preprocessor : 1;
	//	fckc_u8 unused       : 7;
	// } setting = {0};

	char *directive   = NULL;
	char *accumulator = NULL;

	const fckc_i128 mask_constant[2] = {
		fck_simd_i128_zero(),
		fck_simd_i128_ones(),
	};

	const fckc_i128 carry_constant[] = {
		fck_simd_i128_zero(),
		fck_simd_i128_set_i8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF),
	};

	for (char *current = source; current < eof; current = current + 16)
	{

		fckc_i128 batch = fck_simd_i128_loadu(current);
		fckc_i128 next  = fck_simd_i128_loadu(current + 1);

		const fckc_i128 backslashes = fck_simd_i128_cmpeq_i8(batch, backslash_mask);

		fckc_i128 newlines[] = {
			fck_simd_i128_cmpeq_i8(batch, newline_mask),
			fck_simd_i128_cmpeq_i8(next, newline_mask),
		};

		{
			const fckc_i128 returns           = fck_simd_i128_cmpeq_i8(next, return_mask);
			const fckc_i128 backslash_newline = fck_simd_i128_and(backslashes, newlines[1]);
			fckc_i128       backslash_returns = fck_simd_i128_and(backslashes, returns);
			fckc_i128       backslash_crlf    = fck_simd_i128_and(fck_simd_i128_bsll(backslash_returns, 1), newlines[1]);

			fckc_i128 ignored_lf   = fck_simd_i128_bsll(backslash_newline, 1);
			fckc_i128 ignored_crlf = fck_simd_i128_bsll(backslash_crlf, 1);

			fckc_i128 carry_lf   = carry_constant[carry.backlash];
			fckc_i128 carry_crlf = fck_simd_i128_bsll(fck_simd_i128_and(carry_lf, fck_simd_i128_cmpeq_i8(batch, return_mask)), 1);

			fckc_i128 ignored = fck_simd_i128_or(fck_simd_i128_or(ignored_lf, ignored_crlf), fck_simd_i128_or(carry_lf, carry_crlf));

			newlines[0] = fck_simd_i128_andnot(newlines[0], ignored);

			const fckc_i128 active_escapes = fck_simd_i128_or(backslashes, fck_simd_i128_bsll(backslash_returns, 1));

			// When carrying '\\''\r', we treat it as '\\'
			carry.backlash = (fck_simd_i128_movemask_i8(active_escapes) >> 15) & 1;
		}

		fckc_i128 literals;
		{
			fckc_i128 single_quotes[] = {
				fck_simd_i128_cmpeq_i8(batch, single_qutation_mask),
				fck_simd_i128_cmpeq_i8(next, single_qutation_mask),
			};

			fckc_i128 double_quotes[] = {
				fck_simd_i128_cmpeq_i8(batch, double_quotation_mask),
				fck_simd_i128_cmpeq_i8(next, double_quotation_mask),
			};

			const fckc_i128 escaped = carry_constant[carry.backlash];

			// ignore cases due to escaping -> These newlines need to get semantically skipped!
			fckc_i128 escaped_single = fck_simd_i128_and(backslashes, single_quotes[1]);
			escaped_single           = fck_simd_i128_or(escaped, fck_simd_i128_bsll(escaped_single, 1));
			single_quotes[0]         = fck_simd_i128_andnot(single_quotes[0], escaped_single);

			fckc_i128 escaped_double = fck_simd_i128_and(backslashes, double_quotes[1]);
			escaped_double           = fck_simd_i128_or(escaped, fck_simd_i128_bsll(escaped_double, 1));
			double_quotes[0]         = fck_simd_i128_andnot(double_quotes[0], escaped_double);

			fckc_i128 string_literals = simd_prefix_xor(double_quotes[0]);
			string_literals           = fck_simd_i128_xor(string_literals, mask_constant[carry.double_quotation]);

			const fckc_i128 valid_single_quotes = fck_simd_i128_andnot(single_quotes[0], string_literals);

			fckc_i128 char_literals = simd_prefix_xor(valid_single_quotes);
			char_literals           = fck_simd_i128_xor(char_literals, mask_constant[carry.single_quotation]);

			literals = fck_simd_i128_or(string_literals, char_literals);
			literals = fck_simd_i128_or(literals, fck_simd_i128_bsll(literals, 1));
			literals = fck_simd_i128_or(literals, carry_constant[carry.double_quotation]);

			carry.double_quotation = (fck_simd_i128_movemask_i8(string_literals) >> 15) & 1;
			carry.single_quotation = (fck_simd_i128_movemask_i8(char_literals) >> 15) & 1;
		}

		fckc_i128 block_comments;
		fckc_i128 line_comments;
		{
			fckc_i128 slashes[] = {
				fck_simd_i128_cmpeq_i8(batch, slash_mask),
				fck_simd_i128_cmpeq_i8(next, slash_mask),
			};
			fckc_i128 stars[] = {
				fck_simd_i128_cmpeq_i8(batch, star_mask),
				fck_simd_i128_cmpeq_i8(next, star_mask),
			};

			// Mask away comments within literals
			slashes[0] = fck_simd_i128_andnot(slashes[0], literals);

			fckc_i128 block_comment_start = fck_simd_i128_and(slashes[0], stars[1]);
			block_comment_start           = fck_simd_i128_or(block_comment_start, carry_constant[carry.block_comment]);

			fckc_i128 line_comment_start = fck_simd_i128_and(slashes[0], slashes[1]);
			line_comment_start           = fck_simd_i128_or(line_comment_start, carry_constant[carry.line_comment]);

			const fckc_i128 block_comment_end = fck_simd_i128_and(stars[0], slashes[1]);
			const fckc_i128 line_comment_end  = newlines[0];

			block_comments = simd_prefix_set_reset(block_comment_start, block_comment_end);
			line_comments  = simd_prefix_set_reset(line_comment_start, line_comment_end);

			carry.block_comment = (fck_simd_i128_movemask_i8(block_comments) >> 15) & 1;
			carry.line_comment  = (fck_simd_i128_movemask_i8(line_comments) >> 15) & 1;

			block_comments = fck_simd_i128_or(block_comments, fck_simd_i128_bsll(block_comments, 2)); // ends with */
			block_comments = fck_simd_i128_or(block_comments, carry_constant[carry.block_comment]);

			line_comments = fck_simd_i128_or(line_comments, fck_simd_i128_bsll(line_comments, 1)); // ends with /n
			line_comments = fck_simd_i128_or(line_comments, carry_constant[carry.line_comment]);
		}

		const fckc_i128 non_code =
			fck_simd_i128_or(backslashes, fck_simd_i128_or(literals, fck_simd_i128_or(block_comments, line_comments)));

		batch = fck_simd_i128_select(non_code, space_mask, batch);

		fckc_u8         types[16];
		const fckc_i128 chars = categorise_chars(batch);
		fck_simd_i128_storeu(types, chars);

		const int alphas = fck_simd_i128_movemask_i8(
			fck_simd_i128_cmpgt_i8(fck_simd_i128_and(chars, fck_simd_i128_set1_i8(chartype_alpha)), mask_constant[0]));

		const int digits = fck_simd_i128_movemask_i8(
			fck_simd_i128_cmpgt_i8(fck_simd_i128_and(chars, fck_simd_i128_set1_i8(chartype_digit)), mask_constant[0]));

		const int puncts = fck_simd_i128_movemask_i8(
			fck_simd_i128_cmpgt_i8(fck_simd_i128_and(chars, fck_simd_i128_set1_i8(chartype_punct)), mask_constant[0]));

		const int spaces = fck_simd_i128_movemask_i8(
			fck_simd_i128_cmpgt_i8(fck_simd_i128_and(chars, fck_simd_i128_set1_i8(chartype_space)), mask_constant[0]));

		const int underscores = fck_simd_i128_movemask_i8(fck_simd_i128_cmpeq_i8(batch, underscore_mask));
		const int dots        = fck_simd_i128_movemask_i8(fck_simd_i128_cmpeq_i8(batch, dot_mask));

		const int alnums = alphas | digits;

		int mask = alphas | digits | puncts;
		while (mask)
		{
			const int index = fck_lszcnt_u32(mask);

			// Forced to get processed if accumulator exists!
			char *raw = accumulator ? accumulator : current + index;

			const int bit = 1 << index;

			// This "conditional switch" just allows us to break out of if-statements into normal flow
			tokenise_state_type state = tokenise_state_none;
			conditional_switch
			{
				if (accumulator)
				{
					accumulator = NULL;
					if (carry.identifier)
					{
						state = tokenise_state_identifier_or_keyword;
						break;
					}
					if (carry.digit)
					{
						state = tokenise_state_integer_or_decimal_number;
						break;
					}
					if (carry.punctuator)
					{
						state = tokenise_state_symbol;
						break;
					}
					// TODO: Assert, unreachable!
					break;
				}
				else
				{
					if ((alphas & bit) | (underscores & bit))
					{
						state = tokenise_state_identifier_or_keyword;
						break;
					}
					if (digits & bit)
					{
						state = tokenise_state_integer_or_decimal_number;
						break;
					}
					if (puncts & bit)
					{
						state = tokenise_state_symbol;
						break;
					}
					if (spaces & bit)
					{
						// Hm. Idk. Maybe for newline and such later
						state = tokenise_state_none;
						break;
					}
				}
			}

			switch (state)
			{
			case tokenise_state_identifier_or_keyword: {
				const int identifiers = underscores | alphas | digits;
				const int smear       = ((bit - 1) & (carry.identifier - 1));
				const int rest        = (identifiers | smear) & 0xFFFF;

				carry.identifier = rest == 0xFFFF;
				if (carry.identifier)
				{
					break;
				}

				const int consumed = fck_lsocnt_u32(rest);

				char             *end = current + consumed;
				const fckc_size_t len = to_size_t(end) - to_size_t(raw);
				if (directive)
				{
					directive = NULL;
					if (!preprocessor_directive(raw, end, &result))
					{
						return NULL;
					}
				}

				const source_text          text  = source_text_from_string_length(raw, len);
				const fckc_u64             hash  = fck_hash(raw, len);
				const token_keyword_entry *entry = find_keyword(hash, raw, len);
				// if we received #, run [raw, end] through preprocessor and be done with it!
				if (entry)
				{
					fck_dynarr_add(result, token_create_keyword(&text, entry->type));
				}
				else
				{
					fck_dynarr_add(result, token_create_common(&text, token_common_identifier));
				}
				mask = mask & (~0U << consumed);
				break;
			}
			case tokenise_state_integer_or_decimal_number: {
				// TODO: Proper decimal handling, just not now!
				// We have to reach the preprocessor and decimals are illegal in there anyway
				const int digits = alnums | dots;
				const int cutoff = bit - 1;
				const int rest   = (digits | cutoff) & 0xFFFF;

				carry.digit = rest == 0xFFFF;
				if (carry.digit)
				{
					break;
				}
				const int decimal  = (dots | cutoff) & 0xFFFF;
				const int consumed = fck_lsocnt_u32(rest);

				const char       *end = current + consumed;
				const fckc_size_t len = to_size_t(end) - to_size_t(raw);

				const source_text text = source_text_from_string_length(raw, len);
				const fckc_u64    hash = fck_hash(raw, len);

				const token_constant_type type = decimal ? token_constant_decimal : token_constant_integer;
				fck_dynarr_add(result, token_create_constant(&text, type));
				mask = mask & (~0U << consumed);
				break;
			}
			case tokenise_state_symbol: {
				const int smear = ((bit - 1) & (carry.punctuator - 1));
				const int rest  = (puncts | smear) & 0xFFFF;

				carry.punctuator = rest == 0xFFFF;
				if (carry.punctuator)
				{
					break;
				}

				const int consumed = fck_lsocnt_u32(rest);
				char     *end      = current + consumed;
				for (char *at = raw; at < end; at++)
				{
					char *begin = at;
					if (!symbol(&at, eof, &result))
					{
						// Uuhh...
						break;
					}
				}

				if (end[-1] == '#')
				{
					directive = end - 1;
				}

				mask = mask & (~0U << consumed);
				break;
			}
			default: {
				mask = mask & (mask - 1);
				break;
			}
			}

			if (carry.identifier || carry.digit || carry.punctuator)
			{
				// Gets carried over through raw
				accumulator = raw;
				// we are completely done here
				break;
			}
		}
	}
	return result;
}

static xtoken *generate_tokens_from_source(char *source, char *eof)
{
	source_text_state state = source_text_code;

	xtoken *result = fck_dynarr_new(xtoken, kll->system, 512);

	char *literal = source;
	for (char *current = source; current != eof; current = current + 1)
	{
		source_text_state previous = state;

		state = source_text_state_update(current, state);

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
				char *prev = current - 1;
				char *next = symbol(&prev, eof, &result);
				fck_assert(next && "We should know better.");
				// Umm... we leave current alone sine we only re-process piece of shit /
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

					if (*current == '#')
					{
						if (!preprocessor(&current, eof, &result))
						{
							const source_text text = source_text_from_string_length(current, 1);
							fck_dynarr_add(result, token_create_punctuator(&text, token_punctuator_hash));
						}
						break;
					}

					if (!identifier_or_keyword(&current, eof, &result))
					{
						if (!number(&current, eof, &result))
						{
							chartype ct = isspace(*current);
							if (ct)
							{
								if (*current == '\n')
								{
									const source_text text = source_text_from_string_length(current, 1);
									fck_dynarr_add(result, token_create_punctuator(&text, token_punctuator_newline));
								}
								break;
							}
							if (!symbol(&current, eof, &result))
							{
								break;
							}
						}
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
void tokens_print(xtoken *tokens, fckc_size_t count)
{
	for (fckc_size_t index = 0; index < count; index++)
	{
		const xtoken *tok = tokens + index;
		const char   *raw = source_text_get(&tok->text);
		if (tok->category == token_category_punctuator)
		{
			if (tok->as.punctuator == token_punctuator_newline)
			{
				printf("\n");
				continue;
			}
			if (tok->as.punctuator == token_punctuator_semicolon)
			{
				printf("\n");
				continue;
			}
		}
		printf("%.*s ", to_int(tok->text.len), raw);
	}
}

char *text_buffer_create(struct kll_allocator *allocator, fckc_size_t len)
{
	// For the sake of vectorisation
	const fckc_size_t preferred = fckc_align(len + 1, 16);

	// TODO: we may have to do extra alignment :)
	char *buffer = (char *)kll_malloc(kll->system, preferred);
	memset(buffer + len, 0, to_size_t(preferred - len));

	return buffer;
}

typedef struct token_chunk_carry
{
	fckc_u8 backlash : 1;
	// fckc_u8 single_quotation : 1;
	// fckc_u8 double_quotation : 1;
	// fckc_u8 line_comment : 1;
	// fckc_u8 block_comment : 1;
	// fckc_u8 identifier : 1;
	// fckc_u8 digit : 1;
	// fckc_u8 punctuator : 1;
} token_chunk_carry;

xtoken *generate_tokens_simd_v3(char *source, char *eof)
{
	// TODO: \t -> ' '
	const fckc_i128 tabs    = fck_simd_i128_set1_i8('\t');
	const fckc_i128 newline = fck_simd_i128_set1_i8('\n');

	const fckc_i128 single_quote = fck_simd_i128_set1_i8('\'');
	const fckc_i128 double_quote = fck_simd_i128_set1_i8('"');

	const fckc_i128 carriage_return = fck_simd_i128_set1_i8('\r');

	const fckc_i128 backslash = fck_simd_i128_set1_i8('\\');

	const fckc_i128 space     = fck_simd_i128_set1_i8(' ');
	const fckc_i128 space_min = fck_simd_i128_set1_i8('\t' - 1);
	const fckc_i128 space_max = fck_simd_i128_set1_i8('\r' + 1);

	const fckc_i128 digit_min = fck_simd_i128_set1_i8('0' - 1);
	const fckc_i128 digit_max = fck_simd_i128_set1_i8('9' + 1);

	const fckc_i128 lower_min = fck_simd_i128_set1_i8('a' - 1);
	const fckc_i128 lower_max = fck_simd_i128_set1_i8('z' + 1);

	// x | ('a' - 'A') -> everything becomes 'a'
	const fckc_i128 to_lower = fck_simd_i128_set1_i8('a' - 'A');

	// x >= ' ' -> graph | max implicit (we ignore backspace)
	const fckc_i128 graph_min = fck_simd_i128_set1_i8(' ');

	struct
	{
		fckc_u8 backslash    : 1;
		fckc_u8 single_quote : 1;
		fckc_u8 double_quote : 1;
		// fckc_u8 line_comment : 1;
		// fckc_u8 block_comment : 1;
		// fckc_u8 identifier : 1;
		// fckc_u8 digit : 1;
		// fckc_u8 punctuator : 1;
	} carry = {0};

	const fckc_i128 mask_constant[] = {
		fck_simd_i128_zero(),
		fck_simd_i128_ones(),
	};

	const fckc_i128 carry_constant[] = {
		fck_simd_i128_zero(),
		fck_simd_i128_set_i8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF),
	};

	// char(127)			-> ignored
	// [char(0); char(8)]	-> ignored
	// [char(14); char(31)]	-> ignored

	xtoken *result = fck_dynarr_new(xtoken, kll->system, 512);

	char *checkpoint = NULL;
	for (char *cursor = source; cursor < eof; cursor = cursor + 64)
	{
		const fckc_i128 original[] = {
			fck_simd_i128_load(cursor + 0),
			fck_simd_i128_loadu(cursor + 1),
		};

		const fckc_i128 escaped_backslash = carry_constant[carry.backslash];
		const fckc_i128 backslashes       = fck_simd_i128_cmpeq_i8(original[0], backslash);

		fckc_i128 newlines[] = {
			fck_simd_i128_cmpeq_i8(original[0], newline),
			fck_simd_i128_cmpeq_i8(original[1], newline),
		};

		{
			// Handle line merging via escaping
			const fckc_i128 carriage_returns[] = {
				fck_simd_i128_cmpeq_i8(original[0], carriage_return),
				fck_simd_i128_cmpeq_i8(original[1], carriage_return),
			};

			// Thanks Windows for CRLF - Stupid
			// '\\' '\n' -> 0xFF & 0xFF -> 0xFF
			const fckc_i128 backslash_newline = fck_simd_i128_and(backslashes, newlines[1]);
			// '\\' '\r' -> 0xFF & 0xFF -> 0xFF
			const fckc_i128 backslash_returns = fck_simd_i128_and(backslashes, carriage_returns[1]);
			// '\\' '\r' '\n' ->  0xFF & 0xFF & 0xFF -> 0xFF
			const fckc_i128 backslash_crlf    = fck_simd_i128_and(fck_simd_i128_bsll(backslash_returns, 1), newlines[1]);

			// Escaped
			const fckc_i128 ignored_lf           = fck_simd_i128_bsll(backslash_newline, 1);
			const fckc_i128 ignored_crlf         = fck_simd_i128_bsll(backslash_crlf, 1);
			const fckc_i128 ignored_line_endings = fck_simd_i128_or(ignored_lf, ignored_crlf);

			// Sadly, we can have '\\' in a previous chunk, so let's mix that shit in
			// if carried -> 0x00 0x00 ... 0xFF
			const fckc_i128 carry_ignored_lf           = escaped_backslash;
			// Also consider it for CLRF, thanks again, Windows
			const fckc_i128 carry_ignored_crlf         = fck_simd_i128_bsll(fck_simd_i128_and(carry_ignored_lf, carriage_returns[0]), 1);
			// Will effectivaly, and conditionally, adjust the first byte of chunk cause carrying
			const fckc_i128 carry_ignored_line_endings = fck_simd_i128_or(carry_ignored_lf, carry_ignored_crlf);
			const fckc_i128 all_ignored_line_endings   = fck_simd_i128_or(ignored_line_endings, carry_ignored_line_endings);

			// When carrying '\\''\r', we treat it as '\\'; '\r' should be quite useless to us...
			// The downside is that we handle carry.backslash in this section
			const fckc_i128 active_escapes = fck_simd_i128_or(backslashes, fck_simd_i128_bsll(backslash_returns, 1));
			carry.backslash                = (fck_simd_i128_movemask_i8(active_escapes) >> 15) & 1;

			// Apply escaped CL or CRLF (Thanks for nothing, Windows)
			newlines[0] = fck_simd_i128_andnot(newlines[0], all_ignored_line_endings);
		}

		{
			// Handle string and char literals
			fckc_i128 single_quotes[] = {
				fck_simd_i128_cmpeq_i8(original[0], single_quote),
				fck_simd_i128_cmpeq_i8(original[1], single_quote),
			};

			fckc_i128 double_quotes[] = {
				fck_simd_i128_cmpeq_i8(original[0], double_quote),
				fck_simd_i128_cmpeq_i8(original[1], double_quote),
			};

			// Mask away escaped single quotes
			fckc_i128 escaped_single = fck_simd_i128_and(backslashes, single_quotes[1]);
			escaped_single           = fck_simd_i128_or(escaped_backslash, fck_simd_i128_bsll(escaped_single, 1));
			// Combine masked away single quotes backed away into all single quotes
			single_quotes[0]         = fck_simd_i128_andnot(single_quotes[0], escaped_single);

			// Do literally the same as with single quotes for double quotes
			fckc_i128 escaped_double = fck_simd_i128_and(backslashes, double_quotes[1]);
			escaped_double           = fck_simd_i128_or(escaped_backslash, fck_simd_i128_bsll(escaped_double, 1));
			double_quotes[0]         = fck_simd_i128_andnot(double_quotes[0], escaped_double);

			// Prefix XOR from " until the next ", if something got escaped, look above ;)
			fckc_i128 string_literals = simd_prefix_xor(double_quotes[0]);
			// Clever carry-over thing. If we had a carry over, we jsut XOR the mask which pretty much just flips the start
			string_literals           = fck_simd_i128_xor(string_literals, mask_constant[carry.double_quote]);

			// Mask away ALL single quotes within string literals - This can happen
			const fckc_i128 valid_single_quotes = fck_simd_i128_andnot(single_quotes[0], string_literals);

			// Same	shit as with the string literals 
			fckc_i128 char_literals = simd_prefix_xor(valid_single_quotes);
			char_literals           = fck_simd_i128_xor(char_literals, mask_constant[carry.single_quote]);

			carry.double_quote = (fck_simd_i128_movemask_i8(string_literals) >> 15) & 1;
			carry.single_quote = (fck_simd_i128_movemask_i8(char_literals) >> 15) & 1;

			// TODO: After processing everything, we MIGHT want to include the end quotes OR exclude the beginning quotes. Idk
			/*fckc_i128 literals = fck_simd_i128_or(string_literals, char_literals);
			literals           = fck_simd_i128_or(literals, fck_simd_i128_bsll(literals, 1));
			literals           = fck_simd_i128_or(literals, carry_constant[carry.double_quote]);*/
		}

		// movemask combine
		int mask = 0;
		while (mask)
		{
			const int index = fck_lszcnt_u32(mask);
			mask            = mask & (mask - 1);
		}
	}
}

fckc_size_t tokens_generate(char *source, xtoken **tokens)
{
	if (!tokeniser_up_and_running)
	{
		token_keywords_init();
		tokeniser_up_and_running = 1;
	}

	const fckc_size_t len = strlen(source);

	*tokens = generate_tokens_simd(source, source + len);
	tokens_print(*tokens, fck_dynarr_count(*tokens));

	*tokens = generate_tokens_from_source(source, source + len);
	return fck_dynarr_count(*tokens);
}
