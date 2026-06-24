#include "reflection.h"

#include <fckc_inttypes.h>
#include <kll.h>
#include <kll_format.h>
#include <kll_malloc.h>

#include <fck_hash.h>

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum fck_glsl_reflection_token
{
	fck_glsl_reflection_identifier = 0,
	fck_glsl_reflection_brace_open,
	fck_glsl_reflection_brace_close,
	fck_glsl_reflection_bracket_open,
	fck_glsl_reflection_bracket_close,
	fck_glsl_reflection_paren_open,
	fck_glsl_reflection_paren_close,
	fck_glsl_reflection_struct,
	fck_glsl_reflection_layout,
	fck_glsl_reflection_uniform,
	fck_glsl_reflection_readonly,
	fck_glsl_reflection_shared,
	fck_glsl_reflection_buffer,
	fck_glsl_reflection_in,
	fck_glsl_reflection_out,
	fck_glsl_reflection_semicolon,
	fck_glsl_reflection_operator,
	fck_glsl_reflection_constant,
	fck_glsl_reflection_comma,
	fck_glsl_reflection_data_type,
	fck_glsl_reflection_binding,
	fck_glsl_reflection_unknown,
	fck_glsl_reflection_end_of_source,
	fck_glsl_reflection_count
} fck_glsl_reflection_token;

typedef struct fck_glsl_reflection_source_token
{
	fck_glsl_reflection_token type;
	const char *source;
	fckc_size_t length;
} fck_glsl_reflection_source_token;

typedef struct fck_glsl_reflection_source_token_reference
{
	fck_glsl_reflection_source_token *value;
} fck_glsl_reflection_source_token_reference;

typedef struct fck_glsl_reflection_source_token_reference_list
{
	fck_glsl_reflection_source_token_reference *values;
	fckc_size_t count;
} fck_glsl_reflection_source_token_reference_list;

typedef struct fck_glsl_reflection_source
{
	fck_glsl_reflection_source_token *tokens;
	fckc_size_t count;
	fck_glsl_reflection_source_token_reference_list references[fck_glsl_reflection_count];
} fck_glsl_reflection_source;

typedef enum fck_glsl_reflection_declaration_kind
{
	fck_glsl_reflection_declaration_kind_none = 0,
	fck_glsl_reflection_declaration_kind_interface_block, // e.g., uniform Config { ... } name;
	fck_glsl_reflection_declaration_kind_struct,          // e.g., struct Light { ... } name;
	fck_glsl_reflection_declaration_kind_variable,        // e.g., uniform int value; OR in vec3 pos;
	fck_glsl_reflection_declaration_kind_function,        // e.g., void identifier( ... ) or  void identifier( ... ) { ... };
} fck_glsl_reflection_declaration_kind;

typedef struct fck_glsl_reflection_declaration
{
	fck_glsl_reflection_declaration_kind kind;

	fck_glsl_reflection_declaration_qualifier qualifiers;

	int layout_binding;
	int layout_location;

	const fck_glsl_reflection_source_token *type_name;  // e.g., "vec4", "Light", "Config" (Can be NULL for anonymous structs)
	const fck_glsl_reflection_source_token *identifier; // e.g., "value", "pos", "configuration"

	// Ehhhhh
	int is_array;
	// Should be parsed directly
	const fck_glsl_reflection_source_token *array_size; // Points to the value inside [ ]

	fckc_size_t body_start_index;
	fckc_size_t body_end_index;
} fck_glsl_reflection_declaration;

typedef struct fck_glsl_reflection_iterator
{
	const fck_glsl_reflection_source *source;
	fckc_size_t current_index;
	fckc_size_t end_index; // Prevents the iterator from bleeding out of a nested scope
} fck_glsl_reflection_iterator;

static fckc_size_t fck_glsl_reflection_scope(const fck_glsl_reflection_source *source, fckc_size_t current,
                                             fck_glsl_reflection_token open_tok, fck_glsl_reflection_token close_tok)
{
	fckc_size_t depth = 1;
	fckc_size_t next = current + 1;

	while (next < source->count)
	{
		const fck_glsl_reflection_source_token next_token = source->tokens[next];
		if (next_token.type == open_tok)
			depth++;
		else if (next_token.type == close_tok)
		{
			depth--;
			if (depth == 0)
				return next;
		}
		next++;
	}
	return next;
}

