
#include <fck_apis.h>
#include <fck_hash.h>
#include <fck_input.h>
#include <fck_mouse.h>
#include <fck_os.h>
#include <fck_pkey.h>
#include <fck_plugins.h>
#include <fck_shader.h>
#include <fckc_assert.h>
#include <fckc_inttypes.h>
#include <sht_render.h>

#include <kll.h>
#include <kll_malloc.h>
#include <kll_system.h>

#include <stdio.h>

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static void purge_files(const char *pattern)
{
	char **paths;
	const fckc_size_t count = os->glob->executable(pattern, &paths);
	for (fckc_size_t index = 0; index < count; index++)
	{
		char *path = paths[index];
		os->fs->remove(path);
	}
	os->glob->free(paths);
}

static fck_api_registry *fck_api_registry_load(const char *path)
{
	// This badboy needs to get released
	const fck_shared_object so = os->so->load(path);
	fck_load_func *loader = (fck_load_func *)os->so->symbol(so, "fck_api_load");
	fck_api_registry *registry = (fck_api_registry *)loader(NULL, NULL);
	return registry;
}

static fck_plugins_api *fck_plugins_load(fck_api_registry *registry, const char *path)
{
	// This badboy needs to get released
	const fck_shared_object so = os->so->load(path);
	fck_load_func *loader = (fck_load_func *)os->so->symbol(so, "fck_plugins_load");
	fck_plugins_api *plugins = (fck_plugins_api *)loader(registry, NULL);
	return plugins;
}

static void load_config(int argc, char **argv)
{
	for (int index = 0; index < argc; index++)
	{
		char *value = argv[index];
		os->io->log(value);
	}
}

typedef struct app_screen
{
	float width;
	float height;
} app_screen;

typedef struct app_config
{
	fckc_i32 gradient;
	fckc_i32 is_sdf;
} app_config;

typedef struct app_quad_transform
{
	float x;
	float y;
	float z;
	float rotation;
	float width;
	float height;
	float scale;
} app_quad_transform;

typedef struct app_line_transform
{
	float sx;
	float sy;
	float ex;
	float ey;
	float z;
	float thickness;
	float scale;
} app_line_transform;

// This will backlash. Try to keep them the same size, else the stride and all that stuff needs to stay opaque! We are lucky for now :D
typedef union app_shape_transform {
	app_quad_transform quad;
	app_line_transform line;
} app_shape_transform;

typedef struct app_quads
{
	app_quad_transform transforms[64];
	fckc_u32 count;
} app_quads;

static void app_quads_add(app_quads *quads, float x, float y)
{
	const app_quad_transform init_transform = {
		.x = x,
		.y = y,
		.z = 0.0f,
		.rotation = 0.0f,
		.width = 100.0f,
		.height = 100.0f,
		.scale = 1.0f,
	};

	app_quad_transform *transform = quads->transforms + quads->count;
	*transform = init_transform;
	quads->count = quads->count + 1;
}

typedef struct app_lines
{
	app_line_transform transforms[64];
	fckc_u32 count;
} app_lines;

static void app_lines_add(app_lines *quads, float sx, float sy, float ex, float ey, float thickness)
{
	const app_line_transform init_transform = {
		.sx = sx,
		.sy = sy,
		.ex = ex,
		.ey = ey,
		.z = 0.0f,
		.thickness = thickness,
		.scale = 1.0f,
	};

	app_line_transform *transform = quads->transforms + quads->count;
	*transform = init_transform;
	quads->count = quads->count + 1;
}

typedef enum app_graphics_shape
{
	app_shape_quad,
	app_shape_line,
	app_shape_count,
} app_graphics_shape;

static const char *app_graphics_shape_to_string(app_graphics_shape primitive)
{
	switch (primitive)
	{
	case app_shape_quad:
		return "app_shape_quad";
	case app_shape_line:
		return "app_shape_line";
	case app_shape_count:
		return "app_shape_line";
	}
	return "app_shape_unknown";
}

typedef enum app_graphics_style
{
	app_style_solid,
	app_style_textured,
	app_style_rounded,
	app_style_count,
} app_graphics_style;

static const char *app_graphic_material_to_string(app_graphics_style material)
{
	switch (material)
	{
	case app_style_solid:
		return "app_style_solid";
	case app_style_textured:
		return "app_style_textured";
	case app_style_rounded:
		return "app_style_rounded";
	case app_style_count:
		break;
	}
	return "app_style_unknown";
}

