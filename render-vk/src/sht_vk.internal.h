
#ifndef SHT_VK_INTERNAL_H_INCLUDED
#define SHT_VK_INTERNAL_H_INCLUDED

// TODO: Make inline!

#include "sht_render.h"

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan_core.h>
#include <vulkan/vk_enum_string_helper.h>

#include <fck_os.h>
#include <fckc_assert.h>

// I am unsure that that makes sense to have a graphics pipeline limit... We will see
// We might need this or something similar later!

#define sht_vk_render_pass_storage_capacity 32
#define sht_vk_framebuffer_storage_capacity 128
#define sht_vk_capacity 128
#define sht_vk_graphics_pipeline_capacity 64
#define sht_vk_descriptor_pool_capacity 64
#define sht_vk_bss_binding_capacity 8
#define sht_vk_bss_descriptor_set_bind_copies 1024
#define sht_vk_swapchain_image_capacity 8

#define sht_vk_success(vk_result) ((vk_result) == VK_SUCCESS)

#define sht_vk_propagate_on_error(vk_result)                                                                                               \
	{                                                                                                                                      \
		const VkResult _sht_vk_result_ = (vk_result);                                                                                            \
		if (!sht_vk_success(_sht_vk_result_))                                                                                              \
		{                                                                                                                                  \
			return _sht_vk_result_;                                                                                                        \
		}                                                                                                                                  \
	}

static inline VkResult sht_vk_report(VkResult result, const char *msg)
{
	if (sht_vk_success(result))
	{
		return result;
	}

	os->io->log("Error: %s - %s", string_VkResult(result), msg);
	return result;
}

#define sht_vk_assert fck_assert
#define sht_vk_report_defer(sht_vk_report_func) sht_vk_report_func
#define sht_vk_report_(vk_result, func) sht_vk_report(vk_result, func)
#define sht_vk_error(vk_result) (sht_vk_report((vk_result), sht_vk_report_defer(__func__)))
#define sht_vk_crash(vk_result) sht_vk_assert(sht_vk_success(sht_vk_report_((vk_result), sht_vk_report_defer(__func__))))

// Vulkan API loading
#define sht_vk_declare(function_name) PFN_vk##function_name function_name

