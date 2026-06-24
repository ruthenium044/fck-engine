#ifndef SHT_RENDER_H_INCLUDED
#define SHT_RENDER_H_INCLUDED

#include <fckc_inttypes.h>

#define sht_render_api_name "sht-render"

// Not much here is widely ABI compatible yet!
struct fck_shader_generic;
struct fck_window;

#define sht_make_version(major, minor, patch) ((((fckc_u32)(major)) << 22U) | (((fckc_u32)(minor)) << 12U) | ((fckc_u32)(patch)))
#define sht_version_major(version) ((fckc_u32)(version) >> 22U)
#define sht_version_minor(version) (((fckc_u32)(version) >> 12U) & 0x3FFU)
#define sht_version_patch(version) ((fckc_u32)(version) & 0xFFFU)

#define sht_header_version sht_make_version(0, 0, 1)

// TODO: Lots of stuff would benefit from being marked as const... Just to incidate it is no out/ref

typedef struct sht_render_api_config
{
	fckc_u32 version;
} sht_render_api_config;

// I do not fucking care if higher
#define SHT_VK_IMAGE_COUNT 4

typedef fckc_u32 sht_bool32;
#define sht_true 1
#define sht_false 0

typedef void sht_heap;
typedef void sht_handle;
typedef fckc_u64 sht_id;

typedef enum sht_queue_type
{
	SHT_QUEUE_GRAPHIC,
	SHT_QUEUE_PRESENT,
	SHT_QUEUE_TRANSFER,
	SHT_QUEUE_COMPUTE,
	SHT_QUEUE_COUNT,
} sht_queue_type;

typedef struct sht_offset
{
	fckc_f32 x;
	fckc_f32 y;
} sht_offset;

typedef struct sht_extent
{
	fckc_f32 width;
	fckc_f32 height;
} sht_extent;

typedef struct sht_range_f32
{
	fckc_f32 min;
	fckc_f32 max;
} sht_range_f32;

typedef struct sht_viewport
{
	sht_offset offset;
	sht_extent extent;
	sht_range_f32 depth;
} sht_viewport;

typedef struct sht_scissor
{
	sht_offset offset;
	sht_extent extent;
} sht_scissor;

typedef enum sht_memory_type
{
	SHT_MEMORY_GPU,
	SHT_MEMORY_CPU,
	SHT_MEMORY_COUNT
} sht_memory_type;

typedef enum sht_transfer_flags
{
	SHT_TRANSFER_RETAINED = 0x00,
	SHT_TRANSFER_SOURCE = 0x01,
	SHT_TRANSFER_TARGET = 0x02,
} sht_transfer_flags;

typedef enum sht_buffer_usage_flags
{
	SHT_BUFFER_USAGE_UNIFORM = 0x0000001,
	SHT_BUFFER_USAGE_STORAGE = 0x0000002,
	SHT_BUFFER_USAGE_INDEX = 0x0000004,
	SHT_BUFFER_USAGE_VERTEX = 0x0000008,
	SHT_BUFFER_USAGE_INDIRECT = 0x0000010,
} sht_buffer_usage_flags;

typedef enum sht_image_usage_flags
{
	SHT_IMAGE_USAGE_SAMPLED = 0x00000001,
	SHT_IMAGE_USAGE_STORAGE = 0x00000002,
	SHT_IMAGE_USAGE_COLOR_ATTACHMENT = 0x00000004,
	SHT_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT = 0x00000008,
	SHT_IMAGE_USAGE_INPUT_ATTACHMENT = 0x00000010,
	// SHT_IMAGE_USAGE_TRANSIENT_ATTACHMENT = 0x00000010, // Not now!
} sht_image_usage_flags;

typedef enum sht_memory_access_operation
{
	SHT_LOAD = 0,
	SHT_STORE = 0,
	SHT_DONT_CARE = 1, // Maybe, discard?
	SHT_CLEAR = 2,
} sht_memory_access_operation;

