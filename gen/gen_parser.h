#ifndef GEN_PARSER_H_INCLUDED
#define GEN_PARSER_H_INCLUDED

typedef enum syntax_kind
{
	syntax_invalid = 0,
	syntax_translation_unit,
	syntax_declaration,
	syntax_statement,
	syntax_expression,
	syntax_initializer,
} syntax_kind;

typedef enum declaration_kind
{
	declaration_variable,
	declaration_function,
	declaration_parameter,
	declaration_struct,
	declaration_union,
	declaration_enum,
	declaration_enumerator,
	declaration_field,
	declaration_typedef,
} declaration_kind;

typedef enum statement_kind
{
	statement_compound,   // { ... }
	statement_expression, // x = 5; or foo();
	statement_null,       // ;

	statement_if,
	statement_switch,
	statement_case,
	statement_default,

	statement_while,
	statement_do_while,
	statement_for,

	statement_goto,
	statement_label,
	statement_break,
	statement_continue,
	statement_return,
} statement_kind;

typedef enum expression_kind
{
	expression_literal,
	expression_identifier,

	expression_unary,
	expression_binary,
	expression_assignment,
	expression_ternary,
	expression_comma,

	expression_call,
	expression_subscript,
	expression_member_dot,
	expression_member_arrow,

	expression_cast,
	expression_sizeof,
	expression_compound_literal,
} expression_kind;

typedef enum initializer_kind
{
	initializer_list,             // { 1, 2, 3 }
	initializer_designator_field, // .x = ...
	initializer_designator_index, // [0] = ...
} initializer_kind;


#endif // !GEN_PARSER_H_INCLUDED