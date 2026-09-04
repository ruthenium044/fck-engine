// #include <dxc/WinAdapter.h>

extern "C"
{
#include "fck_shader.h"
#include <fck_apis.h>
#include <fck_db.h>
#include <fck_os.h>
#include <fckc_assert.h>
#include <kll.h>
#include <kll_malloc.h>
}

#include <shaderc/shaderc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// We use malloc for now cause lazy!

static fck_shader_generic fck_shader_create_generic(fckc_u32 lang, fck_shader_desc *desc, void const *source, fckc_size_t source_size)
{
	fck_shader_generic generic;
	memset(&generic, 0xCF, sizeof(generic));
	generic.language = lang;
	generic.desc.type = desc->type;

	const fckc_size_t source_len = source_size; // No need for alignment cause first!
	const fckc_size_t file_len = strlen(desc->file);
	const fckc_size_t ep_len = strlen(desc->entry_point);
	const fckc_size_t terminator_count = 4;
	const fckc_size_t total_len = file_len + ep_len + source_len + terminator_count;

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

static fck_spirv_object fck_shader_create_spirv(struct fck_shader_compiler *compiler, fck_shader_generic *shader)
{
	shaderc_shader_kind shader_kind;

	switch (shader->desc.type)
	{
	case fck_shader_unkown:
		shader_kind = shaderc_glsl_infer_from_source;
		break;
	case fck_shader_vertex:
		shader_kind = shaderc_vertex_shader;
		break;
	case fck_shader_fragment:
		shader_kind = shaderc_fragment_shader;
		break;
	case fck_shader_compute:
		shader_kind = shaderc_compute_shader;
		break;
	default:
		shader_kind = shaderc_glsl_infer_from_source;
		break;
	}

	if (shader->language == fck_shader_spirv)
	{
		return fck_spirv_object();
	}
	// TODO: Make this pretty :)
	if (shader->language == fck_shader_glsl || shader->language == fck_shader_hlsl)
	{
		shaderc_compiler_t shaderc = (shaderc_compiler_t)compiler->handle;
		shaderc_compilation_result_t result;

		shaderc_compile_options_t options = shaderc_compile_options_initialize();
		shaderc_source_language lang;
		switch (shader->language)
		{
		case fck_shader_glsl:
			lang = shaderc_source_language_glsl;
			break;
		case fck_shader_hlsl:
			lang = shaderc_source_language_hlsl;
			break;
		}

		{
			const char *source = shader->source;
			const char *file = shader->desc.file;
			const char *entry_point = shader->desc.entry_point;
			const fckc_size_t byte_size = shader->souce_byte_size;
			shaderc_compile_options_set_source_language(options, lang);
			result = shaderc_compile_into_spv(shaderc, source, byte_size, shader_kind, file, entry_point, options);
		}

		shaderc_compile_options_release(options);
		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success)
		{
			const char *error = shaderc_result_get_error_message(result);
			printf("%s", error);
			return fck_spirv_object();
		}

		const size_t size = shaderc_result_get_length(result);
		const char *source = shaderc_result_get_bytes(result);

		fck_spirv_object spirv;
		spirv.generic = fck_shader_create_generic(fck_shader_spirv, &shader->desc, source, size);
		shaderc_result_release(result);
		return spirv;
	}

	return fck_spirv_object();
}

static fck_glsl_object fck_shader_create_glsl(struct fck_shader_compiler *compiler, fck_shader_desc *desc, const char *source)
{
	(void)compiler;
	fck_glsl_object glsl;
	glsl.generic = fck_shader_create_generic(fck_shader_glsl, desc, source, strlen(source));
	return glsl;
}

static fck_hlsl_object fck_shader_create_hlsl(struct fck_shader_compiler *compiler, fck_shader_desc *desc, const char *source)
{
	(void)compiler;
	fck_hlsl_object hlsl;
	hlsl.generic = fck_shader_create_generic(fck_shader_hlsl, desc, source, strlen(source));
	return hlsl;
}

static fck_hlsl_object fck_shader_create_hlsl_from_file(struct fck_shader_compiler *compiler, fck_shader_desc *desc, fck_file *file)
{
	const fckc_size_t size = os->fs->size(*file);
	char *text = (char *)kll_malloc(kll->system, size + 1);
	const fckc_size_t read = os->fs->read(*file, text, size);
	fck_assert(size == read);
	text[read] = '\0';

	fck_hlsl_object hlsl = compiler->create_hlsl(compiler, desc, text);
	return hlsl;
}

static fck_glsl_object fck_shader_create_glsl_from_file(struct fck_shader_compiler *compiler, fck_shader_desc *desc, fck_file *file)
{
	const fckc_size_t size = os->fs->size(*file);
	char *text = (char *)kll_malloc(kll->system, size + 1);
	const fckc_size_t read = os->fs->read(*file, text, size);
	fck_assert(size == read);
	text[read] = '\0';

	fck_glsl_object glsl = compiler->create_glsl(compiler, desc, text);
	return glsl;
}

static void fck_shader_destroy(struct fck_shader_compiler *compiler, fck_shader_generic *shader)
{
	(void)compiler;
	free((void *)shader->source);
}