static const char *fck_glsl_reflection_source_skip_space(const char *str)
{
	while (*str && isspace(*str))
		str++;
	return str;
}

static const char *fck_glsl_reflection_source_skip_until_non_number(const char *str)
{
	while (*str && (isdigit(*str) || *str == '.'))
		str++;
	return str;
}

static int fck_glsl_reflection_isc_identifier(char c)
{
	return isalnum(c) || c == '_';
}

static const char *fck_glsl_reflection_source_skip_until_non_identifier(const char *str)
{
	while (*str && fck_glsl_reflection_isc_identifier(*str))
		str++;
	return str;
}

static const char *fck_glsl_reflection_source_token_next(const char *current, fck_glsl_reflection_source_token *out_token)
{
	typedef struct keywords_entry
	{
		const char *keyword;
		fck_glsl_reflection_token token;
	} keywords_entry;

	const static keywords_entry keywords[] = {{"struct", fck_glsl_reflection_struct},
	                                          {"layout", fck_glsl_reflection_layout},
	                                          {"uniform", fck_glsl_reflection_uniform},
	                                          {"readonly", fck_glsl_reflection_readonly},
	                                          {"shared", fck_glsl_reflection_shared},
	                                          {"buffer", fck_glsl_reflection_buffer},
	                                          {"in", fck_glsl_reflection_in},
	                                          {"out", fck_glsl_reflection_out},
	                                          {"float", fck_glsl_reflection_data_type},
	                                          {"int", fck_glsl_reflection_data_type},
	                                          {"uint", fck_glsl_reflection_data_type},
	                                          {"vec2", fck_glsl_reflection_data_type},
	                                          {"vec3", fck_glsl_reflection_data_type},
	                                          {"vec4", fck_glsl_reflection_data_type},
	                                          {"mat3", fck_glsl_reflection_data_type},
	                                          {"mat4", fck_glsl_reflection_data_type},
	                                          {"sampler2D", fck_glsl_reflection_data_type},
	                                          {"if", fck_glsl_reflection_unknown},
	                                          {"else if", fck_glsl_reflection_unknown},
	                                          {"else", fck_glsl_reflection_unknown}};

	current = fck_glsl_reflection_source_skip_space(current);
	out_token->type = fck_glsl_reflection_unknown;
	out_token->source = current;
	out_token->length = 1;

	if (*current == '\0')
	{
		out_token->type = fck_glsl_reflection_end_of_source;
		out_token->length = 0;
		return current;
	}

	if (*current == '_' || isalpha(*current))
	{
		const char *next = fck_glsl_reflection_source_skip_until_non_identifier(current + 1);
		out_token->length = (fckc_size_t)(next - current);
		out_token->type = fck_glsl_reflection_identifier;

		for (fckc_size_t i = 0; i < (sizeof(keywords) / sizeof(keywords[0])); i++)
		{
			const fckc_size_t kw_len = strlen(keywords[i].keyword);
			if (out_token->length == kw_len && strncmp(current, keywords[i].keyword, kw_len) == 0)
			{
				out_token->type = keywords[i].token;
				break;
			}
		}
	}
	else if (isdigit(*current))
	{
		const char *next = fck_glsl_reflection_source_skip_until_non_number(current);
		out_token->type = fck_glsl_reflection_constant;
		out_token->length = (fckc_size_t)(next - current);
	}
	else if (ispunct(*current))
	{
		out_token->length = 1;
		switch (*current)
		{
		case '{':
			out_token->type = fck_glsl_reflection_brace_open;
			break;
		case '}':
			out_token->type = fck_glsl_reflection_brace_close;
			break;
		case '(':
			out_token->type = fck_glsl_reflection_paren_open;
			break;
		case ')':
			out_token->type = fck_glsl_reflection_paren_close;
			break;
		case '[':
			out_token->type = fck_glsl_reflection_bracket_open;
			break;
		case ']':
			out_token->type = fck_glsl_reflection_bracket_close;
			break;
		case ';':
			out_token->type = fck_glsl_reflection_semicolon;
			break;
		case ',':
			out_token->type = fck_glsl_reflection_comma;
			break;
		default:
			out_token->type = fck_glsl_reflection_operator;
			break;
		}
	}

	return out_token->source + out_token->length;
}

// --- Public API Implementation ---