static fckc_size_t app_graphic_pipeline_bindings(app_graphics_shape primitive, app_graphics_style material,
                                                 sht_binding const **out_bindings)
{
	switch (primitive)
	{
	case app_shape_quad:
	case app_shape_line:
		switch (material)
		{
		case app_style_solid:
		case app_style_rounded:
			static const sht_binding bindings[] = {
				{.id = 0, .type = SHT_BINDING_UNIFORM, .stages = SHT_STAGE_VERTEX_SHADER},
				{.id = 1, .type = SHT_BINDING_STORAGE, .stages = SHT_STAGE_VERTEX_SHADER},
				{.id = 2, .type = SHT_BINDING_UNIFORM, .stages = SHT_STAGE_VERTEX_SHADER},
				{.id = 3, .type = SHT_BINDING_READ_ONLY_IMAGE, .stages = SHT_STAGE_FRAGMENT_SHADER},
				{.id = 5, .type = SHT_BINDING_UNIFORM, .stages = SHT_STAGE_VERTEX_SHADER | SHT_STAGE_FRAGMENT_SHADER},
				// TODO: Configuration binding
			};
			*out_bindings = bindings;
			return fck_arraysize(bindings);
		case app_style_textured:
			static const sht_binding textured_bindings[] = {
				{.id = 0, .type = SHT_BINDING_UNIFORM, .stages = SHT_STAGE_VERTEX_SHADER},
				{.id = 1, .type = SHT_BINDING_STORAGE, .stages = SHT_STAGE_VERTEX_SHADER},
				{.id = 2, .type = SHT_BINDING_UNIFORM, .stages = SHT_STAGE_VERTEX_SHADER},
				{.id = 3, .type = SHT_BINDING_READ_ONLY_IMAGE, .stages = SHT_STAGE_FRAGMENT_SHADER},
				{.id = 5, .type = SHT_BINDING_UNIFORM, .stages = SHT_STAGE_VERTEX_SHADER | SHT_STAGE_FRAGMENT_SHADER},
			};
			*out_bindings = textured_bindings;
			return fck_arraysize(textured_bindings);
		case app_style_count:
			break;
		}
		break;
	case app_shape_count:
		break;
	}
	return 0;
}

typedef struct app_property_string
{
	// If there is a need for a longer string, fuck you
	const char *value;
} app_property_string;

typedef enum app_property_type
{
	// What else do you need for now?
	app_property_float,
} app_property_type;

#define app_properties_capacity 16

typedef struct app_properties
{
	app_property_string keys[app_properties_capacity];
	app_property_type types[app_properties_capacity];
	fckc_size_t offsets[app_properties_capacity];
	fckc_u8 buffer[sizeof(float) * app_properties_capacity];
	fckc_size_t size;
} app_properties;

static void app_property_structure_set_float(app_properties *properties, const char *name, float value)
{
	for (fckc_size_t index = 0; index < app_properties_capacity; index++)
	{
		app_property_string *key = properties->keys + index;
		if (key->value == NULL)
		{
			key->value = name;
		}
		if (strcmp(key->value, name) == 0)
		{
			app_property_type *type = properties->types + index;
			*type = app_property_float;

			// Do not forget alignup later! :)
			fckc_size_t *offset = properties->offsets + index;
			*offset = properties->size;
			properties->size = properties->size + sizeof(value);

			fckc_u8 *dst = properties->buffer + *offset;
			memcpy(dst, &value, sizeof(value));
			return;
		}
	}
}

static void app_property_structure_upload(app_properties *properties, sht_driver driver, sht_bss bss)
{
	if (properties->size == 0)
	{
		return;
	}

	const sht_buffer_upload_desc upload = {
		.data = properties->buffer,
		.size = properties->size,
		.count = 1,
	};
	// TODO: Hardcoded binding!!!
	driver.vt->bss->upload_buffer(bss, 5, &upload);
}

typedef struct app_graphic_pipeline
{
	sht_graphics_pipeline pipeline;
	sht_bss bss;

	app_properties properties;

	app_shape_transform *transforms;
	fckc_u32 count;
	fckc_u32 capacity;
} app_graphic_pipeline;

typedef struct app_graphics
{
	fck_shader_api *shader;
	sht_driver driver;
	app_graphic_pipeline values[app_shape_count][app_style_count];
} app_graphics;

static void app_graphics_init(app_graphics *graphics, fck_shader_api *shader, sht_driver driver)
{
	graphics->shader = shader;
	graphics->driver = driver;
}

static const char *app_parse_number(const char *begin, const char *end, unsigned long *number)
{
	const char *str = begin;
	while (!isdigit(*str))
	{
		str++;
		if (str == end)
		{
			return NULL;
		}
	}

	char *after;
	*number = strtoul(str, &after, 10);
	// Query error?
	// Check for ULONG_MAX?
	// YOLO
	return after;
}

typedef enum app_shader_primitive_type
{
	app_shader_primitive_none,
	app_shader_primitive_bool,
	app_shader_primitive_int,
	app_shader_primitive_uint,
	app_shader_primitive_float,
	app_shader_primitive_double,
	app_shader_primitive_not_supportd,
} app_shader_primitive_type;

typedef enum app_shader_token
{
	app_shader_identifier = 0,
	app_shader_brace_open = 1,
	app_shader_brace_close = 2,
	app_shader_bracket_open = 3,
	app_shader_bracket_close = 4,
	app_shader_paren_open = 5,
	app_shader_paren_close = 6,
	app_shader_struct = 7,
	app_shader_layout = 8,
	app_shader_uniform = 9,
	app_shader_readonly = 10,
	app_shader_shared = 11,
	app_shader_buffer = 12,
	app_shader_void = 14,
	app_shader_semicolon = 15,
	app_shader_operator = 16,
	app_shader_constant = 17,
	app_shader_comma = 18,
	app_shader_block = 19,
	app_shader_type = 20,
	app_shader_binding = 21,
	app_shader_unknown,
	app_shader_end_of_source,
	app_shader_count,
} app_shader_token;