typedef enum sht_layout_type
{
	SHT_LAYOUT_UNDEFINED = 0,
	SHT_LAYOUT_GENERAL = 1,
	SHT_LAYOUT_COLOR_ATTACHMENT = 2,
	SHT_LAYOUT_DEPTH_STENCIL_ATTACHMENT = 3,
	SHT_LAYOUT_DEPTH_STENCIL_READ_ONLY = 4,
	SHT_LAYOUT_SHADER_READ_ONLY = 5,
	SHT_LAYOUT_PRESENT = 1000001002,
} sht_layout_type;

typedef enum sht_stage_flags
{
	SHT_STAGE_VERTEX_SHADER = 0x00000001,
	SHT_STAGE_FRAGMENT_SHADER = 0x00000002,
} sht_stage_flags;

typedef enum sht_access_flags
{
	SHT_ACCESS_INDIRECT_COMMAND_READ = 0x00000001,
	SHT_ACCESS_INDEX_READ = 0x00000002,
	SHT_ACCESS_VERTEX_ATTRIBUTE_READ = 0x00000004,
	SHT_ACCESS_UNIFORM_READ = 0x00000008,
	SHT_ACCESS_INPUT_ATTACHMENT_READ = 0x00000010,
	SHT_ACCESS_SHADER_READ = 0x00000020,
	SHT_ACCESS_SHADER_WRITE = 0x00000040,
	SHT_ACCESS_COLOR_ATTACHMENT_READ = 0x00000080,
	SHT_ACCESS_COLOR_ATTACHMENT_WRITE = 0x00000100,
	SHT_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ = 0x00000200,
	SHT_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE = 0x00000400,
	SHT_ACCESS_TRANSFER_READ = 0x00000800,
	SHT_ACCESS_TRANSFER_WRITE = 0x00001000,
	SHT_ACCESS_HOST_READ = 0x00002000,
	SHT_ACCESS_HOST_WRITE = 0x00004000,
	SHT_ACCESS_MEMORY_READ = 0x00008000,
	SHT_ACCESS_MEMORY_WRITE = 0x00010000,
} sht_access_flags;

typedef struct sht_buffer_configuration
{
	sht_transfer_flags transfer;
	sht_buffer_usage_flags usage;
	fckc_size_t size;
} sht_buffer_configuration;

#define sht_buffer_source(usage_flags, memory_size)                                                                                        \
	(sht_buffer_configuration)                                                                                                             \
	{                                                                                                                                      \
		.transfer = SHT_TRANSFER_SOURCE, .usage = (usage_flags), .size = (memory_size)                                                     \
	}

#define sht_buffer_target(usage_flags, memory_size)                                                                                        \
	(sht_buffer_configuration)                                                                                                             \
	{                                                                                                                                      \
		.transfer = SHT_TRANSFER_TARGET, .usage = (usage_flags), .size = (memory_size)                                                     \
	}

#define sht_buffer_retained(usage_flags, memory_size)                                                                                      \
	(sht_buffer_configuration)                                                                                                             \
	{                                                                                                                                      \
		.transfer = SHT_TRANSFER_RETAINED, .usage = (usage_flags), .size = (memory_size)                                                   \
	}

typedef enum sht_format
{
	// Rename them into something friendlier!
	SHT_FORMAT_UNDEFINED = 0,
	SHT_FORMAT_R8G8B8A8_UNORM = 1,
	SHT_FORMAT_R8G8B8A8_SRGB = 2,
	SHT_FORMAT_B8G8R8A8_UNORM = 3,
	SHT_FORMAT_B8G8R8A8_SRGB = 4,
	SHT_FORMAT_D16_UNORM = 5,

	SHT_FORMAT_R32_SFLOAT = 418,
	SHT_FORMAT_R32G32_SFLOAT = 419,
	SHT_FORMAT_R32G32B32_SFLOAT = 420,
	SHT_FORMAT_R32G32B32A32_SFLOAT = 421,
} sht_format;

typedef struct sht_vertex_binding
{
	fck_alias(sht_format, fckc_u32) format;
	fckc_u32 location;
	fckc_size_t offset;
} sht_vertex_binding;

typedef struct sht_vertex_desc
{
	fckc_size_t stride;
	const sht_vertex_binding *bindings;
	fckc_size_t count;
} sht_vertex_desc;

