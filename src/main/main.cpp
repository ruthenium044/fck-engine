#include "game/game_systems.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3_image/SDL_image.h>

#include <shaderc/shaderc.h>

#include "fck.h"

#include "fck_ui.h"

// Type Table
// id:u64	- name:string - size:int - serialiser:void(serialiser*,T*)
// 0		- "Int32"	  - 4		 - void(serialiser*,int32*)
// 1		- "Float"	  - 4		 - void(serialiser*,float*)
// 2		- "Double"	  - 8		 - void(serialiser*,double*)
// 3		- "Bool"	  - 4		 - void(serialiser*,bool*)
// 4		- "Short"	  - 2		 - void(serialiser*,short*)
// 5		- "Char"	  - 1		 - void(serialiser*,char*)
// 6		- "Float3"	  - 12		 - void(serialiser*,float3*)
// 7		- "Float3x3"  - 48		 - void(serialiser*,float3x3*)

// Gameplay Table
// enity:u64 - Components[T] -> Signature
// 0		 - ... int, float
// 1		 - ... char float
// 2		 - ... int
// 3		 - ... int
// 4		 - ... float3x3
// 5		 - ... float3x3, float
// 6		 - ... char
// 7		 - ... short, int

// Prefix Table - Broadcasted
// id:u64	- Type Table Ids			- Table
// 0		- ids of int, float			- *
// 1		- ids of char float			- *
// 2		- ids of int				- *
// 3		- ids of float3x3			- *
// 4		- ids of float3x3, float	- *
// 5		- ids of char				- *
// 6		- ids of int, short			- *

// Archetype Table - Broadcasted 
// Meta: (Components[T])
// enity:u64 : Components[T]
// Pointer to data for each type in signature

// typedef void* serialiser;
// typedef void(*serialise_generic)(serialiser*, void*);
//
//// BindingTable
// struct meta_table
//{
// };
//
// struct prefix_table
//{
// };
//
// uint64_t meta_table_register(meta_table*table, const char *name, size_t size, serialise_generic serialise)
//{
//
// }
// uint64_t meta_table_add(meta_table*table, uint64_t id, size_t size);
// uint64_t meta_table_remove(meta_table*table, uint64_t id, size_t size);
//
// uint64_t prefix_table_prefix(prefix_table* table, uint64_t* properties, uint64_t property_count);

void game_instance_setup(fck_ecs *ecs)
{
	
	// Good old fashioned init systems
	// TODO: sprite_sheet_setup is loading: Cammy. That is not so muy bien
	fck_ecs_system_add(ecs, fck_ui_setup);
	// fck_ecs_system_add(ecs, game_networking_setup);
	fck_ecs_system_add(ecs, game_authority_controllable_create);
	fck_ecs_system_add(ecs, game_networking_setup);

	// Good old fasioned update systems
	fck_ecs_system_add(ecs, game_input_process);
	fck_ecs_system_add(ecs, game_gameplay_process);

	fck_ecs_system_add(ecs, game_network_ui_process);
	fck_ecs_system_add(ecs, game_render_process);
}

const char *get_entry_point_name(SDL_GPUShaderStage stage)
{
	switch (stage)
	{
	case SDL_GPU_SHADERSTAGE_FRAGMENT:
		return "fragment";
	case SDL_GPU_SHADERSTAGE_VERTEX:
		return "vertex";
	}
	// Will never happen, if it happens LOL
	return nullptr;
}