typedef enum app_shader_token_flags
{
	app_shader_token_none = 0,
	app_shader_token_identifier = 1 << app_shader_identifier,
	app_shader_token_brace_open = 1 << app_shader_brace_open,
	app_shader_token_brace_close = 1 << app_shader_brace_close,
	app_shader_token_bracket_open = 1 << app_shader_bracket_open,
	app_shader_token_bracket_close = 1 << app_shader_bracket_close,
	app_shader_token_paren_open = 1 << app_shader_paren_open,
	app_shader_token_paren_close = 1 << app_shader_paren_close,
	app_shader_token_struct = 1 << app_shader_struct,
	app_shader_token_layout = 1 << app_shader_layout,
	app_shader_token_uniform = 1 << app_shader_uniform,
	app_shader_token_readonly = 1 << app_shader_readonly,
	app_shader_token_shared = 1 << app_shader_shared,
	app_shader_token_buffer = 1 << app_shader_buffer,
	app_shader_token_void = 1 << app_shader_void,
	app_shader_token_semicolon = 1 << app_shader_semicolon,
	app_shader_token_operator = 1 << app_shader_operator,
	app_shader_token_constant = 1 << app_shader_constant,
	app_shader_token_comma = 1 << app_shader_comma,
	app_shader_token_block = 1 << app_shader_block, // TODO: Maybe this should be any? Or text? idk...
	app_shader_token_type = 1 << app_shader_type,
	app_shader_token_binding = 1 << app_shader_binding,
} app_shader_token_flags;

typedef enum app_shader_semantic_type
{
	app_shader_semantic_end_of_source = 0,
	app_shader_semantic_unknown = ~0,
	app_shader_semantic_brace_block = app_shader_token_brace_open | app_shader_token_block | app_shader_token_brace_close,
	app_shader_semantic_struct_declaration = app_shader_token_struct | app_shader_token_identifier,

	//
	app_shader_semantic_struct_definition =
		app_shader_semantic_struct_declaration | app_shader_token_brace_open | app_shader_token_brace_close,

	app_shader_semantic_layout = app_shader_token_layout | app_shader_token_paren_open | app_shader_token_paren_close,

	//
	app_shader_semantic_uniform_definition =
		app_shader_token_uniform | app_shader_token_identifier | app_shader_token_brace_open | app_shader_token_brace_close,
} app_shader_semantic_type;

// TODO: Maybe kind?
// TODO: Find out a way how to deal with layout qualifiers! layout(binding = 4) ... (Maybe a mapping?)
typedef struct app_shader_source_token
{
	app_shader_token type;
	const char *source;
	fckc_size_t length;
} app_shader_source_token;

typedef struct app_shader_source_token_reference
{
	app_shader_source_token *value;
} app_shader_source_token_reference;

typedef struct app_shader_source_token_reference_list
{
	app_shader_source_token_reference *values;
	fckc_size_t count;
} app_shader_source_token_reference_list;

typedef struct app_shader_source
{
	app_shader_source_token *tokens;
	fckc_size_t count;

	// TODO: Compute index?
	app_shader_source_token_reference_list references[app_shader_count];
	// TODO: Addsome pointer to pointer magic
	// Or maybe intrusive shit, idk yet
} app_shader_source;

static fckc_size_t app_bitcnt(fckc_u32 n)
{
	n = n - ((n >> 1) & 0x55555555);
	n = (n & 0x33333333) + ((n >> 2) & 0x33333333);
	n = (n + (n >> 4)) & 0x0F0F0F0F;
	n = n + (n >> 8);
	n = n + (n >> 16);
	return (fckc_size_t)(n & 0x3F);
}

static fckc_size_t app_scope(const app_shader_source *source, fckc_size_t current, app_shader_token close)
{
	const app_shader_source_token token = source->tokens[current];

	fckc_size_t skipped = 0;

	for (;;)
	{
		const fckc_size_t next = current + skipped + 1;
		const app_shader_source_token next_token = source->tokens[next];
		if (next >= source->count)
		{
			// Closed by end - will not compile anyway.
			return next;
		}
		if (next_token.type == close)
		{
			return next;
		}
		skipped = skipped + 1;
	}
}

typedef struct app_shader_syntax // (?)
{
	// Hmmmm
	// In between [current:next] we can find member declarations, arguments, and so on
	const app_shader_source_token *current;
	const app_shader_source_token *next;

	app_shader_semantic_type type;
	// Idk if this space is enough
	char identifier[256];
} app_shader_syntax;

typedef struct app_shader_member_syntax
{
	const app_shader_source_token *type;
	const app_shader_source_token *next;
} app_shader_member_syntax;

static void app_shader_declaration(app_shader_syntax *syntax)
{
}