typedef struct sht_image_configuration
{
	// TODO: make alias!
	sht_format format;
	sht_transfer_flags transfer;
	sht_image_usage_flags usage;
	fckc_u32 width;
	fckc_u32 height;
} sht_image_configuration;

typedef struct sht_buffer
{
	// You have seen and heard of magic, now you will see shit!
	sht_heap *heap;

	fckc_size_t size;
	void *gpu; // TODO: Shall be void*
	void *cpu;
} sht_buffer;

typedef struct sht_image
{
	sht_heap *heap;
	fck_alias(sht_format, fckc_u32) format;
	fck_alias(sht_image_usage_flags, fckc_u32) usage;
	fckc_u32 width;
	fckc_u32 height;
	// 32 bits whole on 64-bit systems :-(
	// fckc_size_t size;
	void *gpu;
	void *cpu;
} sht_image;

typedef struct sht_image_view
{
	fck_alias(sht_format, fckc_u32) format;
	void *gpu;
} sht_image_view;

// Maybe make some utility around these!
typedef struct sht_elements
{
	sht_buffer buffer;
	fckc_size_t count;
} sht_elements;

typedef struct sht_memory_arena
{
	sht_handle *owner;

	void *cpu[SHT_MEMORY_COUNT];
	fckc_size_t offset[SHT_MEMORY_COUNT];
	fckc_size_t capacity[SHT_MEMORY_COUNT];
	sht_heap *heaps[SHT_MEMORY_COUNT];
} sht_memory_arena;

typedef struct sht_memory_image
{
	sht_image (*create)(sht_memory_arena *mem, sht_image_configuration *config, sht_memory_type memory_type);
	void (*destroy)(sht_memory_arena *mem, sht_image *image);

	sht_bool32 (*is_ok)(sht_image *image);
	// Get used to this terminology.
	// You view and discard resource views, you create and destroy the resources!
	sht_image_view (*view)(sht_memory_arena *mem, sht_image image, fck_alias(sht_format, fckc_u32) format);
	void (*discard)(sht_memory_arena *mem, sht_image_view *view);

	sht_bool32 (*recreate)(sht_memory_arena *mem, sht_image *image, sht_extent extent, sht_image_view *views, fckc_size_t view_count);
} sht_memory_image;

typedef struct sht_memory
{
	// API is not that nice...
	//  Named convenience pointers
	sht_memory_arena *bump;
	sht_memory_arena *temp;

	sht_memory_arena objects[2];

	// UUUUH - I think this is only being used internally
	sht_memory_arena *(*of)(struct sht_memory *mem, sht_heap *buffer);

	sht_buffer (*malloc)(sht_memory_arena *mem, sht_buffer_configuration *config, sht_memory_type memory_type);
	void (*free)(sht_memory_arena *mem, sht_buffer *buffer);
	void (*reset)(sht_memory_arena *mem);
	sht_bool32 (*is_ok)(sht_buffer buffer);

	sht_memory_image *image;
} sht_memory;

typedef struct sht_swapchain
{
	sht_handle *handle;
	struct sht_swapchain_vt *vt;
} sht_swapchain;

typedef enum sht_swapchain_state
{
	SHT_SWAPCHAIN_FIRST_INDEX = 0,
	SHT_SWAPCHAIN_LAST_INDEX = 127,
	SHT_SWAPCHAIN_ISSUES = 1 << 31,
	SHT_SWAPCHAIN_NEEDS_RESIZE = SHT_SWAPCHAIN_ISSUES | 1,
} sht_swapchain_state;

typedef struct sht_swapchain_vt
{
	// TODO: More stuff
	sht_image_view (*wait_and_acquire)(sht_swapchain swapchain, fck_alias(sht_swapchain_state *, fckc_u32 *) index_or_state);
	sht_extent (*extent)(sht_swapchain swapchain);
	sht_extent (*display)(sht_swapchain swapchain);
	float (*scale)(sht_swapchain swapchain);
	sht_bool32 (*is_ok)(sht_swapchain swapchain, fck_alias(sht_swapchain_state, fckc_u32) index_or_state);
	// void *(*get_family)(sht_queues queues); // TODO:
} sht_swapchain_vt;