SDL_GPUShader *create_shader(SDL_GPUDevice *device, const char *path, const char *name, shaderc_shader_kind kind, int sampler_count,
                             int uniform_buffer_count)
{
	SDL_assert(SDL_GetGPUShaderFormats(device) & SDL_GPU_SHADERFORMAT_SPIRV);

	size_t shader_size = 0;
	void *source = SDL_LoadFile(path, &shader_size);
	if (source == nullptr)
	{
		SDL_Log("%s", SDL_GetError());
		return nullptr;
	}
	shaderc_compiler_t compiler = shaderc_compiler_initialize();
	shaderc_compile_options_t options = shaderc_compile_options_initialize();

	shaderc_compile_options_set_source_language(options, shaderc_source_language_hlsl);
	shaderc_compile_options_set_hlsl_io_mapping(options, true);
	shaderc_compile_options_set_optimization_level(options, shaderc_optimization_level_zero);

	// TODO: Map it better later
	SDL_GPUShaderStage stage = kind == shaderc_vertex_shader ? SDL_GPU_SHADERSTAGE_VERTEX : SDL_GPU_SHADERSTAGE_FRAGMENT;
	const char *entry_point = get_entry_point_name(stage);
	shaderc_compilation_result_t result =
		shaderc_compile_into_spv(compiler, (const char *)source, shader_size, kind, name, entry_point, options);

	SDL_GPUShader *shader;
	if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success)
	{
		SDL_Log("Failed to compile shader: %s", shaderc_result_get_error_message(result));
		shader = nullptr;
	}
	else
	{
		SDL_GPUShaderCreateInfo shader_info;
		shader_info.code = (const Uint8 *)shaderc_result_get_bytes(result); // SDL takes care of the correct cast
		shader_info.code_size = shaderc_result_get_length(result);
		shader_info.entrypoint = entry_point;
		shader_info.format = SDL_GPU_SHADERFORMAT_SPIRV;
		shader_info.stage = stage;
		shader_info.num_samplers = sampler_count;
		shader_info.num_storage_textures = 0;
		shader_info.num_storage_buffers = 0;
		shader_info.num_uniform_buffers = uniform_buffer_count;
		shader_info.props = 0;

		shader = SDL_CreateGPUShader(device, &shader_info);
	}
	shaderc_result_release(result);
	shaderc_compile_options_release(options);
	shaderc_compiler_release(compiler);

	return shader;
}

struct vertex
{
	float x;
	float y;

	float u;
	float v;
};

union alignas(16) float3 {
	struct
	{
		float x;
		float y;
		float z;
	};
	float v[3];
};

union float3x3 {
	float3 v[3];
};

float3x3 float3x3_identity()
{
	float3x3 matrix;
	matrix.v[0].v[0] = 1.0f;
	matrix.v[0].v[1] = 0.0f;
	matrix.v[0].v[2] = 0.0f;
	matrix.v[1].v[0] = 0.0f;
	matrix.v[1].v[1] = 1.0f;
	matrix.v[1].v[2] = 0.0f;
	matrix.v[2].v[0] = 0.0f;
	matrix.v[2].v[1] = 0.0f;
	matrix.v[2].v[2] = 1.0f;
	return matrix;
}

float3x3 float3x3_translation(float x, float y)
{
	float3x3 matrix;
	matrix.v[0].v[0] = 1.0f;
	matrix.v[0].v[1] = 0.0f;
	matrix.v[0].v[2] = x;
	matrix.v[1].v[0] = 0.0f;
	matrix.v[1].v[1] = 1.0f;
	matrix.v[1].v[2] = y;
	matrix.v[2].v[0] = 0.0f;
	matrix.v[2].v[1] = 0.0f;
	matrix.v[2].v[2] = 1.0f;
	return matrix;
}

float3x3 float3x3_rotation(float rad)
{
	float3x3 matrix;
	float c = SDL_cosf(rad);
	float s = SDL_sinf(rad);

	matrix.v[0].v[0] = c;
	matrix.v[0].v[1] = -s;
	matrix.v[0].v[2] = 0.0f;
	matrix.v[1].v[0] = s;
	matrix.v[1].v[1] = c;
	matrix.v[1].v[2] = 0.0f;
	matrix.v[2].v[0] = 0.0f;
	matrix.v[2].v[1] = 0.0f;
	matrix.v[2].v[2] = 1.0f;
	return matrix;
}