static app_shader_semantic_type app_shader_semantic_match(const app_shader_source *source, app_shader_syntax *syntax)
{
	fckc_size_t token_count = 0;

	if (syntax->current == NULL)
	{
		syntax->current = source->tokens;
	}
	else
	{
		syntax->current = syntax->next;
		fck_assert(syntax->current != NULL);
		fck_assert(syntax->next != NULL);
	}

	const fckc_size_t current_token_index = syntax->current - source->tokens;

	app_shader_semantic_type semantic = 0;
	{
		const app_shader_source_token token = source->tokens[current_token_index];
		if (token.type == app_shader_end_of_source)
		{
			return app_shader_semantic_end_of_source;
		}

		// We test the waters
		// We start with testing a struct and then use the largest count
		// If that does not apply, we go with other grammars
		// Grammars should be mututally exclusive, though
		// layout can exist with uniform, with readonly buffer and so on
		// These extra attributes should get accounted for(?)
		switch (token.type)
		{
		case app_shader_struct:
			token_count = app_bitcnt(app_shader_semantic_struct_definition);
			break;
		case app_shader_layout:
			token_count = app_bitcnt(app_shader_semantic_layout);
			break;
		case app_shader_uniform:
			token_count = app_bitcnt(app_shader_semantic_uniform_definition);
			break;
		default:
			break;
		}
		semantic = 1 << token.type;
	}

	if (current_token_index + token_count > source->count)
	{
		return app_shader_semantic_end_of_source;
	}

	// Construct a bitmask to get closer to a semantic type
	fckc_size_t offset = current_token_index + 1;
	for (fckc_size_t i = 1; i < token_count; i++)
	{
		const fckc_size_t current = offset;
		const app_shader_source_token token = source->tokens[current];
		semantic = semantic | (1 << token.type);

		// Down here we want to create a "scope"
		// The content of the scope are opaque for semantic purpose
		// The content of the scope do not really matter cause it can be anything
		// open | close -> should mean a content is optional
		// open | body | close -> should mean content is not optional?
		fckc_size_t next = offset + 1;
		switch (token.type)
		{
		case app_shader_brace_open:
			next = app_scope(source, current, app_shader_brace_close);
			break;
		case app_shader_paren_open:
			next = app_scope(source, current, app_shader_paren_close);
			break;
		case app_shader_bracket_open:
			next = app_scope(source, current, app_shader_bracket_close);
			break;
		default:
			break;
		}

		if (next >= source->count)
		{
			return app_shader_semantic_end_of_source;
		}
		offset = next;
	}

	syntax->type = semantic;
	syntax->identifier[0] = '\0';
	syntax->next = source->tokens + offset;

	const app_shader_source_token *identifier;

	switch (semantic)
	{
	case app_shader_semantic_layout:
		return app_shader_semantic_layout;
	case app_shader_semantic_struct_definition:
	case app_shader_semantic_uniform_definition:
		// We find the identifier after the struct keyword
		identifier = &source->tokens[current_token_index + 1];
		memcpy(syntax->identifier, identifier->source, identifier->length);
		syntax->identifier[identifier->length] = '\0';
		return semantic;
	default:
		break;
	}
	return app_shader_semantic_unknown;
}

static const char *app_shader_source_skip_space(const char *str)
{
	for (;;)
	{
		if (*str == '\0')
		{
			return str;
		}
		if (!isspace(*str))
		{
			return str;
		}
		str = str + 1;
	}
}

static const char *app_shader_source_skip_until_non_number(const char *str)
{
	for (;;)
	{
		if (*str == '\0')
		{
			return str;
		}
		if (!isdigit(*str))
		{
			if (*str != '.')
			{
				return str;
			}
		}
		str = str + 1;
	}
}

static int isc_identifier(char c)
{
	return isalnum(c) || c == '_';
}

static const char *app_shader_source_skip_until_non_identifier(const char *str)
{
	for (;;)
	{
		if (*str == '\0')
		{
			return str;
		}
		if (!isalnum(*str))
		{
			if (*str != '_')
			{
				return str;
			}
		}
		str = str + 1;
	}
}

// name bad
static const char *app_shader_source_text_equals(const char *str, const char *end, const char *other)
{
	// Excludes null-terminator!
	const fckc_size_t len = strlen(other);
	const fckc_size_t distance = (fckc_size_t)end - (fckc_size_t)str;

	if (distance < len)
	{
		// This should be correct
		return NULL;
	}

	for (fckc_size_t index = 0; index < len; index++)
	{
		const char lc = str[index];
		const char rc = other[index];
		if (str + index >= end)
		{
			return NULL;
		}

		if (lc != rc)
		{
			return NULL;
		}
	}
	// Meh, it just returns the equal part's end
	return str + len;
}

static const char *app_shader_source_parse_keyword(const char *current, const char *next, const char *keyword)
{
	// We are guaranteed to hit a new token, cool, cool
	const char *end_of_keyword = app_shader_source_text_equals(current, next, keyword);
	if (end_of_keyword)
	{
		// Null-terminator got our back - We look at the next one
		if (!isc_identifier(*end_of_keyword))
		{
			return end_of_keyword;
		}
	}
	return NULL;
}