static void fck_glsl_reflection_source_parse(const char *text, fck_glsl_reflection_source *source)
{
	const char *current = text;
	memset(source->references, 0, sizeof(source->references));

	while (*current)
	{
		fck_glsl_reflection_source_token dummy;
		current = fck_glsl_reflection_source_token_next(current, &dummy);
		if (dummy.type == fck_glsl_reflection_end_of_source)
			break;
		source->references[dummy.type].count++;
	}

	source->count = 0;
	for (fckc_size_t index = 0; index < fck_glsl_reflection_count; index++)
	{
		fck_glsl_reflection_source_token_reference_list *list = source->references + index;
		if (list->count > 0)
		{
			list->values = (fck_glsl_reflection_source_token_reference *)kll_malloc(kll->system, list->count * sizeof(*list->values));
			source->count += list->count;
		}
	}

	source->tokens = (fck_glsl_reflection_source_token *)kll_malloc(kll->system, (source->count + 1) * sizeof(*source->tokens));

	// Pass 2
	current = text;
	fckc_size_t token_entry_index[fck_glsl_reflection_count] = {0};
	fckc_size_t token_index = 0;

	while (*current)
	{
		fck_glsl_reflection_source_token *token = source->tokens + token_index;
		current = fck_glsl_reflection_source_token_next(current, token);
		if (token->type == fck_glsl_reflection_end_of_source)
			break;

		fck_glsl_reflection_source_token_reference_list *list = source->references + token->type;
		list->values[token_entry_index[token->type]++].value = token;
		token_index++;
	}
}

static void fck_glsl_reflection_source_free(fck_glsl_reflection_source *source)
{
	if (source->tokens)
	{
		kll_free(kll->system, source->tokens);
	}
	for (int i = 0; i < fck_glsl_reflection_count; i++)
	{
		if (source->references[i].values)
		{
			kll_free(kll->system, source->references[i].values);
		}
	}
	memset(source, 0, sizeof(*source));
}

static fck_glsl_reflection_iterator fck_glsl_reflection_iterate_begin(const fck_glsl_reflection_source *source)
{
	fck_glsl_reflection_iterator iter = {source, 0, source->count};
	return iter;
}

static fck_glsl_reflection_iterator fck_glsl_reflection_iterate_scope(const fck_glsl_reflection_source *source, fckc_size_t start,
                                                                      fckc_size_t end)
{
	fck_glsl_reflection_iterator iter = {source, start, end};
	return iter;
}

static const fck_glsl_reflection_source_token *fck_glsl_token_at(const fck_glsl_reflection_source *source, fckc_size_t index,
                                                                 fckc_size_t end_limit)
{
	if (index >= source->count || index >= end_limit)
		return NULL;
	return &source->tokens[index];
}

