#ifndef FCK_SHADER_H_INCLUDED
#define FCK_SHADER_H_INCLUDED

#include <fckc_inttypes.h>

#define fck_shader_api_name "fck_shader"

struct fck_file;

typedef enum fck_shader_language
{
	fck_shader_none,
	fck_shader_glsl,
	fck_shader_hlsl,
	fck_shader_spirv,
} fck_shader_language;

typedef enum fck_shader_stage_type
{
	fck_shader_unkown = 0,
	fck_shader_vertex = 1,
	fck_shader_fragment = 2,
	fck_shader_compute = 3,

	// For whoever prefers that
	fck_shader_pixel = fck_shader_fragment,
} fck_shader_stage_type;

typedef struct fck_shader_desc
{
	fck_alias(fck_shader_stage_type, fckc_u32) type;

	// idk if all as null-terminated strings is wise... we will see!
	const char *file;
	const char *entry_point;
} fck_shader_desc;

typedef struct fck_shader_generic
{
	fck_shader_desc desc;
	fck_alias(fck_shader_language, fckc_u32) language;

	const char *source;
	fckc_size_t souce_byte_size;
} fck_shader_generic;

typedef struct fck_spirv_object
{
	fck_shader_generic generic;
} fck_spirv_object;

typedef struct fck_glsl_object
{
	fck_shader_generic generic;
} fck_glsl_object;

typedef struct fck_hlsl_object
{
	fck_shader_generic generic;
} fck_hlsl_object;

struct fck_file;
struct fck_shader_compiler;

typedef struct fck_shader_compiler
{
	void *handle;
	void (*shutdown)(struct fck_shader_compiler *compiler);

	fck_spirv_object (*create_spirv)(struct fck_shader_compiler *compiler, fck_shader_generic *shader);
	fck_hlsl_object (*create_hlsl)(struct fck_shader_compiler *compiler, fck_shader_desc *desc, const char *source);
	fck_glsl_object (*create_glsl)(struct fck_shader_compiler *compiler, fck_shader_desc *desc, const char *source);
	void (*destroy)(struct fck_shader_compiler *compiler, fck_shader_generic *shader);

	fck_hlsl_object (*create_hlsl_from_file)(struct fck_shader_compiler *compiler, fck_shader_desc *desc, struct fck_file *file);
	fck_glsl_object (*create_glsl_from_file)(struct fck_shader_compiler *compiler, fck_shader_desc *desc, struct fck_file *file);

	// TODO: Either we are stubborn and say "you need at least ONE compiler to understand a shader object"
	// or we redesign this API :)
	// Why the fuck are the parms pointers?
	fck_shader_stage_type (*type)(fck_shader_generic *shader);
	fck_shader_language (*language)(fck_shader_generic *shader);
	const char *(*file)(fck_shader_generic *shader);
	const char *(*entry_point)(fck_shader_generic *shader);
	const void *(*source)(fck_shader_generic *shader);
	fckc_size_t (*size)(fck_shader_generic *shader);
} fck_shader_compiler;

// TODO: This api is a bit rubbish... We should include shader::destroy in the shader_api
// Same applies to all the getters that do not need the compiler!!

typedef struct fck_shader_api
{
	fck_shader_compiler (*create)(void);
	int (*is_ok)(fck_shader_compiler compiler);
} fck_shader_api;

#endif