float3x3 float3x3_scale(float x, float y)
{
	float3x3 matrix;
	matrix.v[0].v[0] = x;
	matrix.v[0].v[1] = 0.0f;
	matrix.v[0].v[2] = 0.0f;
	matrix.v[1].v[0] = 0.0f;
	matrix.v[1].v[1] = y;
	matrix.v[1].v[2] = 0.0f;
	matrix.v[2].v[0] = 0.0f;
	matrix.v[2].v[1] = 0.0f;
	matrix.v[2].v[2] = 1.0f;
	return matrix;
}

float3x3 float3x3_multiply_float3x3(float3x3 lhs, float3x3 rhs)
{
	float3x3 result;

	result.v[0].v[0] = lhs.v[0].v[0] * rhs.v[0].v[0] + lhs.v[0].v[1] * rhs.v[1].v[0] + lhs.v[0].v[2] * rhs.v[2].v[0];
	result.v[0].v[1] = lhs.v[0].v[0] * rhs.v[0].v[1] + lhs.v[0].v[1] * rhs.v[1].v[1] + lhs.v[0].v[2] * rhs.v[2].v[1];
	result.v[0].v[2] = lhs.v[0].v[0] * rhs.v[0].v[2] + lhs.v[0].v[1] * rhs.v[1].v[2] + lhs.v[0].v[2] * rhs.v[2].v[2];
	result.v[1].v[0] = lhs.v[1].v[0] * rhs.v[0].v[0] + lhs.v[1].v[1] * rhs.v[1].v[0] + lhs.v[1].v[2] * rhs.v[2].v[0];
	result.v[1].v[1] = lhs.v[1].v[0] * rhs.v[0].v[1] + lhs.v[1].v[1] * rhs.v[1].v[1] + lhs.v[1].v[2] * rhs.v[2].v[1];
	result.v[1].v[2] = lhs.v[1].v[0] * rhs.v[0].v[2] + lhs.v[1].v[1] * rhs.v[1].v[2] + lhs.v[1].v[2] * rhs.v[2].v[2];
	result.v[2].v[0] = lhs.v[2].v[0] * rhs.v[0].v[0] + lhs.v[2].v[1] * rhs.v[1].v[0] + lhs.v[2].v[2] * rhs.v[2].v[0];
	result.v[2].v[1] = lhs.v[2].v[0] * rhs.v[0].v[1] + lhs.v[2].v[1] * rhs.v[1].v[1] + lhs.v[2].v[2] * rhs.v[2].v[1];
	result.v[2].v[2] = lhs.v[2].v[0] * rhs.v[0].v[2] + lhs.v[2].v[1] * rhs.v[1].v[2] + lhs.v[2].v[2] * rhs.v[2].v[2];

	return result;
}

float3x3 float3x3_ortho(float x, float y, float width, float height, float rad, float units)
{
	float3x3 translation = float3x3_translation(-x, -y);
	float3x3 rotation = float3x3_rotation(rad);

	float3x3 scale;
	if (width > height)
	{
		float scale_factor = units / height;
		scale = float3x3_scale(scale_factor * ((float)height / (float)width), scale_factor);
	}
	else
	{
		float scale_factor = units / width;
		scale = float3x3_scale(scale_factor, scale_factor * ((float)width / (float)height));
	}

	float3x3 matrix = float3x3_multiply_float3x3(rotation, translation);
	return float3x3_multiply_float3x3(scale, matrix);
};