static int fck_glsl_reflection_iterate_declarations(fck_glsl_reflection_iterator *iter, fck_glsl_reflection_declaration *out_decl)
{
	memset(out_decl, 0, sizeof(*out_decl));
	out_decl->layout_binding = -1;
	out_decl->layout_location = -1;

	const fck_glsl_reflection_source *src = iter->source;
	const fckc_size_t limit = iter->end_index;

	while (iter->current_index < limit)
	{
		const fck_glsl_reflection_source_token *tok = fck_glsl_token_at(src, iter->current_index, limit);
		if (!tok || tok->type == fck_glsl_reflection_end_of_source)
			return 0;

		if (tok->type == fck_glsl_reflection_layout)
		{
			const fckc_size_t next_idx = iter->current_index + 1;
			const fck_glsl_reflection_source_token *next = fck_glsl_token_at(src, next_idx, limit);
			if (next && next->type == fck_glsl_reflection_paren_open)
			{
				const fckc_size_t close_idx =
					fck_glsl_reflection_scope(src, next_idx, fck_glsl_reflection_paren_open, fck_glsl_reflection_paren_close);
				for (fckc_size_t i = next_idx + 1; i < close_idx; i++)
				{
					const fck_glsl_reflection_source_token *inner = fck_glsl_token_at(src, i, limit);
					if (inner && inner->type == fck_glsl_reflection_identifier)
					{
						if (inner->length == 7 && strncmp(inner->source, "binding", 7) == 0)
						{
							const fck_glsl_reflection_source_token *val = fck_glsl_token_at(src, i + 2, limit);
							if (val && val->type == fck_glsl_reflection_constant)
								out_decl->layout_binding = atoi(val->source);
						}
						else if (inner->length == 8 && strncmp(inner->source, "location", 8) == 0)
						{
							const fck_glsl_reflection_source_token *val = fck_glsl_token_at(src, i + 2, limit);
							if (val && val->type == fck_glsl_reflection_constant)
								out_decl->layout_location = atoi(val->source);
						}
					}
				}
				iter->current_index = close_idx + 1;
				continue;
			}
		}

		if (tok->type == fck_glsl_reflection_uniform)
		{
			out_decl->qualifiers = out_decl->qualifiers | fck_glsl_reflection_declaration_qualifier_uniform;
			iter->current_index++;
			continue;
		}
		if (tok->type == fck_glsl_reflection_buffer)
		{
			out_decl->qualifiers = out_decl->qualifiers | fck_glsl_reflection_declaration_qualifier_buffer;
			iter->current_index++;
			continue;
		}
		if (tok->type == fck_glsl_reflection_shared)
		{
			out_decl->qualifiers = out_decl->qualifiers | fck_glsl_reflection_declaration_qualifier_shared;
			iter->current_index++;
			continue;
		}
		if (tok->type == fck_glsl_reflection_readonly)
		{
			out_decl->qualifiers = out_decl->qualifiers | fck_glsl_reflection_declaration_qualifier_readonly;
			iter->current_index++;
			continue;
		}
		if (tok->type == fck_glsl_reflection_in)
		{
			out_decl->qualifiers = out_decl->qualifiers | fck_glsl_reflection_declaration_qualifier_in;
			iter->current_index++;
			continue;
		}
		if (tok->type == fck_glsl_reflection_out)
		{
			out_decl->qualifiers = out_decl->qualifiers | fck_glsl_reflection_declaration_qualifier_out;
			iter->current_index++;
			continue;
		}

		if (tok->type == fck_glsl_reflection_struct)
		{
			out_decl->kind = fck_glsl_reflection_declaration_kind_struct;

			// Might be anonymous! (GLSL specifications forbid it)
			const fck_glsl_reflection_source_token *next = fck_glsl_token_at(src, iter->current_index + 1, limit);
			fckc_size_t brace_idx = iter->current_index + 1;

			if (next && next->type == fck_glsl_reflection_identifier)
			{
				out_decl->type_name = next;
				brace_idx++;
			}

			const fck_glsl_reflection_source_token *brace = fck_glsl_token_at(src, brace_idx, limit);
			if (brace && brace->type == fck_glsl_reflection_brace_open)
			{
				out_decl->body_start_index = brace_idx + 1;
				out_decl->body_end_index =
					fck_glsl_reflection_scope(src, brace_idx, fck_glsl_reflection_brace_open, fck_glsl_reflection_brace_close);
			}

			const fck_glsl_reflection_source_token *after_brace = fck_glsl_token_at(src, out_decl->body_end_index + 1, limit);
			if (after_brace && after_brace->type == fck_glsl_reflection_identifier)
				out_decl->identifier = after_brace;

			goto advance_to_semicolon;
		}

		if (tok->type == fck_glsl_reflection_data_type || tok->type == fck_glsl_reflection_identifier)
		{
			out_decl->type_name = tok;
			const fck_glsl_reflection_source_token *next = fck_glsl_token_at(src, iter->current_index + 1, limit);

			// Interface Block
			if ((out_decl->qualifiers | fck_glsl_reflection_declaration_qualifier_interface_mask) && next &&
			    next->type == fck_glsl_reflection_brace_open)
			{
				out_decl->kind = fck_glsl_reflection_declaration_kind_interface_block;
				out_decl->body_start_index = iter->current_index + 2;
				out_decl->body_end_index = fck_glsl_reflection_scope(src, iter->current_index + 1, fck_glsl_reflection_brace_open,
				                                                     fck_glsl_reflection_brace_close);

				const fck_glsl_reflection_source_token *after_brace = fck_glsl_token_at(src, out_decl->body_end_index + 1, limit);
				if (after_brace && after_brace->type == fck_glsl_reflection_identifier)
					out_decl->identifier = after_brace;

				goto advance_to_semicolon;
			}

			// Standard Variable
			if (next && next->type == fck_glsl_reflection_identifier)
			{
				// We identify paren scope and we know it is a function
				// If we can also see a curly block, we can skip it all cause it is a function definition
				out_decl->identifier = next;
				const fckc_size_t possible_extra = iter->current_index + 2;
				const fck_glsl_reflection_source_token *paren_open = fck_glsl_token_at(src, possible_extra, limit);
				if (paren_open && paren_open->type == fck_glsl_reflection_paren_open)
				{
					out_decl->kind = fck_glsl_reflection_declaration_kind_function;

					const fckc_size_t parameters_end =
						fck_glsl_reflection_scope(src, possible_extra, fck_glsl_reflection_paren_open, fck_glsl_reflection_paren_close);
					const fck_glsl_reflection_source_token *after_parens = fck_glsl_token_at(src, parameters_end + 1, limit);
					if (after_parens && after_parens->type == fck_glsl_reflection_brace_open)
					{
						out_decl->body_start_index = possible_extra + 1;
						out_decl->body_end_index =
							fck_glsl_reflection_scope(src, possible_extra, fck_glsl_reflection_paren_open, fck_glsl_reflection_paren_close);
					}
				}
				else
				{
					out_decl->kind = fck_glsl_reflection_declaration_kind_variable;

					// Array check
					const fck_glsl_reflection_source_token *bracket = fck_glsl_token_at(src, iter->current_index + 2, limit);
					if (bracket && bracket->type == fck_glsl_reflection_bracket_open)
					{
						out_decl->is_array = 1;
						const fck_glsl_reflection_source_token *sz = fck_glsl_token_at(src, iter->current_index + 3, limit);
						if (sz && (sz->type == fck_glsl_reflection_constant || sz->type == fck_glsl_reflection_identifier))
							out_decl->array_size = sz;
					}
				}
				goto advance_to_semicolon;
			}
		}

		iter->current_index++;
		continue;

	advance_to_semicolon:
		while (iter->current_index < limit)
		{
			if (fck_glsl_token_at(src, iter->current_index, limit)->type == fck_glsl_reflection_semicolon)
			{
				iter->current_index++;
				return 1;
			}
			iter->current_index++;
		}
		return 1;
	}
	return 0;
}