static const char *app_shader_source_token_next(const char *current, app_shader_source_token *out_token)
{
	typedef struct app_shader_source_keywords_entry
	{
		const char *keyword;
		app_shader_token token;
	} app_shader_source_keywords_entry;

	// TODO: Make it some little hash lookup instead! :)
	const static app_shader_source_keywords_entry keywords[] = {
		{"struct", app_shader_struct},     //
		{"float", app_shader_type},        //
		{"layout", app_shader_layout},     //
		{"uniform", app_shader_uniform},   //
		{"readonly", app_shader_readonly}, //
		{"shared", app_shader_shared},     //
		{"buffer", app_shader_buffer},     //
	};

	current = app_shader_source_skip_space(current);
	// Maybe doing it more per char is a bit better
	out_token->type = app_shader_unknown;
	out_token->source = current;
	out_token->length = 1;

	if (*current == '_' || isalpha(*current))
	{
		// idenfitiers cannot start with numbers
		// Identifier-ish. Probably, idk
		const char *next = app_shader_source_skip_until_non_identifier(current + 1);

		out_token->type = app_shader_identifier;
		out_token->length = (fckc_size_t)next - (fckc_size_t)current;

		for (fckc_size_t index = 0; index < fck_arraysize(keywords); index++)
		{
			const app_shader_source_keywords_entry *entry = keywords + index;
			const char *result = app_shader_source_parse_keyword(current, next, entry->keyword);
			if (result)
			{
				// Beyond the keyword, we need to check if it is non identifier text
				// Else we might be looking at something else. I.e., floattest <- see
				// This is a float keyword!
				out_token->type = entry->token;
				out_token->length = (fckc_size_t)next - (fckc_size_t)current;
				break;
			}
		}
	}
	else if (isdigit(*current))
	{
		const char *next = app_shader_source_skip_until_non_number(current);
		out_token->type = app_shader_constant;
		out_token->length = (fckc_size_t)next - (fckc_size_t)current;
	}
	else if (ispunct(*current))
	{
		out_token->length = 1;
		switch (*current)
		{
		case '{':
			out_token->type = app_shader_brace_open;
			break;
		case '}':
			out_token->type = app_shader_brace_close;
			break;
		case '(':
			out_token->type = app_shader_paren_open;
			break;
		case ')':
			out_token->type = app_shader_paren_close;
			break;
		case '[':
			out_token->type = app_shader_bracket_open;
			break;
		case ']':
			out_token->type = app_shader_bracket_close;
			break;
		case ';':
			out_token->type = app_shader_semicolon;
			break;
		default:
			out_token->type = app_shader_operator;
			break;
		}
	}
	else if (*current == '\0')
	{
		// If we fuck up, we just spin on this one. I do not really care tbh
		out_token->type = app_shader_end_of_source;
		out_token->length = 0;
	}
	return out_token->source + out_token->length;
}

static void app_shader_source_parse(const char *text, app_shader_source *source)
{
	// shared keyword only valid in compute shaders
	// buffer keyword
	const char *current = text;

	// Set this whole bad boy to 0
	memset(source->references, 0, sizeof(source->references));
	while (*current)
	{
		app_shader_source_token dummy;
		current = app_shader_source_token_next(current, &dummy);
		source->references[dummy.type].count = source->references[dummy.type].count + 1;
	}

	source->count = 0;
	for (fckc_size_t index = 0; index < fck_arraysize(source->references); index++)
	{
		app_shader_source_token_reference_list *list = source->references + index;
		list->values = (app_shader_source_token_reference *)kll_malloc(kll_system, list->count * sizeof(*list->values));
		source->count = source->count + list->count;
	}

	source->tokens = (app_shader_source_token *)kll_malloc(kll_system, source->count * sizeof(*source->tokens));

	// Restart that bad boy since we allocated memory
	current = text;

	fckc_size_t token_entry_index[fck_arraysize(source->references)] = {0};

	fckc_size_t token_index = 0;
	while (*current)
	{
		app_shader_source_token *token = source->tokens + token_index;
		current = app_shader_source_token_next(current, token);

		app_shader_source_token_reference_list *list = source->references + token->type;
		app_shader_source_token_reference *reference = list->values + token_entry_index[token->type];
		reference->value = token;

		token_entry_index[token->type] = token_entry_index[token->type] + 1;
		token_index = token_index + 1;
	}
}

static void app_parse_variables(const char *open, const char *close)
{
	const char *current = open + 1;
	while (current < close)
	{
		const char *semicolon = strchr(current, ';');
		if (semicolon > close)
		{
			return;
		}
		current = semicolon + 1;
	}
}

static void app_parse_shader_properties(const char *source)
{
	const char layout_token[] = "layout";
	const char binding_token[] = "binding";
	const char uniform_token[] = "uniform";

	const char *eos = source + strlen(source);
	const char *current = source;
	while (current < eos)
	{
		const char *layout = strstr(current, layout_token);
		if (layout == NULL)
		{
			// No layout anywhere.
			return;
		}

		const char *preempt_semicolon = strchr(layout, ';');
		if (preempt_semicolon == NULL)
		{
			// No semicolon left - Broken
			return;
		}

		const char *binding_open = strchr(layout + sizeof(layout_token), '(');
		if (binding_open == NULL)
		{
			// No '(' left... Non-sense to keep going
			return;
		}
		const char *binding_close = strchr(binding_open + 1, ')');
		if (binding_close == NULL)
		{
			// No '(' left... Non-sense to keep going
			return;
		}
		current = binding_close + 1;

		const char *binding = strstr(binding_open, binding_token);
		if (binding <= binding_open || binding > binding_close)
		{
			continue;
		}
		const char *binding_assignmet = strchr(binding + sizeof(binding_token), '=');
		if (binding_assignmet <= binding_open || binding_assignmet > binding_close)
		{
			continue;
		}
		unsigned long binding_index = to_i32(~0LLU);
		const char *binding_index_end = app_parse_number(binding_assignmet, binding_close, &binding_index);

		// We now know we are in a binding block. Let's take a pivot to an end. This is just any end
		const char *uniform = strstr(binding_index_end, uniform_token);
		if (uniform == NULL || uniform > preempt_semicolon)
		{
			continue;
		}

		const char *block_open = strchr(uniform + sizeof(uniform_token), '{');
		if (block_open == NULL || block_open > preempt_semicolon)
		{
			continue;
		}
		const char *block_close = strchr(block_open + 1, '}');
		if (block_close == NULL)
		{
			// The match did absolutely fail
			return;
		}

		// Scan and save eveyrthing within this blocK!
		app_parse_variables(block_open, block_close);

		const char *semicolon = strchr(block_close + 1, ';');
	}
}