int main(int argc, char **argv)
{
	SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

	const SDL_GPUShaderFormat shader_format = SDL_GPU_SHADERFORMAT_SPIRV;
	SDL_GPUDevice *device = SDL_CreateGPUDevice(shader_format, true, nullptr);
	if (device == nullptr)
	{
		SDL_Log("%s", SDL_GetError());
		return -1;
	}
	SDL_Log("Current Backend: %s", SDL_GetGPUDeviceDriver(device));

	SDL_Window *window = SDL_CreateWindow("GPU Window", 800, 600, SDL_WINDOW_RESIZABLE);
	if (window == nullptr)
	{
		SDL_Log("%s", SDL_GetError());
		return -1;
	}

	bool claim_window_result = SDL_ClaimWindowForGPUDevice(device, window);
	if (claim_window_result == false)
	{
		SDL_Log("%s", SDL_GetError());
		return -1;
	}

	// TODO: Do not pass in name explicitly, parse the name from the path, replace hlsl with vert/fragment
	SDL_GPUShader *vert =
		create_shader(device, FCK_RESOURCE_DIRECTORY_PATH "/shaders/triangle.hlsl", "Triangle.vertex", shaderc_vertex_shader, 0, 1);
	SDL_GPUShader *frag =
		create_shader(device, FCK_RESOURCE_DIRECTORY_PATH "/shaders/triangle.hlsl", "Triangle.fragment", shaderc_fragment_shader, 1, 0);

	if (vert == nullptr)
	{
		SDL_Log("%s", SDL_GetError());
		return -1;
	}
	if (frag == nullptr)
	{
		SDL_Log("%s", SDL_GetError());
		return -1;
	}

	SDL_GPUGraphicsPipelineTargetInfo pipeline_target_info;
	pipeline_target_info.color_target_descriptions =
		(SDL_GPUColorTargetDescription[]){{.format = SDL_GetGPUSwapchainTextureFormat(device, window)}};
	pipeline_target_info.num_color_targets = 1;
	pipeline_target_info.depth_stencil_format = {};
	pipeline_target_info.has_depth_stencil_target = {};

	SDL_GPUVertexInputState vertex_input_state;
	vertex_input_state.num_vertex_buffers = 1;
	vertex_input_state.vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[]){
		{.slot = 0, .pitch = sizeof(vertex), .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX, .instance_step_rate = 0}};
	vertex_input_state.num_vertex_attributes = 2;
	vertex_input_state.vertex_attributes = (SDL_GPUVertexAttribute[]){
		{.location = 0, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = 0},
		{.location = 1, .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .offset = sizeof(float) * 2}};

	SDL_GPUGraphicsPipelineCreateInfo pipeline_create_info;
	SDL_zero(pipeline_create_info);
	pipeline_create_info.vertex_shader = vert;
	pipeline_create_info.fragment_shader = frag;
	pipeline_create_info.vertex_input_state = vertex_input_state;
	pipeline_create_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
	pipeline_create_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
	pipeline_create_info.multisample_state = {};
	pipeline_create_info.depth_stencil_state = {};
	pipeline_create_info.target_info = pipeline_target_info;
	pipeline_create_info.props = 0;

	SDL_GPUGraphicsPipeline *pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_create_info);

	SDL_ReleaseGPUShader(device, vert);
	SDL_ReleaseGPUShader(device, frag);

	SDL_GPUSamplerCreateInfo sampler_create_info;
	sampler_create_info.min_filter = SDL_GPU_FILTER_NEAREST;
	sampler_create_info.mag_filter = SDL_GPU_FILTER_NEAREST;
	sampler_create_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
	sampler_create_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	sampler_create_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	sampler_create_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
	sampler_create_info.mip_lod_bias = 0.0f;
	sampler_create_info.max_anisotropy = 0.0f;
	sampler_create_info.compare_op = SDL_GPU_COMPAREOP_INVALID;
	sampler_create_info.min_lod = 0.0f;
	sampler_create_info.max_lod = 0.0f;
	sampler_create_info.enable_anisotropy = false;
	sampler_create_info.enable_compare = false;

	SDL_GPUSampler *sampler = SDL_CreateGPUSampler(device, &sampler_create_info);

	// "Runtime Data"

	const size_t vertex_count = 6;

	SDL_GPUBufferCreateInfo vertex_buffer_create_info;
	vertex_buffer_create_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
	vertex_buffer_create_info.size = sizeof(vertex) * vertex_count;
	vertex_buffer_create_info.props = 0;
	SDL_GPUBuffer *vertex_buffer = SDL_CreateGPUBuffer(device, &vertex_buffer_create_info);

	SDL_GPUTransferBufferCreateInfo vertex_transfer_buffer_info;
	vertex_transfer_buffer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	vertex_transfer_buffer_info.size = vertex_buffer_create_info.size;
	vertex_transfer_buffer_info.props = 0;
	SDL_GPUTransferBuffer *vertex_transfer_buffer = SDL_CreateGPUTransferBuffer(device, &vertex_transfer_buffer_info);

	vertex *vertices = (vertex *)SDL_MapGPUTransferBuffer(device, vertex_transfer_buffer, false);
	vertices[0] = {-1.0f, -1.0f, 0.0f, 1.0f};
	vertices[1] = {1.0f, -1.0f, 1.0f, 1.0f};
	vertices[2] = {-1.0f, 1.0f, 0.0f, 0.0f};
	vertices[3] = {1.0f, 1.0f, 1.0f, 0.0f};
	vertices[4] = {-1.0f, 1.0f, 0.0f, 0.0f};
	vertices[5] = {1.0f, -1.0f, 1.0f, 1.0f};
	SDL_UnmapGPUTransferBuffer(device, vertex_transfer_buffer);

	// Load Texture
	SDL_Surface *image = IMG_Load(FCK_RESOURCE_DIRECTORY_PATH "/PestLogo.png");

	SDL_GPUTextureCreateInfo texture_create_info;
	texture_create_info.type = SDL_GPU_TEXTURETYPE_2D;
	texture_create_info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
	texture_create_info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
	texture_create_info.width = image->w;
	texture_create_info.height = image->h;
	texture_create_info.layer_count_or_depth = 1;
	texture_create_info.num_levels = 1;
	texture_create_info.sample_count = SDL_GPU_SAMPLECOUNT_1;
	texture_create_info.props = 0;
	SDL_GPUTexture *texture = SDL_CreateGPUTexture(device, &texture_create_info);

	SDL_GPUTransferBufferCreateInfo texture_transfer_buffer_info;
	texture_transfer_buffer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
	texture_transfer_buffer_info.size = image->w * image->h * 4; // RGBA bytes
	texture_transfer_buffer_info.props = 0;

	SDL_GPUTransferBuffer *texture_transfer_buffer = SDL_CreateGPUTransferBuffer(device, &texture_transfer_buffer_info);
	Uint8 *data = (Uint8 *)SDL_MapGPUTransferBuffer(device, texture_transfer_buffer, false);
	SDL_memcpy(data, image->pixels, (size_t)image->w * (size_t)image->h * 4);
	SDL_UnmapGPUTransferBuffer(device, texture_transfer_buffer);

	{
		// TODO: Error checks :)
		SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(device);
		SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(command_buffer);

		{
			SDL_GPUTransferBufferLocation source;
			source.transfer_buffer = vertex_transfer_buffer;
			source.offset = 0;

			SDL_assert(vertex_buffer_create_info.size == vertex_transfer_buffer_info.size);
			SDL_GPUBufferRegion destination;
			destination.buffer = vertex_buffer;
			destination.offset = 0;
			destination.size = vertex_buffer_create_info.size;

			SDL_UploadToGPUBuffer(copy_pass, &source, &destination, false);
		}

		{
			SDL_GPUTextureTransferInfo source;
			source.transfer_buffer = texture_transfer_buffer;
			source.offset = 0;
			source.pixels_per_row = 0;
			source.rows_per_layer = 0;

			SDL_GPUTextureRegion destination;
			destination.texture = texture;
			destination.mip_level = 0;
			destination.layer = 0;
			destination.x = 0;
			destination.y = 0;
			destination.z = 0;
			destination.w = image->w;
			destination.h = image->h;
			destination.d = 1;
			SDL_UploadToGPUTexture(copy_pass, &source, &destination, false);
		}
		SDL_EndGPUCopyPass(copy_pass);
		SDL_SubmitGPUCommandBuffer(command_buffer);

		SDL_ReleaseGPUTransferBuffer(device, vertex_transfer_buffer);
		SDL_ReleaseGPUTransferBuffer(device, texture_transfer_buffer);
		SDL_DestroySurface(image);
	}

	while (true)
	{
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_EVENT_QUIT)
			{
				return 0;
			}
		}

		int window_width;
		int window_height;
		/*Discard reutrn */ SDL_GetWindowSize(window, &window_width, &window_height);

		float aspect = (float)window_width / (float)window_height;

		// Draw here :)
		SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(device);
		if (command_buffer == NULL)
		{
			SDL_Log("AcquireGPUCommandBuffer failed: %s", SDL_GetError());
			return -1;
		}

		SDL_GPUTexture *swapchain_texture;
		if (!SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, window, &swapchain_texture, NULL, NULL))
		{
			SDL_Log("WaitAndAcquireGPUSwapchainTexture failed: %s", SDL_GetError());
			return -1;
		}

		if (swapchain_texture != nullptr)
		{
			SDL_GPUColorTargetInfo color_target_info;
			SDL_zero(color_target_info);
			color_target_info.texture = swapchain_texture;
			color_target_info.mip_level = 0;
			color_target_info.layer_or_depth_plane = 0;
			color_target_info.clear_color = (SDL_FColor){0.0f, 0.0f, 0.0f, 1.0f};
			color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
			color_target_info.store_op = SDL_GPU_STOREOP_STORE;
			color_target_info.resolve_texture = nullptr;
			color_target_info.resolve_mip_level = 0;
			color_target_info.resolve_layer = 0;
			color_target_info.cycle = false;
			color_target_info.cycle_resolve_texture = false;

			// TODO: Implement unit system
			const float units = 1000.0f;

			float3x3 model = float3x3_scale(1.0f, 1.0f);
			float3x3 view = float3x3_translation(0.0, 0.0f);
			float3x3 projection = float3x3_ortho(0.0f, 0.0f, (float)window_width, (float)window_height, 0.0f, units);
			float3x3 mvp = float3x3_multiply_float3x3(projection, float3x3_multiply_float3x3(view, model));

			SDL_PushGPUVertexUniformData(command_buffer, 0, &mvp, sizeof(mvp));

			SDL_GPURenderPass *render_pass = SDL_BeginGPURenderPass(command_buffer, &color_target_info, 1, nullptr);
			SDL_BindGPUGraphicsPipeline(render_pass, pipeline);

			SDL_GPUViewport viewport;
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.w = window_width;
			viewport.h = window_height;
			viewport.min_depth = 0.0f;
			viewport.max_depth = 1.0f;

			SDL_Rect scissor;
			scissor.x = 0;
			scissor.y = 0;
			scissor.w = window_width;
			scissor.h = window_height;

			SDL_SetGPUViewport(render_pass, &viewport);
			SDL_SetGPUScissor(render_pass, &scissor);

			SDL_GPUBufferBinding vertex_buffer_binding;
			vertex_buffer_binding.buffer = vertex_buffer;
			vertex_buffer_binding.offset = 0;
			SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_buffer_binding, 1);

			SDL_GPUTextureSamplerBinding texture_binding;
			texture_binding.texture = texture;
			texture_binding.sampler = sampler;
			SDL_BindGPUFragmentSamplers(render_pass, 0, &texture_binding, 1);

			SDL_DrawGPUPrimitives(render_pass, vertex_count, 1, 0, 0);

			SDL_EndGPURenderPass(render_pass);
		}

		// This one returns
		SDL_SubmitGPUCommandBuffer(command_buffer);
	}

	SDL_Quit();
	return 0;

	fck fck;
	fck_init(&fck, 3);
	{
		fck_instance_info client_info0;
		client_info0.title = "fck engine - client 0";
		fck_prepare(&fck, &client_info0, game_instance_setup);

		fck_instance_info client_info1;
		client_info1.title = "fck engine - client 1";
		fck_prepare(&fck, &client_info1, game_instance_setup);

		fck_instance_info host_info;
		host_info.title = "fck engine - host";
		fck_prepare(&fck, &host_info, game_instance_setup);
	}

	int exit_code = fck_run(&fck);
	SDL_Log("fck - exit code: %d", exit_code);

	return exit_code;
}