static void fck_shader_compiler_shutdown(fck_shader_compiler *compiler)
{
	shaderc_compiler_t shaderc = (shaderc_compiler_t)compiler->handle;
	shaderc_compiler_release(shaderc);
}

static fck_shader_stage_type fck_shader_compiler_type(fck_shader_generic *shader)
{
	return (fck_shader_stage_type)shader->desc.type;
}
static fck_shader_language fck_shader_compiler_language(fck_shader_generic *shader)
{
	return (fck_shader_language)shader->language;
}
static const char *(fck_shader_compiler_file)(fck_shader_generic * shader)
{
	return shader->desc.file;
}
static const char *fck_shader_compiler_entry_point(fck_shader_generic *shader)
{
	return shader->desc.entry_point;
}
static const void *fck_shader_compiler_source(fck_shader_generic *shader)
{
	return shader->source;
}
static fckc_size_t fck_shader_compiler_size(fck_shader_generic *shader)
{
	return shader->souce_byte_size;
}

static fck_shader_compiler fck_shader_compiler_create(void)
{
	shaderc_compiler_t shaderc = shaderc_compiler_initialize();

	fck_shader_compiler compiler;
	memset(&compiler, 0, sizeof(compiler));
	compiler.handle = shaderc;

	compiler.create_spirv = fck_shader_create_spirv;
	compiler.create_hlsl_from_file = fck_shader_create_hlsl_from_file;
	compiler.create_glsl_from_file = fck_shader_create_glsl_from_file;
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

static int fck_shader_api_is_ok(fck_shader_compiler compiler)
{
	return compiler.handle != NULL;
}

extern "C"
{
	typedef struct fck_shader_asset
	{
		fckc_i64 timestamp;
		fck_glsl_object value;

	} fck_shader_asset;

	int fck_shader_asset_api_dirty(const struct fck_db_asset *asset)
	{
		if (strcmp(asset->category, fck_category_shader) != 0)
		{
			return 0;
		}
		fck_shader_asset *data = (fck_shader_asset *)asset->userdata;
		const int result = data->timestamp <= asset->timestamp;
		data->timestamp = os->chrono->now();
		return result;
	}

	static fck_glsl_object fck_shader_asset_api_resolve(const struct fck_db_asset *asset)
	{
		if (strcmp(asset->category, fck_category_shader) != 0)
		{
			return fck_glsl_object();
		}
		fck_shader_asset *data = (fck_shader_asset *)asset->userdata;
		return data->value;
	}

	static fck_shader_asset_api shader_asset_api = {
		fck_shader_asset_api_dirty,
		fck_shader_asset_api_resolve,
	};

	static fck_shader_api shader_api = {
		&shader_asset_api,
		fck_shader_compiler_create,
		fck_shader_api_is_ok,
	};

	static void *fck_shader_import(const fck_db_loader_args *args, const char *file)
	{
		os->io->log("Load Shader: %s", file);
		fck_shader_api *shader = (fck_shader_api *)args->registry->find(fck_shader_api_name);

		const char *ext = fck_db_extension(file);
		fck_shader_stage_type type = fck_shader_unkown;
		if (ext)
		{
			if (strcmp(ext, "vert") == 0 || strcmp(ext, "vs") == 0)
			{
				type = fck_shader_vertex;
			}
			else if (strcmp(ext, "frag") == 0 || strcmp(ext, "fs") == 0)
			{
				type = fck_shader_fragment;
			}
		}
		fck_shader_compiler compiler = shader->create();
		fck_assert(shader->is_ok(compiler));

		const fck_db_accessor accessor = args->api->object->edit(args->db, args->target);

		fck_file file_handle = os->fs->open(file, "rb");
		fck_shader_desc desc = {to_u32(type), file, "main"};
		fck_glsl_object shader_object = compiler.create_glsl_from_file(&compiler, &desc, &file_handle);

		fck_shader_asset *asset = (fck_shader_asset *)accessor.read->userdata(accessor, fck_category_shader);
		if (asset == NULL)
		{
			fck_shader_asset value = {.timestamp = os->chrono->now()};
			asset = (fck_shader_asset *)accessor.edit->userdata(accessor, fck_category_shader, &value, sizeof(value));
		}
		asset->value = shader_object;
		// Kill the previous one?
		accessor.edit->commit(accessor, fck_db_no_undo);

		os->fs->close(file_handle);
		// compiler.destroy(&compiler, &shader_object.generic);
		compiler.shutdown(&compiler);
		return asset;
	}

	static fckc_size_t fck_shader_supports(const char ***extensions)
	{
		static const char *supported[] = {"vert", "frag", "vs", "fs"};
		*extensions = supported;
		return fck_arraysize(supported);
	}

	static fck_db_loader_interface fck_db_loader_interface_create_cpp()
	{
		fck_db_loader_interface result = {};
		result.category = fck_category_shader;
		result.import = fck_shader_import;
		result.supports = fck_shader_supports;
		return result;
	}

	fck_db_loader_interface shader_loader = fck_db_loader_interface_create_cpp();

	FCK_EXPORT_API void *fck_shader_load(fck_api_registry *registry, void *old)
	{
		(void)old;
		registry->add(fck_shader_api_name, &shader_api);
		registry->add(fck_db_loader_interface_name, &shader_loader);
		return &shader_api;
	}
}