#define sht_vk_load_function(api_namespace, api_so, api_member)                                                                                    \
	(api_namespace)->api_member = (PFN_vk##api_member)os->so->symbol(api_so, "vk" #api_member)

#define sht_static_assert(condition, note) extern char sht_static_assertion[(condition) ? 1 : -1]

struct sht_vk_gpu;

struct sht_vk_instance;

// CORE QUEUES
typedef struct sht_vk_queues
{
	struct sht_vk_gpu *gpu;
	
	fckc_u32 family[sht_queue_count];
	fckc_u32 primary[sht_queue_count];

	sht_vk_declare(QueueSubmit);
	sht_vk_declare(QueuePresentKHR);
	sht_vk_declare(GetDeviceQueue);
} sht_vk_queues;

typedef struct sht_vk_gpu
{
	struct sht_vk_instance *vk;

	VkPhysicalDevice device;

	sht_vk_queues queues;

	sht_vk_declare(EnumeratePhysicalDevices);
	sht_vk_declare(GetPhysicalDeviceProperties);
	sht_vk_declare(GetPhysicalDeviceFeatures);
	sht_vk_declare(GetPhysicalDeviceMemoryProperties);
	sht_vk_declare(GetPhysicalDeviceQueueFamilyProperties);
	sht_vk_declare(EnumerateDeviceExtensionProperties);
	sht_vk_declare(GetPhysicalDeviceFormatProperties);

	// KHR Extension, but pretty much guranteed lol
	sht_vk_declare(GetPhysicalDeviceSurfaceFormatsKHR);
	sht_vk_declare(GetPhysicalDeviceSurfaceCapabilitiesKHR);
	sht_vk_declare(GetPhysicalDeviceSurfacePresentModesKHR);
	sht_vk_declare(GetPhysicalDeviceSurfaceSupportKHR);
} sht_vk_gpu;

typedef struct sht_vk_platform
{
	// Remove all this... This is kinda useless... I guess
	PFN_vkVoidFunction CreateSurfaceOpaque;
} sht_vk_platform;

typedef struct sht_vk_common_sync_resources
{
	// sht_vk_driver *driver; // Might not be needed

	VkFence wait_fences[sht_frame_count];
	VkSemaphore graphics_completed[sht_frame_count];
	VkSemaphore presentation_completed[sht_frame_count];

	fckc_u32 frame_index_to_swapchain_image_index[sht_frame_count];
	fckc_u32 index;
} sht_vk_common_sync_resources;

struct sht_vk_driver;

typedef struct sht_vk_swapchain
{
	struct sht_vk_driver *driver;

	VkSurfaceKHR surface;
	sht_vk_common_sync_resources sync;
	VkSwapchainKHR swapchain;

	VkSwapchainCreateInfoKHR info;
	VkImage images[sht_vk_swapchain_image_capacity];
	VkImageView views[sht_vk_swapchain_image_capacity];

	fckc_u32 count;

	sht_vk_declare(CreateSwapchainKHR);
	sht_vk_declare(GetSwapchainImagesKHR);
	sht_vk_declare(AcquireNextImageKHR);
	sht_vk_declare(DestroySwapchainKHR);
} sht_vk_swapchain;

typedef struct sht_vk_command
{
	struct sht_vk_driver *driver;

	VkCommandPool pool;
	VkCommandBuffer buffers[sht_frame_count];

	// Present, transfer, copy??? We will see!

	sht_vk_declare(CreateCommandPool);
	sht_vk_declare(DestroyCommandPool);
	sht_vk_declare(AllocateCommandBuffers);
	sht_vk_declare(FreeCommandBuffers);
	sht_vk_declare(BeginCommandBuffer);
	sht_vk_declare(EndCommandBuffer);
	sht_vk_declare(ResetCommandBuffer);
	sht_vk_declare(CmdBindPipeline);
	sht_vk_declare(CmdSetViewport);
	sht_vk_declare(CmdSetScissor);
	sht_vk_declare(CmdSetLineWidth);
	sht_vk_declare(CmdSetDepthBias);
	sht_vk_declare(CmdSetBlendConstants);
	sht_vk_declare(CmdSetDepthBounds);
	sht_vk_declare(CmdSetStencilCompareMask);
	sht_vk_declare(CmdSetStencilWriteMask);
	sht_vk_declare(CmdSetStencilReference);
	sht_vk_declare(CmdBindDescriptorSets);
	sht_vk_declare(CmdBindIndexBuffer);
	sht_vk_declare(CmdBindVertexBuffers);
	sht_vk_declare(CmdDraw);
	sht_vk_declare(CmdDrawIndexed);
	sht_vk_declare(CmdDrawIndirect);
	sht_vk_declare(CmdDrawIndexedIndirect);
	sht_vk_declare(CmdDispatch);
	sht_vk_declare(CmdDispatchIndirect);
	sht_vk_declare(CmdCopyBuffer);
	sht_vk_declare(CmdCopyImage);
	sht_vk_declare(CmdBlitImage);
	sht_vk_declare(CmdCopyBufferToImage);
	sht_vk_declare(CmdCopyImageToBuffer);
	sht_vk_declare(CmdUpdateBuffer);
	sht_vk_declare(CmdFillBuffer);
	sht_vk_declare(CmdClearColorImage);
	sht_vk_declare(CmdClearDepthStencilImage);
	sht_vk_declare(CmdClearAttachments);
	sht_vk_declare(CmdResolveImage);
	sht_vk_declare(CmdSetEvent);
	sht_vk_declare(CmdResetEvent);
	sht_vk_declare(CmdWaitEvents);
	sht_vk_declare(CmdPipelineBarrier);
	sht_vk_declare(CmdBeginQuery);
	sht_vk_declare(CmdEndQuery);
	sht_vk_declare(CmdResetQueryPool);
	sht_vk_declare(CmdWriteTimestamp);
	sht_vk_declare(CmdCopyQueryPoolResults);
	sht_vk_declare(CmdPushConstants);
	sht_vk_declare(CmdBeginRenderPass);
	sht_vk_declare(CmdNextSubpass);
	sht_vk_declare(CmdEndRenderPass);
	sht_vk_declare(CmdExecuteCommands);
} sht_vk_command;

typedef struct sht_vk_graphics_pipeline
{
	VkPipelineCache cache;
	VkPipeline pipeline;
} sht_vk_graphics_pipeline;

typedef struct sht_vk_color_target_desc
{
	sht_format format;
	sht_memory_access_operation load_op;
	sht_memory_access_operation store_op;
	// fckc_f32 clear_value[4];
	//  ...
} sht_vk_color_target_desc;

typedef struct sht_vk_depth_target_desc
{
	sht_format format;
	sht_memory_access_operation load_op;
	sht_memory_access_operation store_op;
	// fckc_f32 clear_value;
	//  ...
} sht_vk_depth_target_desc;

typedef struct sht_vk_render_pass_desc
{
	sht_vk_depth_target_desc depth;
	sht_vk_color_target_desc colour;
} sht_vk_render_pass_desc;

typedef struct sht_vk_render_pass
{
	sht_vk_render_pass_desc desc;
	VkRenderPass handle;
} sht_vk_render_pass;

typedef struct sht_vk_framebuffer
{
	sht_render_desc desc;
	VkFramebuffer handle;
	VkExtent2D extent;
} sht_vk_framebuffer;

typedef struct sht_vk_render_pass_storage
{
	sht_vk_render_pass handles[sht_vk_render_pass_storage_capacity];
	fckc_size_t count;
} sht_vk_render_pass_storage;

typedef struct sht_vk_framebuffer_storage
{
	sht_vk_framebuffer handles[sht_vk_framebuffer_storage_capacity];
	fckc_size_t count;
} sht_vk_framebuffer_storage;

typedef struct sht_graphics_pipeline_key
{
	fckc_u64 invalid : 1;
	fckc_u64 generation : 31;

	fckc_u64 hash : 32;
} sht_graphics_pipeline_key;

typedef struct sht_vk_graphics_pipeline_storage
{
	sht_vk_graphics_pipeline handles[sht_vk_graphics_pipeline_capacity];
	fckc_size_t count;

	sht_graphics_pipeline_key keys[sht_vk_graphics_pipeline_capacity];
} sht_vk_graphics_pipeline_storage;

typedef struct sht_bss_buffer_backends
{
	sht_buffer buffers[sht_vk_bss_binding_capacity];
} sht_bss_buffer_backends;

typedef struct sht_vk_binding_desc
{
	sht_binding bindings[sht_vk_bss_binding_capacity];
	fckc_size_t count;
} sht_vk_binding_desc;

typedef struct sht_vk_descriptor_set_copies
{
	VkDescriptorSet sets[sht_vk_bss_descriptor_set_bind_copies];
	fckc_size_t at;
} sht_vk_descriptor_set_copies;

typedef struct sht_vk_descriptor_pool_storage_entry
{
	VkDescriptorSetLayout layout;
	VkPipelineLayout pipeline_layout;
	VkDescriptorPool dynamic_pools[sht_frame_count];
	VkDescriptorPool constant_pool;

	fckc_u32 ref_count;
} sht_vk_descriptor_pool_storage_entry;

typedef struct sht_vk_descriptor_pool_storage_key
{
	sht_vk_descriptor_pool_storage_entry *entry;
} sht_vk_descriptor_pool_storage_key;

typedef struct sht_vk_bss_node
{
	struct sht_vk_bss_nodes *next;
	struct sht_vk_bss_nodes *prev;
} sht_vk_bss_node;

typedef struct sht_vk_bss_nodes
{
	// TODO: Maybe embed tagged info in here!
	sht_vk_bss_node values[sht_frame_count];
} sht_vk_bss_nodes;

typedef struct sht_vk_bss
{
	// Header
	sht_vk_bss_nodes nodes;

	// Data
	sht_vk_binding_desc desc;
	// VkDescriptorPool pool;
	// VkDescriptorSetLayout layout;
	// VkPipelineLayout pipeline_layout; // Idk man. I REALLY DO NOT KNOW
	sht_vk_descriptor_pool_storage_key pool_storage_key;
	// sht_vk_descriptor_set_copies copies[SHT_VK_IMAGE_COUNT];
	//  VkDescriptorSet sets[SHT_VK_IMAGE_COUNT];
	VkDescriptorSet latest[sht_frame_count];
	VkDescriptorSet baselines[sht_frame_count];
	sht_bss_buffer_backends buffer_backends[sht_frame_count];
} sht_vk_bss;

typedef struct sht_vk_bss_storage
{
	sht_vk_bss_nodes inflight;

	sht_vk_bss handles[sht_vk_capacity];
	fckc_size_t count;
} sht_vk_bss_storage;

typedef struct sht_vk_descriptor_pool_storage
{
	sht_vk_descriptor_pool_storage_entry entries[sht_vk_descriptor_pool_capacity];
	fckc_size_t count;
} sht_vk_descriptor_pool_storage;

typedef struct sht_resource_storages
{
	sht_vk_bss_storage bss;
	sht_vk_render_pass_storage render_pass;
	sht_vk_framebuffer_storage framebuffer;
	sht_vk_graphics_pipeline_storage graphics_pipeline;
	sht_vk_descriptor_pool_storage descriptor_pool;
} sht_resource_storages;

typedef struct sht_vk_driver
{
	sht_vk_gpu *gpu; // does that make sense?
	VkDevice device;

	// Render pass here does not make sense...
	sht_vk_declare(CreateDevice);
	sht_vk_declare(DestroyDevice);
	sht_vk_declare(DestroySurfaceKHR);

	sht_vk_declare(GetImageMemoryRequirements);
	sht_vk_declare(GetBufferMemoryRequirements);

	sht_vk_declare(CreateImageView);
	sht_vk_declare(CreateImage);
	sht_vk_declare(DestroyImageView);
	sht_vk_declare(DestroyImage);

	sht_vk_declare(CreateRenderPass);
	sht_vk_declare(DestroyRenderPass);
	sht_vk_declare(BindImageMemory);

	sht_vk_declare(CreateBuffer);
	sht_vk_declare(DestroyBuffer);
	sht_vk_declare(AllocateMemory);
	sht_vk_declare(FreeMemory);
	sht_vk_declare(BindBufferMemory);

	sht_vk_declare(MapMemory);
	sht_vk_declare(UnmapMemory);

	sht_vk_declare(CreateDescriptorSetLayout);
	sht_vk_declare(CreateDescriptorPool);
	sht_vk_declare(UpdateDescriptorSets);
	sht_vk_declare(ResetDescriptorPool);
	sht_vk_declare(AllocateDescriptorSets);
	sht_vk_declare(FreeDescriptorSets);
	sht_vk_declare(DestroyDescriptorPool);
	sht_vk_declare(DestroyDescriptorSetLayout);

	sht_vk_declare(CreateShaderModule);
	sht_vk_declare(DestroyShaderModule);

	sht_vk_declare(CreatePipelineLayout);
	sht_vk_declare(DestroyPipelineLayout);

	sht_vk_declare(CreateGraphicsPipelines);
	sht_vk_declare(CreatePipelineCache);

	sht_vk_declare(DestroyPipeline);
	sht_vk_declare(DestroyPipelineCache);

	sht_vk_declare(CreateSemaphore);
	sht_vk_declare(DestroySemaphore);

	sht_vk_declare(CreateFence);
	sht_vk_declare(DestroyFence);
	sht_vk_declare(ResetFences);
	sht_vk_declare(WaitForFences);
	sht_vk_declare(GetFenceStatus);

	sht_vk_declare(DeviceWaitIdle);

	sht_vk_declare(CreateSampler);
	sht_vk_declare(DestroySampler);

	sht_vk_declare(CreateFramebuffer);
	sht_vk_declare(DestroyFramebuffer);

	sht_vk_command command;
	sht_vk_swapchain swapchain;
	sht_memory memory;
	sht_resource_storages storages;
	fck_window window;
} sht_vk_driver;

/* Why the weird structure with pointers to parents and then back down?
 * Well, unlike a big monolith, I decided to split it up to understand for the future
 * what level of fine-grain-ness I need in specific functions.
 * Some problems are only solvable with a complete image of the data at hand. For example:
 * Sometimes we need driver functionality, but we also need to deal with the swapchain.
 * In this example we can either pass the driver or the data we need from the swapchain
 * or we pass down the driver AND the swapchain or we ONLY pass the swapchain
 * It is a bit weird, helps navigation though! */
typedef struct sht_vk_instance
{
	VkInstance instance;
	sht_vk_declare(CreateInstance);
	sht_vk_declare(DestroyInstance);

	fckc_char *name;
	struct kll_allocator *allocator;

	sht_vk_gpu gpu;
	sht_vk_platform platform;
	sht_vk_driver driver;
	fck_shared_object so;
} sht_vk_instance;

void sht_vk_platform_adjust_instance(VkInstanceCreateInfo* create_info);
void sht_vk_platform_adjust_extensions(const char** instance_extension_names, fckc_size_t* count);

VkResult sht_vk_platform_init(sht_vk_instance *vk, sht_vk_platform *platform, sht_vk_gpu *gpu, fck_window window,
                              VkSurfaceKHR *out_surface);
#endif // !SHT_VK_INTERNAL_H_INCLUDED