//
typedef struct sht_queues
{
	sht_handle *handle;
	struct sht_queues_vt *vt;
} sht_queues;

typedef struct sht_queues_vt
{
	void *(*get_family)(sht_queues queues); // TODO:
} sht_queues_vt;

typedef struct sht_driver
{
	sht_handle *handle;
	struct sht_driver_vt *vt;
} sht_driver;

typedef struct sht_render_pass
{
	sht_handle *handle;
} sht_render_pass;

typedef struct sht_graphics_pipeline
{
	sht_handle *owner;
	sht_id handle;
} sht_graphics_pipeline;

typedef struct sht_color_target_desc
{
	sht_image_view view;
	sht_memory_access_operation load_op;
	sht_memory_access_operation store_op;
	fckc_f32 clear_value[4];
	// ...
} sht_color_target_desc;

typedef struct sht_depth_target_desc
{
	sht_image_view view;
	sht_memory_access_operation load_op;
	sht_memory_access_operation store_op;
	fckc_f32 clear_value;
	// ...
} sht_depth_target_desc;

typedef struct sht_render_desc
{
	sht_depth_target_desc depth;
	sht_color_target_desc colour; // TODO: Make this multiple!
} sht_render_desc;

typedef enum sht_topology_type
{
	SHT_TRIANGLE_LIST,
} sht_topology_type;

typedef enum sht_cull_mode_flags
{
	SHT_CULL_MODE_NONE = 0x0,
	SHT_CULL_MODE_FRONT = 0x1,
	SHT_CULL_MODE_BACK = 0x2,
	SHT_CULL_MODE_BOTH = 0x3,
} sht_cull_mode_flags;

typedef struct sht_raster_desc
{
	sht_topology_type topology;
	sht_cull_mode_flags cull_mode;

	sht_format color;
	sht_format depth;
} sht_raster_desc;

typedef enum sht_filter
{
	sht_filter_nearest,
	sht_filter_linear,
} sht_filter;

typedef struct sht_sampler
{
	sht_handle *handle;
} sht_sampler;

typedef enum sht_binding_type
{
	SHT_BINDING_NONE = 0,
	SHT_BINDING_UNIFORM, // Maybe this one is not needed if STORAGE can fulfill the same role!!!
	SHT_BINDING_STORAGE, // Currently only readonly? Feature vertexPipelineStoresAndAtomics enabled? Maybe only relevant for vertex stage
	SHT_BINDING_READ_ONLY_IMAGE,
	// TODO: sampler and all that bs
	SHT_BINDING_TYPE_COUNT,
} sht_binding_type;

typedef struct sht_binding
{
	fck_alias(sht_binding_type, fckc_u32) type;
	fck_alias(sht_stage_flags, fckc_u32) stages;
	fckc_u32 id;
} sht_binding;

typedef struct sht_binding_desc
{
	// sht_binding_type bindings[16];
	const sht_binding *bindings;
	fckc_size_t count;
} sht_binding_desc;

typedef struct sht_bss
{
	sht_handle *owner;
	sht_handle *handle;
} sht_bss;

typedef struct sht_graphic_desc
{
	sht_raster_desc raster;

	sht_vertex_desc *vertex_desc;

	struct fck_shader_generic *vertex;
	struct fck_shader_generic *fragment;
} sht_graphic_desc;

typedef struct sht_command_buffer
{
	sht_handle *owner;
	sht_handle *handle;
} sht_command_buffer;

typedef struct sht_draw_indexed_desc
{
	fckc_i32 vertex_offset;
	fckc_u32 first_index;
	fckc_u32 index_count;
	fckc_u32 first_instance;
	fckc_u32 instance_count;
} sht_draw_indexed_desc;

#define sht_draw_indexed_params &(sht_draw_indexed_desc)

typedef struct sht_render_pass_vt
{
	sht_render_pass (*begin)(sht_command_buffer command_buffer, sht_render_desc *desc);
	sht_bool32 (*is_ok)(sht_render_pass render_pass);
	void (*end)(sht_command_buffer command_buffer);
} sht_render_pass_vt;

typedef struct sht_buffer_upload_desc
{
	const void *data;
	fckc_size_t size;
	fckc_size_t count;
} sht_buffer_upload_desc;