typedef struct fck_glsl_reflection
{
	// Static allocation for now
	kll_arena *variables_arena;
	kll_arena *names_arena;

	fck_glsl_reflection_type *types;
	fckc_size_t types_capacity;
} fck_glsl_reflection;

static fck_glsl_reflection_type *fck_glsl_reflection_find_type(fck_glsl_reflection *reflection, const char *name)
{
	const fckc_u64 hash = to_u64(fck_hash(name, strlen(name)));
	const fckc_u64 size = reflection->types_capacity;

	for (fckc_u64 index = 0; index < size; index++)
	{
		const fckc_u64 slot = (hash + index) % size;
		fck_glsl_reflection_type *type = reflection->types + slot;
		if (type->name == NULL)
		{
			return NULL;
		}
		if (strcmp(type->name, name) == 0)
		{
			return type;
		}
	}

	return NULL;
}

static fck_glsl_reflection_type *fck_glsl_reflection_declare_type(fck_glsl_reflection *reflection, const char *name,
                                                                  const fck_glsl_reflection_type **current)
{
	const fckc_u64 hash = to_u64(fck_hash(name, strlen(name)));
	const fckc_u64 size = reflection->types_capacity;

	for (fckc_u64 index = 0; index < size; index++)
	{
		const fckc_u64 slot = (hash + index) % size;
		fck_glsl_reflection_type *type = reflection->types + slot;
		if (current)
		{
			*current = type;
		}

		if (type->name == NULL)
		{
			type->name = kll_format(reflection->names_arena, name);
			type->first = NULL;
			return type;
		}
		if (strcmp(type->name, name) == 0)
		{
			return NULL;
		}
	}

	if (current)
	{
		*current = NULL;
	}
	return NULL;
}