static void app_graphics_create(app_graphics *pipelines, app_graphics_shape primitive, app_graphics_style material, const char *vertex,
                                const char *fragment)
{
	fck_shader_compiler compiler = pipelines->shader->create();
	if (!pipelines->shader->is_ok(compiler))
	{
		return;
	}

	app_graphic_pipeline *gfx = &pipelines->values[primitive][material];

	fck_file vert_file = os->fs->open(vertex, "r");
	fck_shader_desc vert_desc = (fck_shader_desc){FCK_SHADER_VERTEX, app_graphics_shape_to_string(primitive), "main"};

	fck_file frag_file = os->fs->open(fragment, "r");
	fck_shader_desc frag_desc = (fck_shader_desc){FCK_SHADER_FRAGMENT, app_graphic_material_to_string(material), "main"};
	fck_glsl_object vert = {0};
	fck_glsl_object frag = {0};
	vert = compiler.create_glsl_from_file(&compiler, &vert_desc, &vert_file);
	frag = compiler.create_glsl_from_file(&compiler, &frag_desc, &frag_file);

	app_shader_source shader_source = {0};
	app_shader_source_parse(vert.generic.source, &shader_source);

	app_shader_syntax syntax = {0};
	while (app_shader_semantic_match(&shader_source, &syntax))
	{
		if (syntax.type == app_shader_semantic_struct_definition)
		{
			os->io->log("struct %s", syntax.identifier);
		}
		if (syntax.type == app_shader_semantic_uniform_definition)
		{
			os->io->log("uniform %s", syntax.identifier);
		}
	}

	// for (fckc_size_t index = 0; index < shader_source.count; index++)
	//{
	//	const app_shader_source_token token = shader_source.tokens[index];
	//	if (token.type == app_shader_layout)
	//	{
	//		const app_shader_source_token paren_open = shader_source.tokens[index + 1];

	//		os->io->log("%s", token.source);
	//	}
	//	else if (token.type == app_shader_struct)
	//	{
	//		app_shader_syntax syntax = {0};
	//		app_shader_semantic_type semantic_type = app_shader_semantic_match(&shader_source, &syntax);
	//		(void)semantic_type;
	//	}
	//}

	// TODO: Rules
	// paren open and paren close count need to be the same
	// block open and block close count need to be the same
	//
	// app_parse_shader_properties(frag.generic.source);

	sht_binding const *bindings;
	const fckc_size_t binding_count = app_graphic_pipeline_bindings(primitive, material, &bindings);

	sht_binding_desc binding_desc = {.bindings = bindings, .count = binding_count};
	gfx->bss = pipelines->driver.vt->bss->create(pipelines->driver, &binding_desc);

	sht_vertex_desc vertex_desc = {
		.stride = 0,
		.bindings = NULL,
		.count = 0,
	};
	const sht_raster_desc raster_desc = {
		.cull_mode = SHT_CULL_MODE_NONE,
		.topology = SHT_TRIANGLE_LIST,
		.color = SHT_FORMAT_B8G8R8A8_UNORM,
		.depth = SHT_FORMAT_UNDEFINED,
	};
	sht_graphic_desc graphic_desc = {
		.fragment = &frag.generic,
		.vertex = &vert.generic,
		.vertex_desc = &vertex_desc,
		.raster = raster_desc,
	};

	os->fs->close(vert_file);
	os->fs->close(frag_file);

	gfx->pipeline = pipelines->driver.vt->graphics_pipeline->create(pipelines->driver, gfx->bss, &graphic_desc);
	compiler.destroy(&compiler, &vert.generic);
	compiler.destroy(&compiler, &frag.generic);
	compiler.shutdown(&compiler);

	const fckc_size_t capacity = 64;
	gfx->transforms = (app_shape_transform *)kll_malloc(kll_system, sizeof(*gfx->transforms) * capacity);
	gfx->capacity = capacity;
	gfx->count = 0;
}

static void app_graphics_add_line(app_graphics *graphics, app_graphics_style material, float sx, float sy, float ex, float ey,
                                  float thickness)
{
	app_graphic_pipeline *g = &graphics->values[app_shape_line][material];
	fck_assert(g->count < g->capacity);

	const app_line_transform init_transform = {
		.sx = sx,
		.sy = sy,
		.ex = ex,
		.ey = ey,
		.z = 0.0f,
		.thickness = thickness,
		.scale = 1.0f,
	};

	app_shape_transform *transform = g->transforms + g->count;
	transform->line = init_transform;
	g->count = g->count + 1;
}