typedef struct sht_image_upload_desc
{
	sht_image_view views;
	sht_sampler samplers;
	// Cut bindless for now! Not worth it.
} sht_image_upload_desc;

typedef struct sht_bss_vt
{
	sht_bss (*create)(sht_driver driver, sht_binding_desc *desc);
	// ... Maybe upload_memory vs upload... Idk if this upload function cuts it tbh
	// Yup... Fuck the upload_desc structure.
	sht_bool32 (*upload_buffer)(sht_bss bss, fckc_u32 id, const sht_buffer_upload_desc *desc);
	sht_bool32 (*upload_image)(sht_bss bss, fckc_u32 id, const sht_image_upload_desc *desc);
	// TODO: This one updates all. It broadcasts... ...
	// sht_bool32 (*broadcast)(sht_bss bss, fckc_u32 id, sht_upload_desc *desc);
	void (*destroy)(sht_bss *bss);
} sht_bss_vt;

typedef struct sht_command_buffer_vt
{
	sht_command_buffer (*create)(sht_driver driver);
	void (*destroy)(sht_command_buffer *command);

	sht_command_buffer (*acquire)(sht_driver driver, fckc_u32 index);
	sht_bool32 (*is_ok)(sht_command_buffer command);

	// No multi-viewport support for now!
	void (*scissor)(sht_command_buffer command, sht_scissor *scissor);
	void (*viewport)(sht_command_buffer command, sht_viewport *viewport);
	void (*submit)(sht_command_buffer command, sht_queue_type queue_type);

	void (*graphics_pipeline)(sht_command_buffer command, sht_graphics_pipeline pipeline);

	void (*vertex_buffer)(sht_command_buffer command, sht_buffer *vertex_buffer, fckc_u64 offset);
	void (*index_buffer)(sht_command_buffer command, sht_buffer *index_buffer, fckc_u64 offset);
	void (*bss)(sht_command_buffer command, sht_bss bss);

	void (*draw_indexed)(sht_command_buffer command, sht_draw_indexed_desc *params);

	sht_render_pass_vt *render_pass;

} sht_command_buffer_vt;

typedef struct sht_graphics_pipeline_vt
{
	sht_graphics_pipeline (*create)(sht_driver driver, sht_bss bss, sht_graphic_desc *desc);
	sht_bool32 (*is_ok)(sht_graphics_pipeline pipeline);
	void (*destroy)(sht_graphics_pipeline pipeline);
	// void (*end)(sht_command_buffer command_buffer);
} sht_graphics_pipeline_vt;

typedef struct sht_driver_vt
{
	sht_bss_vt *bss;
	sht_command_buffer_vt *command_buffer;
	sht_graphics_pipeline_vt *graphics_pipeline;

	void (*shutdown)(sht_driver *driver);

	sht_swapchain (*swapchain)(sht_driver driver);
	sht_memory *(*memory)(sht_driver driver);

	sht_sampler (*create_sampler)(sht_driver driver, fck_alias(sht_filter, fckc_u32) filter);
	void (*destroy_sampler)(sht_driver driver, sht_sampler *sampler);
	void (*copy_buffers)(sht_driver driver, sht_buffer *dst, sht_buffer *src);
	void (*upload_buffer)(sht_driver driver, sht_buffer *dst, const void *src, fckc_size_t size);
	void (*upload_image)(sht_driver driver, sht_image *dst, const void *src, fckc_size_t size);

	// Band-aid - prefer actual sync you idiot
	void (*idle)(sht_driver driver);
} sht_driver_vt;

typedef struct sht_instance
{
	sht_handle *handle;
	struct sht_instance_vt *vt;
} sht_instance;

typedef struct sht_instance_vt
{
	sht_driver (*start)(sht_instance instance, struct fck_window *window);
	sht_bool32 (*is_ok)(sht_driver driver);
	void (*unload)(sht_instance *instance);
} sht_instance_vt;

typedef struct sht_render_api
{
	sht_instance (*load)(fckc_u32 version);
	sht_bool32 (*is_ok)(sht_instance instance);
} sht_render_api;

#endif // !SHT_RENDER_H_INCLUDED