static fck_glsl_reflection_variable *fck_glsl_reflection_add_field(fck_glsl_reflection *reflection, fck_glsl_reflection_type *owner,
                                                                   const fck_glsl_reflection_type *type, const char *name,
                                                                   fck_glsl_reflection_variable *last)
{
	const fckc_size_t size = sizeof(fck_glsl_reflection_variable);
	fck_glsl_reflection_variable *variable = (fck_glsl_reflection_variable *)kll_malloc(reflection->variables_arena, size);

	variable->type = type;
	variable->name = kll_format(reflection->names_arena, name);
	variable->next = NULL;
	variable->binding = -1;
	variable->qualifiers = fck_glsl_reflection_declaration_qualifier_none;

	if (last == NULL)
	{
		owner->first = variable;
	}
	else
	{
		last->next = variable;
	}
	return variable;
}

static fck_glsl_reflection *fck_glsl_reflection_alloc(fckc_size_t types_capacity)
{
	fck_glsl_reflection *reflection;

	const fckc_size_t native_types_count = 64; // Hit or miss
	types_capacity = types_capacity + native_types_count;

	fckc_size_t total = 0;
	total = total + (sizeof(*reflection));

	const fckc_size_t types_offset = total = fckc_align(total, alignof(fck_glsl_reflection_type));
	total = total + (sizeof(*reflection->types) * types_capacity);

	fckc_u8 *memory = (fckc_u8 *)kll_malloc(kll->system, total);
	memset(memory, 0, total);

	reflection = (fck_glsl_reflection *)memory;
	reflection->types = (fck_glsl_reflection_type *)(memory + types_offset);
	reflection->variables_arena = kll->arena->create(kll->system, sizeof(fck_glsl_reflection_variable) * 32);
	reflection->names_arena = kll->arena->create(kll->system, 512);
	reflection->types_capacity = types_capacity;

	fck_glsl_reflection_type *float_type = fck_glsl_reflection_declare_type(reflection, "float", NULL);
	fck_glsl_reflection_type *int_type = fck_glsl_reflection_declare_type(reflection, "int", NULL);
	fck_glsl_reflection_type *bool_type = fck_glsl_reflection_declare_type(reflection, "bool", NULL);
	fck_glsl_reflection_type *uint_type = fck_glsl_reflection_declare_type(reflection, "uint", NULL);
	fck_glsl_reflection_type *double_type = fck_glsl_reflection_declare_type(reflection, "double", NULL);

	// TODO: Add all the other vector and matrix types!!
	fck_glsl_reflection_type *vec2 = fck_glsl_reflection_declare_type(reflection, "vec2", NULL);
	fck_glsl_reflection_type *vec3 = fck_glsl_reflection_declare_type(reflection, "vec3", NULL);
	fck_glsl_reflection_type *vec4 = fck_glsl_reflection_declare_type(reflection, "vec4", NULL);

	fck_glsl_reflection_variable *last = NULL;
	last = fck_glsl_reflection_add_field(reflection, vec2, float_type, "x", last);
	last = fck_glsl_reflection_add_field(reflection, vec2, float_type, "y", last);

	last = NULL;
	last = fck_glsl_reflection_add_field(reflection, vec3, float_type, "x", last);
	last = fck_glsl_reflection_add_field(reflection, vec3, float_type, "y", last);
	last = fck_glsl_reflection_add_field(reflection, vec3, float_type, "z", last);

	last = NULL;
	last = fck_glsl_reflection_add_field(reflection, vec4, float_type, "x", last);
	last = fck_glsl_reflection_add_field(reflection, vec4, float_type, "y", last);
	last = fck_glsl_reflection_add_field(reflection, vec4, float_type, "x", last);
	last = fck_glsl_reflection_add_field(reflection, vec4, float_type, "w", last);

	return reflection;
}

static void fck_glsl_reflection_reflecton_free(fck_glsl_reflection *reflection)
{
	kll->arena->destroy(reflection->names_arena);
	kll->arena->destroy(reflection->variables_arena);
	kll_free(kll->system, reflection);
}

static const char *fck_glsl_reflection_source_token_to_string(const fck_glsl_reflection_source_token *token, char *name_buffer, int size)
{
	const int result = snprintf(name_buffer, size, "%.*s", (int)token->length, token->source);
	if (result >= 0)
	{
		return name_buffer;
	}
	return NULL;
}