static void app_graphics_add_quad(app_graphics *graphics, app_graphics_style material, float x, float y)
{
	app_graphic_pipeline *g = &graphics->values[app_shape_quad][material];
	fck_assert(g->count < g->capacity);

	const app_quad_transform init_transform = {
		.x = x,
		.y = y,
		.z = 0.0f,
		.rotation = 0.0f,
		.width = 100.0f,
		.height = 100.0f,
		.scale = 1.0f,
	};
	app_shape_transform *transform = g->transforms + g->count;
	transform->quad = init_transform;
	g->count = g->count + 1;
}

int main(int argc, char **argv)
{
	load_config(argc, argv);

	purge_files("temp-*.dll");

	// app_properties properties = {0};
	// app_property_structure_set_float(&properties, "r", 0.0f);
	// app_property_structure_set_float(&properties, "g", 0.0f);
	// app_property_structure_set_float(&properties, "b", 0.0f);
	// app_property_structure_set_float(&properties, "a", 0.0f);
	// app_property_structure_set_float(&properties, "roundness", 0.0f);

	fck_api_registry *registry = fck_api_registry_load("fck-api.dll");
	fck_plugins_api *plugins = fck_plugins_load(registry, "fck-plugins.dll");

	plugins->root(os->fs->executable());

	const char *current = NULL;
	while ((current = plugins->unloaded(current)))
	{
		plugins->load(current);
	}

	current = NULL;
	while ((current = plugins->loaded(current)))
	{
		plugins->load(current);
	}

	fck_input *input = (fck_input *)registry->find(fck_input_api_name);
	sht_render_api *render = (sht_render_api *)registry->find(sht_render_api_name);
	fck_shader_api *shader = (fck_shader_api *)registry->find(fck_shader_api_name);

	fck_window window = os->win->create("Test", 1280, 720);

	const sht_instance instance = render->load(sht_header_version);
	if (!render->is_ok(instance))
	{
		return 0;
	}

	sht_driver driver = instance.vt->start(instance, &window);
	if (!instance.vt->is_ok(driver))
	{
		return 0;
	}

	sht_graphics_pipeline graphic_pipelines;

	sht_memory *memory = driver.vt->memory(driver);
	sht_swapchain swapchain = driver.vt->swapchain(driver);
	sht_command_buffer_vt *command = driver.vt->command_buffer;

	sht_elements indices = {0};

	sht_sampler sampler = {0};
	sht_image texture_image = {0};
	sht_image_view texture_view = {0};

	{
		sampler = driver.vt->create_sampler(driver, sht_filter_linear);
		texture_image = memory->image->create(memory->bump,
		                                      &(sht_image_configuration){
												  .format = SHT_FORMAT_R8G8B8A8_UNORM,
												  .width = 4,
												  .height = 1,
												  .transfer = SHT_TRANSFER_TARGET,
												  .usage = SHT_IMAGE_USAGE_SAMPLED,
											  },
		                                      SHT_MEMORY_GPU);
		texture_view = memory->image->view(memory->bump, texture_image, SHT_FORMAT_R8G8B8A8_UNORM);

		fckc_u32 pixels[] = {0xFF0000FF, 0xFF00FF00, 0xFFFF0000, 0xFFFFFFFF};
		driver.vt->upload_image(driver, &texture_image, pixels, sizeof(pixels));
	}

	{
		fckc_u32 index_data[] = {0, 1, 2, 1, 3, 2};
		indices.count = fck_arraysize(index_data);
		indices.buffer = memory->malloc(memory->bump, &sht_buffer_target(SHT_BUFFER_USAGE_INDEX, sizeof(index_data)), SHT_MEMORY_GPU);
		driver.vt->upload_buffer(driver, &indices.buffer, index_data, sizeof(index_data));
	}

	app_graphics graphics = {0};
	app_graphics_init(&graphics, shader, driver);
	app_graphics_create(&graphics, app_shape_quad, app_style_solid, fck_resource_path "quad.vert", fck_resource_path "solid.frag");
	app_graphics_create(&graphics, app_shape_quad, app_style_textured, fck_resource_path "quad.vert", fck_resource_path "textured.frag");
	app_graphics_create(&graphics, app_shape_quad, app_style_rounded, fck_resource_path "quad.vert", fck_resource_path "round.frag");
	app_graphics_create(&graphics, app_shape_line, app_style_solid, fck_resource_path "line.vert", fck_resource_path "solid.frag");
	app_graphics_create(&graphics, app_shape_line, app_style_textured, fck_resource_path "line.vert", fck_resource_path "textured.frag");
	app_graphics_create(&graphics, app_shape_line, app_style_rounded, fck_resource_path "line.vert", fck_resource_path "round.frag");

	app_graphics_add_quad(&graphics, app_style_textured, 0.0f, 0.0f);
	app_graphics_add_quad(&graphics, app_style_solid, 300.0f, 0.0f);
	app_graphics_add_quad(&graphics, app_style_rounded, 500.0f, 0.0f);

	app_graphics_add_line(&graphics, app_style_solid, -50.0f, -50.0f, 50.0f, 50.0f, 16.0f);
	app_graphics_add_line(&graphics, app_style_solid, 50.0f, -50.0f, -50.0f, 50.0f, 16.0f);

	app_graphics_add_line(&graphics, app_style_textured, -300.0f, 0.0f, -350.0f, 100.0f, 24.0f);
	app_graphics_add_line(&graphics, app_style_rounded, -500.0f, 0.0f, -550.0f, 100.0f, 28.0f);

	// How do I scale this to vertex and fragment shader stuff...
	// HMMMMMMM
	for (fckc_size_t index = 0; index < app_shape_count; index++)
	{
		app_properties *properties = &graphics.values[index][app_style_rounded].properties;
		app_property_structure_set_float(properties, "roundness", 0.5f);
	}

	int is_running = 1;
	while (is_running)
	{
		// TODO: Make render-vk hotreloadable :)
		// How hard can it be?
		plugins->hotreload();

		fck_input_event events[32] = {0};
		const fckc_size_t result = input->events(events, fck_arraysize(events));
		for (fckc_size_t index = 0; index < result; index++)
		{
			fck_input_event *e = events + index;
			if (e->source->type == fck_input_source_keyboard)
			{
				if (e->description->id == fck_pkey_escape)
				{
					is_running = 0;
				}
			}
			if (e->source->type == fck_input_source_mouse)
			{
				if (e->description->id == fck_mouse_left)
				{
				}
			}
		}

		memory->reset(memory->temp);

		fckc_u32 frame_index;
		const sht_image_view color_target = swapchain.vt->wait_and_acquire(swapchain, &frame_index);
		if (swapchain.vt->is_ok(swapchain, frame_index))
		{
			sht_extent extent = swapchain.vt->extent(swapchain);

			const sht_command_buffer command_buffer = command->acquire(driver, frame_index);
			if (command->is_ok(command_buffer))
			{
				sht_render_desc desc = {
					.colour =
						{
							.view = color_target,
							.load_op = SHT_CLEAR,
							.store_op = SHT_STORE,
							.clear_value = {0.0f, 0.0f, 0.2f, 1.0f},
						},
				};

				app_screen screen = {
					.width = (float)extent.width,
					.height = (float)extent.height,
				};

				const sht_render_pass render_pass = command->render_pass->begin(command_buffer, &desc);

				sht_viewport viewport;
				viewport.offset.x = 0.0f;
				viewport.offset.y = 0.0f;
				viewport.depth.min = (float)0.0f;
				viewport.depth.max = (float)1.0f;

				sht_scissor scissor;
				scissor.offset.x = 0;
				scissor.offset.y = 0;
				scissor.extent = viewport.extent = swapchain.vt->extent(swapchain);

				if (command->render_pass->is_ok(render_pass))
				{
					command->viewport(command_buffer, &viewport);
					command->scissor(command_buffer, &scissor);

					command->index_buffer(command_buffer, &indices.buffer, 0);

					for (fckc_size_t shape_index = 0; shape_index < app_shape_count; shape_index++)
					{
						for (fckc_size_t style_index = 0; style_index < app_style_count; style_index++)
						{
							app_graphic_pipeline *graphic = &graphics.values[shape_index][style_index];
							if (driver.vt->graphics_pipeline->is_ok(graphic->pipeline))
							{
								if (graphic->count > 0)
								{
									const sht_buffer_upload_desc screen_upload = {
										.data = &screen,
										.size = sizeof(screen),
										.count = 1,
									};
									const sht_buffer_upload_desc quads_upload = {
										.data = graphic->transforms,
										.size = sizeof(*graphic->transforms),
										.count = graphic->count,
									};

									app_config config = {.gradient = 0, .is_sdf = style_index == app_style_rounded};

									const sht_buffer_upload_desc config_upload = {
										.data = &config,
										.size = sizeof(config),
										.count = 1,
									};
									const sht_image_upload_desc image_upload = {
										.views = texture_view,
										.samplers = sampler,
									};

									driver.vt->bss->upload_buffer(graphic->bss, 0, &screen_upload);
									driver.vt->bss->upload_buffer(graphic->bss, 1, &quads_upload);
									driver.vt->bss->upload_buffer(graphic->bss, 2, &config_upload);
									driver.vt->bss->upload_image(graphic->bss, 3, &image_upload);

									app_property_structure_upload(&graphic->properties, driver, graphic->bss);

									command->bss(command_buffer, graphic->bss);

									command->graphics_pipeline(command_buffer, graphic->pipeline);

									sht_draw_indexed_desc indexed = {
										.first_index = 0,
										.first_instance = 0,
										.index_count = to_u32(indices.count),
										.instance_count = graphic->count,
										.vertex_offset = 0,
									};
									command->draw_indexed(command_buffer, &indexed);
								}
							}
						}
					}

					command->render_pass->end(command_buffer);
				}
				command->submit(command_buffer, SHT_QUEUE_GRAPHIC);
			}
		}
	}

	plugins->shutdown();
	purge_files("temp-*.dll");

	os->win->destroy(window);

	return 0;
}
