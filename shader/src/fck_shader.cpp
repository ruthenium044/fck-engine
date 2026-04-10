// #include <dxc/WinAdapter.h>

#include <stdio.h>
extern "C"
{
#define FCK_SHADER_EXPORT
#include "fck_shader.h"
#include <fck_os.h>
#include <fckc_assert.h>
#include <kll.h>
#include <kll_heap.h>
#include <kll_malloc.h>
}
#include <shaderc/shaderc.h>
#include <stdlib.h>
#include <string.h>

// We use malloc for now cause lazy!

fck_shader_generic fck_shader_create_generic(struct fck_shader_compiler *compiler, uint32_t lang, fck_shader_desc *desc, void const *source,
                                             fckc_size_t source_size)
{
	fck_shader_generic generic;
	memset(&generic, 0xCF, sizeof(generic));
	generic.language = lang;
	generic.desc.type = desc->type;

	fckc_size_t source_len = source_size; // No need for alignment cause first!
	fckc_size_t file_len = strlen(desc->file);
	fckc_size_t ep_len = strlen(desc->entry_point);
	const fckc_size_t terminator_count = 4;
	fckc_size_t total_len = file_len + ep_len + source_len + terminator_count;

	char *total = (char *)malloc(total_len);
	memset(total, 0, total_len);
	char *dst = total;
	generic.source = dst = ((char *)memcpy(dst, source, source_len));
	dst = ((char *)memset(dst + source_len + 1, 0, 1));
	generic.desc.entry_point = dst = ((char *)memcpy(dst, desc->entry_point, ep_len));
	dst = ((char *)memset(dst + ep_len + 1, 0, 1));
	generic.desc.file = dst = ((char *)memcpy(dst, desc->file, file_len));
	dst = ((char *)memset(dst + file_len + 1, 0, 1));
	generic.souce_byte_size = source_size;
	return generic;
}

fck_spirv_object fck_shader_create_spirv(struct fck_shader_compiler *compiler, fck_shader_generic *shader)
{
	// MAPS ONE TO ONE FOR NOW! THIS CAN BREAK!
	shaderc_shader_kind shader_kind = (shaderc_shader_kind)(fck_shader_type)shader->desc.type;
	if (shader->language == FCK_SHADER_SPIRV)
	{
		return (fck_spirv_object){};
	}
	// TODO: Make this pretty :)
	if (shader->language == FCK_SHADER_GLSL || shader->language == FCK_SHADER_HLSL)
	{
		shaderc_compiler_t shaderc = (shaderc_compiler_t)compiler->handle;
		shaderc_compilation_result_t result;

		shaderc_compile_options_t options = shaderc_compile_options_initialize();
		shaderc_source_language lang;
		switch (shader->language)
		{
		case FCK_SHADER_GLSL:
			lang = shaderc_source_language_glsl;
			break;
		case FCK_SHADER_HLSL:
			lang = shaderc_source_language_hlsl;
			break;
		}

		{
			const char *source = shader->source;
			const char *file = shader->desc.file;
			const char *entry_point = shader->desc.entry_point;
			fckc_size_t byte_size = shader->souce_byte_size;
			shaderc_compile_options_set_source_language(options, lang);
			result = shaderc_compile_into_spv(shaderc, source, byte_size, shader_kind, file, entry_point, options);
		}

		shaderc_compile_options_release(options);
		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success)
		{
			const char *error = shaderc_result_get_error_message(result);
			printf("%s", error);
			return (fck_spirv_object){};
		}

		size_t size = shaderc_result_get_length(result);
		const char *source = shaderc_result_get_bytes(result);

		fck_spirv_object spirv;
		spirv.generic = fck_shader_create_generic(compiler, FCK_SHADER_SPIRV, &shader->desc, source, size);
		shaderc_result_release(result);
		return spirv;
	}

	return (fck_spirv_object){};
}

fck_glsl_object fck_shader_create_glsl(struct fck_shader_compiler *compiler, fck_shader_desc *desc, const char *source)
{
	fck_glsl_object glsl;
	glsl.generic = fck_shader_create_generic(compiler, FCK_SHADER_GLSL, desc, source, strlen(source));
	return glsl;
}

fck_hlsl_object fck_shader_create_hlsl(struct fck_shader_compiler *compiler, fck_shader_desc *desc, const char *source)
{
	fck_hlsl_object hlsl;
	hlsl.generic = fck_shader_create_generic(compiler, FCK_SHADER_HLSL, desc, source, strlen(source));
	return hlsl;
}

fck_hlsl_object fck_shader_create_hlsl_from_file(struct fck_shader_compiler *compiler, fck_shader_desc *desc, fck_file *file)
{
	fckc_size_t size = os->fs->size(*file);
	char *text = (char *)kll_malloc(kll_heap, size);
	fckc_size_t read = os->fs->read(*file, text, size);
	fck_assert(size == read);
	text[read] = '\0'; 

	fck_hlsl_object hlsl = compiler->create_hlsl(compiler, desc, text);
	return hlsl;
}

void fck_shader_destroy(struct fck_shader_compiler *compiler, fck_shader_generic *shader)
{
	free((void *)shader->source);
}

void fck_shader_compiler_shutdown(fck_shader_compiler *compiler)
{
	shaderc_compiler_t shaderc = (shaderc_compiler_t)compiler->handle;
	shaderc_compiler_release(shaderc);
}

fck_shader_type fck_shader_compiler_type(fck_shader_generic *shader)
{
	return (fck_shader_type)shader->desc.type;
}
fck_shader_language fck_shader_compiler_language(fck_shader_generic *shader)
{
	return (fck_shader_language)shader->language;
}
const char *(fck_shader_compiler_file)(fck_shader_generic * shader)
{
	return shader->desc.file;
}
const char *fck_shader_compiler_entry_point(fck_shader_generic *shader)
{
	return shader->desc.entry_point;
}
const void *fck_shader_compiler_source(fck_shader_generic *shader)
{
	return shader->source;
}
fckc_size_t fck_shader_compiler_size(fck_shader_generic *shader)
{
	return shader->souce_byte_size;
}

FCK_SHADER_API fck_shader_compiler fck_shader_compiler_create()
{
	shaderc_compiler_t shaderc = shaderc_compiler_initialize();

	fck_shader_compiler compiler;
	memset(&compiler, 0, sizeof(compiler));
	compiler.handle = shaderc;

	compiler.create_spirv = fck_shader_create_spirv;
	compiler.create_hlsl_from_file = fck_shader_create_hlsl_from_file;
	compiler.create_glsl = fck_shader_create_glsl;
	compiler.create_hlsl = fck_shader_create_hlsl;
	compiler.destroy = fck_shader_destroy;
	compiler.shutdown = fck_shader_compiler_shutdown;
	compiler.type = fck_shader_compiler_type;
	compiler.language = fck_shader_compiler_language;
	compiler.file = fck_shader_compiler_file;
	compiler.entry_point = fck_shader_compiler_entry_point;
	compiler.source = fck_shader_compiler_source;
	compiler.size = fck_shader_compiler_size;
	return compiler;
}