static void fck_glsl_reflection_reflect_scope(fck_glsl_reflection *reflection, fck_glsl_reflection_type *owner,
                                              fck_glsl_reflection_iterator *iter, int indent)
{
	fck_glsl_reflection_declaration decl;
	fck_glsl_reflection_variable *last = NULL;

	char name_buffer[512];

	while (fck_glsl_reflection_iterate_declarations(iter, &decl))
	{
		// Print formatting
		switch (decl.kind)
		{
			/*case fck_glsl_reflection_declaration_kind_function: {
			     //TODO: Fix it up to handle function declaration and definitions
			}*/
			break;
		case fck_glsl_reflection_declaration_kind_interface_block:
		case fck_glsl_reflection_declaration_kind_struct: {
			if (decl.type_name)
			{
				fck_glsl_reflection_iterator scope =
					fck_glsl_reflection_iterate_scope(iter->source, decl.body_start_index, decl.body_end_index);
				const char *source = fck_glsl_reflection_source_token_to_string(decl.type_name, name_buffer, sizeof(name_buffer));
				if (source)
				{
					const fck_glsl_reflection_type *custom_type = NULL;
					fck_glsl_reflection_type *result = fck_glsl_reflection_declare_type(reflection, name_buffer, &custom_type);
					if (result)
					{
						fck_glsl_reflection_reflect_scope(reflection, result, &scope, indent + 1);

						if (decl.identifier)
						{
							source = fck_glsl_reflection_source_token_to_string(decl.identifier, name_buffer, sizeof(name_buffer));
							last = fck_glsl_reflection_add_field(reflection, owner, custom_type, source, last);
						}
						else
						{
							last = fck_glsl_reflection_add_field(reflection, owner, custom_type, "", last);
						}
						last->qualifiers = decl.qualifiers;
						last->binding = decl.layout_binding;
					}
				}
				// Consume inner index cause we want to skip the scope! :)
				iter->current_index = scope.current_index;
			}
		}
		break;

		case fck_glsl_reflection_declaration_kind_variable: {
			const char *source = fck_glsl_reflection_source_token_to_string(decl.type_name, name_buffer, sizeof(name_buffer));
			if (source)
			{
				const fck_glsl_reflection_type *type;
				fck_glsl_reflection_type *result = fck_glsl_reflection_declare_type(reflection, source, &type);
				if (result)
				{
					// Since it is a variable, there should not be a scope... right?
				}

				source = fck_glsl_reflection_source_token_to_string(decl.identifier, name_buffer, sizeof(name_buffer));
				last = fck_glsl_reflection_add_field(reflection, owner, type, source, last);
				last->binding = decl.layout_binding;
				last->qualifiers = decl.qualifiers;
			}
		}
		break;

		default:
			break;
		}
	}
}

static struct fck_glsl_reflection *fck_glsl_reflection_api_reflect(const char *source, const char *global)
{
	fck_glsl_reflection_source src;
	fck_glsl_reflection_source_parse(source, &src);

	fck_glsl_reflection *reflection = fck_glsl_reflection_alloc(256);
	fck_glsl_reflection_type *global_type = fck_glsl_reflection_declare_type(reflection, global, NULL);
	fck_glsl_reflection_iterator root_iter = fck_glsl_reflection_iterate_begin(&src);
	fck_glsl_reflection_reflect_scope(reflection, global_type, &root_iter, 0);
	fck_glsl_reflection_source_free(&src);

	return reflection;
}

static const fck_glsl_reflection_type *fck_glsl_reflection_api_type_of(fck_glsl_reflection *reflection, const char *name)
{
	fck_glsl_reflection *shader_reflection = (fck_glsl_reflection *)reflection;
	const fck_glsl_reflection_type *type = fck_glsl_reflection_find_type(shader_reflection, name);
	return type;
}

static void fck_glsl_reflection_api_free(fck_glsl_reflection *reflection)
{
	fck_glsl_reflection *shader_reflection = (fck_glsl_reflection *)reflection;
	fck_glsl_reflection_reflecton_free(shader_reflection);
}

static int fck_glsl_reflection_api_is(const fck_glsl_reflection_type *type, const char *name)
{
	if(type == NULL) 
	{
		return 0;
	}

	return strcmp(type->name, name) == 0;
}

static fck_glsl_reflection_api glsl_reflection_api = {
	.reflect = fck_glsl_reflection_api_reflect,
	.type_of = fck_glsl_reflection_api_type_of,
	.free = fck_glsl_reflection_api_free,
	.is = fck_glsl_reflection_api_is,
};

fck_glsl_reflection_api *glsl_reflection = &glsl_reflection_api;
