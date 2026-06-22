// TODO: Rename this to something better
#ifndef FCK_REFLECTION_H_INCLUDED
#define FCK_REFLECTION_H_INCLUDED

#define fck_glsl_reflection_api_name "fck_shader"
#define fck_glsl_reflection_global ""

typedef enum fck_glsl_reflection_declaration_qualifier
{
	fck_glsl_reflection_declaration_qualifier_none = 0,
	fck_glsl_reflection_declaration_qualifier_uniform = 1 << 0,
	fck_glsl_reflection_declaration_qualifier_buffer = 1 << 1,
	fck_glsl_reflection_declaration_qualifier_shared = 1 << 2,
	fck_glsl_reflection_declaration_qualifier_readonly = 1 << 3,
	fck_glsl_reflection_declaration_qualifier_in = 1 << 4,
	fck_glsl_reflection_declaration_qualifier_out = 1 << 5,

	fck_glsl_reflection_declaration_qualifier_interface_mask = fck_glsl_reflection_declaration_qualifier_uniform | fck_glsl_reflection_declaration_qualifier_buffer |
	fck_glsl_reflection_declaration_qualifier_shared | fck_glsl_reflection_declaration_qualifier_in |
	fck_glsl_reflection_declaration_qualifier_out,
	// ...
} fck_glsl_reflection_declaration_qualifier;

struct fck_glsl_reflection_type_field;
typedef struct fck_glsl_reflection_type
{
	const char* name;
	// Having this one here might be overkill
	// We should collect the bindings somehow else!
	// This way we can also react to the scoped and unscoped interface blocks
	int binding;

	fck_glsl_reflection_declaration_qualifier qualifiers;
	const struct fck_glsl_reflection_variable* first;
} fck_glsl_reflection_type;

typedef struct fck_glsl_reflection_variable
{
	const fck_glsl_reflection_type* type;
	const char* name;
	const struct fck_glsl_reflection_variable* next;
} fck_glsl_reflection_variable;

struct fck_glsl_reflection;
typedef struct fck_glsl_reflection_api
{
	struct fck_glsl_reflection* (*reflect)(const char* source, const char* global);
	const fck_glsl_reflection_type* (*type_of)(struct fck_glsl_reflection* reflection, const char* name);
	void (*free)(struct fck_glsl_reflection* reflection);
}fck_glsl_reflection_api;

extern fck_glsl_reflection_api* glsl_reflection;

#endif // !FCK_REFLECTION_H_INCLUDED
