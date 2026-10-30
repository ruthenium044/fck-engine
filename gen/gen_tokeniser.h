#ifndef GEN_TOKENISER_H_INCLUDED
#define GEN_TOKENISER_H_INCLUDED

#include <fckc_inttypes.h>

typedef struct source_text
{
	fckc_size_t len;

	union {
		const char *indirect;
		char        direct[sizeof(void *)];
	} as;
} source_text;

typedef enum token_category
{
	token_category_invalid = 0,
	token_category_common,
	token_category_keyword, // Easy
	token_category_constant,
	token_category_preprocessor,

	token_category_punctuator, // Easy
} token_category;

typedef enum token_common_type
{
	token_common_invalid = 0,
	token_common_identifier,
} token_common_type;

typedef enum token_keyword_type
{
	token_keyword_invalid = 0,
	token_keyword_auto,
	token_keyword_break,
	token_keyword_case,
	token_keyword_char,
	token_keyword_const,
	token_keyword_continue,
	token_keyword_default,
	token_keyword_do,
	token_keyword_double,
	token_keyword_else,
	token_keyword_enum,
	token_keyword_extern,
	token_keyword_float,
	token_keyword_for,
	token_keyword_goto,
	token_keyword_if,
	token_keyword_inline,
	token_keyword_int,
	token_keyword_long,
	token_keyword_register,
	token_keyword_restrict,
	token_keyword_return,
	token_keyword_short,
	token_keyword_signed,
	token_keyword_sizeof,
	token_keyword_static,
	token_keyword_struct,
	token_keyword_switch,
	token_keyword_typedef,
	token_keyword_union,
	token_keyword_unsigned,
	token_keyword_void,
	token_keyword_volatile,
	token_keyword_while,
	token_keyword_Alignas,
	token_keyword_Alignof,
	token_keyword_Atomic,
	token_keyword_Bool,
	token_keyword_Complex,
	token_keyword_Generic,
	token_keyword_Imaginary,
	token_keyword_Noreturn,
	token_keyword_Thread_local,
	token_keyword_Static_assert,
} token_keyword_type;

typedef enum token_constant_type
{
	token_constant_invalid = 0,
	token_constant_string,
	token_constant_char,
	token_constant_integer,
	token_constant_decimal,
} token_constant_type;

typedef enum token_preprocessor_type
{
	token_preprocessor_invalid = 0,

	token_preprocessor_define,
	token_preprocessor_elif,
	token_preprocessor_else,
	token_preprocessor_endif,
	token_preprocessor_error,
	token_preprocessor_if,
	token_preprocessor_ifdef,
	token_preprocessor_ifndef,
	token_preprocessor_include,
	token_preprocessor_line,
	token_preprocessor_pragma,
	token_preprocessor_undef,

	token_preprocessor_elifdef,
	token_preprocessor_elifndef,

	token_preprocessor_hash,
	token_preprocessor_hash_hash,
	token_preprocessor_defined,
	token_preprocessor_header_name,
	token_preprocessor_va_args,
	token_preprocessor_embed,
} token_preprocessor_type;

typedef enum token_punctuator_type
{
	token_punctuator_invalid = 0,
	
	// Hot section
	token_punctuator_lbracket,
	token_punctuator_rbracket,
	token_punctuator_lparen,
	token_punctuator_rparen,
	token_punctuator_lbrace,
	token_punctuator_rbrace,

	token_punctuator_question,
	token_punctuator_semicolon,
	token_punctuator_colon,
	token_punctuator_comma,
	token_punctuator_tilde,
	token_punctuator_newline,
	// Hot

	// Different kind of hot
	token_punctuator_dot,
	token_punctuator_plus,
	token_punctuator_minus,
	token_punctuator_percent,
	token_punctuator_star,
	token_punctuator_slash,
	token_punctuator_caret,
	token_punctuator_ampersand,
	token_punctuator_pipe,
	token_punctuator_exclaim,
	token_punctuator_less,
	token_punctuator_greater,
	token_punctuator_equal,
	token_punctuator_hash,

	token_punctuator_arrow,

	token_punctuator_minus_minus,
	token_punctuator_plus_plus,

	token_punctuator_less_less,
	token_punctuator_greater_greater,

	token_punctuator_less_equal,
	token_punctuator_greater_equal,
	token_punctuator_equal_equal,
	token_punctuator_exclaim_equal,
	token_punctuator_ampersand_ampersand,
	token_punctuator_pipe_pipe,

	token_punctuator_ellipsis,
	token_punctuator_plus_equal,
	token_punctuator_minus_equal,
	token_punctuator_star_equal,
	token_punctuator_slash_equal,
	token_punctuator_percent_equal,
	token_punctuator_less_less_equal,
	token_punctuator_greater_greater_equal,
	token_punctuator_ampersand_equal,
	token_punctuator_pipe_equal,
	token_punctuator_caret_equal,

	token_punctuator_hash_hash,

	token_punctuator_digraph_lbracket,
	token_punctuator_digraph_rbracket,
	token_punctuator_digraph_lbrace,
	token_punctuator_digraph_rbrace,
	token_punctuator_digraph_hash,
	token_punctuator_digraph_hash_hash,
	// Maximum is 64, not enforced...
} token_punctuator_type;

typedef struct xtoken
{
	source_text text;

	token_category category;

	union {
		token_common_type       common;
		token_keyword_type      keyword;
		token_constant_type     constant;
		token_punctuator_type   punctuator;
		token_preprocessor_type preprocessor;
	} as;

} xtoken;

fckc_size_t tokens_generate(char *source, xtoken **tokens);

// TODO: Do very distinct steps! :)
// - Tokenising (1:1 - Straightforward) - >works on tokens
// - Preprocess -> element
// - Syntax -> works on nodes

#endif // !GEN_TOKENISER_H_INCLUDED
