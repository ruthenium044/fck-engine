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
#define sht_frame_count 4

typedef fckc_u32 sht_bool32;
#define sht_true 1
#define sht_false 0

typedef void sht_heap;
typedef void sht_handle;
typedef fckc_u64 sht_id;

typedef enum sht_queue_type
{
	sht_queue_graphic,
	sht_queue_present,
	sht_queue_transfer,
	sht_queue_compute,
	sht_queue_count,
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
	sht_memory_gpu,
	sht_memory_cpu,
	sht_memory_count
} sht_memory_type;

typedef enum sht_transfer_flags
{
	sht_transfer_retained = 0x00,
	sht_trasnfer_source = 0x01,
	sht_transfer_target = 0x02,
} sht_transfer_flags;

typedef enum sht_buffer_usage_flags
{
	sht_buffer_usage_uniform = 0x0000001,
	sht_buffer_usage_storage = 0x0000002,
	sht_buffer_usage_index = 0x0000004,
	sht_buffer_usage_vertex = 0x0000008,
	sht_buffer_usage_indirect = 0x0000010,
} sht_buffer_usage_flags;

typedef enum sht_image_usage_flags
{
	sht_image_usage_sampled = 0x00000001,
	sht_image_usage_storage = 0x00000002,
	sht_image_usage_color_attachment = 0x00000004,
	sht_image_usage_depth_stencil_attachment = 0x00000008,
	sht_image_usage_input_attachment = 0x00000010,
	// SHT_IMAGE_USAGE_TRANSIENT_ATTACHMENT = 0x00000010, // Not now!
} sht_image_usage_flags;

typedef enum sht_memory_access_operation
{
	sht_load = 0,
	sht_store = 0,
	sht_dont_care = 1, // Maybe, discard?
	sht_clear = 2,
} sht_memory_access_operation;

typedef enum sht_layout_type
{
	sht_layout_undefined = 0,
	sht_layout_general = 1,
	sht_layout_color_attachment = 2,
	sht_layout_depth_stencil_attachment = 3,
	sht_layout_depth_stencil_read_only = 4,
	sht_layout_shader_read_only = 5,
	sht_layout_present = 1000001002,
} sht_layout_type;

typedef enum sht_stage_flags
{
	sht_stage_vertex_shader = 0x00000001,
	sht_stage_fragment_shader = 0x00000002,
} sht_stage_flags;

typedef enum sht_access_flags
{
	sht_access_indirect_command_read = 0x00000001,
	sht_access_index_read = 0x00000002,
	sht_access_vertex_attribute_read = 0x00000004,
	sht_access_uniform_read = 0x00000008,
	sht_access_input_attachment_Read = 0x00000010,
	sht_access_shader_read = 0x00000020,
	sht_access_shader_write = 0x00000040,
	sht_access_color_attachment_read = 0x00000080,
	sht_access_color_attachment_write = 0x00000100,
	sht_access_depth_stencil_attachment_read = 0x00000200,
	sht_access_depth_stencil_attachment_write = 0x00000400,
	sht_access_transfer_read = 0x00000800,
	sht_access_transfer_write = 0x00001000,
	sht_access_host_read = 0x00002000,
	sht_access_host_write = 0x00004000,
	sht_access_memory_read = 0x00008000,
	sht_access_memory_write = 0x00010000,
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
		.transfer = sht_trasnfer_source, .usage = (usage_flags), .size = (memory_size)                                                     \
	}

#define sht_buffer_target(usage_flags, memory_size)                                                                                        \
	(sht_buffer_configuration)                                                                                                             \
	{                                                                                                                                      \
		.transfer = sht_transfer_target, .usage = (usage_flags), .size = (memory_size)                                                     \
	}

#define sht_buffer_retained(usage_flags, memory_size)                                                                                      \
	(sht_buffer_configuration)                                                                                                             \
	{                                                                                                                                      \
		.transfer = sht_transfer_retained, .usage = (usage_flags), .size = (memory_size)                                                   \
	}

typedef enum sht_format
{
	// Rename them into something friendlier!
	sht_format_undefined = 0,
	sht_format_r8g8b8a8_unorm = 1,
	sht_format_r8g8b8a8_srgb = 2,
	sht_format_b8g8r8a8_unorm = 3,
	sht_format_b8g8r8a8_srgb = 4,
	sht_format_d16_unorm = 5,

	sht_format_r32_sfloat = 418,
	sht_format_r32_G32_sfloat = 419,
	sht_format_r32g32b32_sfloat = 420,
	sht_format_r32g32b32a32_sfloat = 421,
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

	void *cpu[sht_memory_count];
	fckc_size_t offset[sht_memory_count];
	fckc_size_t capacity[sht_memory_count];
	sht_heap *heaps[sht_memory_count];
} sht_memory_arena;

typedef struct sht_memory_image
{
	sht_image (*create)(sht_memory_arena *mem, const sht_image_configuration *config, sht_memory_type memory_type);
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
	sht_swapchain_first_index = 0,
	sht_swapchain_last_index = 127,
	sht_swapchain_issues = 1 << 31,
	sht_swapchain_needs_resize = sht_swapchain_issues | 1,
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
	sht_triangle_list,
} sht_topology_type;

typedef enum sht_cull_mode_flags
{
	sht_cull_mode_none = 0x0,
	sht_cull_mode_front = 0x1,
	sht_cull_mode_back = 0x2,
	sht_cull_mode_both = 0x3,
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
	sht_binding_none = 0,
	sht_binding_uniform, // Maybe this one is not needed if STORAGE can fulfill the same role!!!
	sht_binding_storage, // Currently only readonly? Feature vertexPipelineStoresAndAtomics enabled? Maybe only relevant for vertex stage
	sht_binding_readonly_image,
	// TODO: sampler and all that bs
	sht_binding_count,
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

	void (*draw_indexed)(sht_command_buffer command, const sht_draw_indexed_desc *params);

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
