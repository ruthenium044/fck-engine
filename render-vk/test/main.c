// #include "SDL3/SDL_vulkan.h"

#include "public.h"

#include "fckc_math.h"
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_metal.h>

#include <vulkan/utility/vk_format_utils.h>
#include <vulkan/vk_enum_string_helper.h>

// #include <MoltenVK/mvk_vulkan.h>

#include <fck_os.h>

#include <ApplicationServices/ApplicationServices.h>
#include <objc/message.h>

#include <fck_events.h>
#include <kll.h>
#include <kll_heap.h>

#include <kll_malloc.h>

#include <fckc_assert.h>
#include <fckc_inttypes.h>

#include <fck_shader.h>

#include <dlfcn.h>
#include <memory.h>

#include "sht_loader.h"

// #define fck_bytes(x) ((fckc_size_t)(x))
#define fck_kilobytes(x) ((fckc_size_t)(x) * 1024UL)
#define fck_megabytes(x) ((fckc_size_t)(x) * 1024UL * 1024UL)
#define fck_gigabytes(x) ((fckc_size_t)(x) * 1024UL * 1024UL * 1024UL)

// Vulkan API loading
#define fck_vk_declare(function_name) PFN_vk##function_name function_name
#define fck_vk_load_function(api_namespace, api_member)                                                                                    \
	(api_namespace)->api_member = (PFN_vk##api_member)dlsym(RTLD_DEFAULT, "vk" #api_member)

#define fck_vk_sucess(vk_result) ((vk_result) == VK_SUCCESS)
#define fck_vk_failure(vk_result) (!(fck_vk_sucess(vk_result)))
#define fck_vk_failure(vk_result) (!(fck_vk_sucess(vk_result)))
#define fck_vk_propagate_on_error(vk_result)                                                                                               \
	{                                                                                                                                      \
		VkResult _fck_vk_result_ = (vk_result);                                                                                            \
		if (fck_vk_failure(_fck_vk_result_))                                                                                               \
		{                                                                                                                                  \
			return _fck_vk_result_;                                                                                                        \
		}                                                                                                                                  \
	}

typedef enum fck_vk_queue_type
{
	FCK_VK_QUEUE_GRAPHIC,
	FCK_VK_QUEUE_PRESENT,
	FCK_VK_QUEUE_TRANSFER,
	FCK_VK_QUEUE_COUNT,
} fck_vk_queue_type;

#define FCK_VK_IMAGE_COUNT 4

VkFormat sht_vk_format_from_sht_format(sht_format format);

// TODO: Obviously I need to tear this whole shit down and re-create it

// TODO: This will become fck_window!!

typedef struct fck_vk_instance
{
	// This one is so small, I do not like it
	VkInstance instance;

	fck_vk_declare(CreateInstance);
	fck_vk_declare(DestroyInstance);
} fck_vk_instance;

typedef struct fck_vk_platform
{
#if defined(__APPLE__)
	fck_vk_declare(CreateMetalSurfaceEXT);
#endif
} fck_vk_platform;

typedef struct fck_vk_gpu
{
	fck_vk_instance *vk; // does that make sense?

	VkPhysicalDevice device;

	fck_vk_declare(EnumeratePhysicalDevices);
	fck_vk_declare(GetPhysicalDeviceProperties);
	fck_vk_declare(GetPhysicalDeviceFeatures);
	fck_vk_declare(GetPhysicalDeviceMemoryProperties);
	fck_vk_declare(GetPhysicalDeviceQueueFamilyProperties);
	fck_vk_declare(EnumerateDeviceExtensionProperties);
	fck_vk_declare(GetPhysicalDeviceFormatProperties);

	// KHR Extension, but pretty much guranteed lol
	fck_vk_declare(GetPhysicalDeviceSurfaceFormatsKHR);
	fck_vk_declare(GetPhysicalDeviceSurfaceCapabilitiesKHR);
	fck_vk_declare(GetPhysicalDeviceSurfacePresentModesKHR);
	fck_vk_declare(GetPhysicalDeviceSurfaceSupportKHR);
} fck_vk_gpu;

// TODO: Replace this with sht_vk_image!!
// typedef struct fck_vk_image
//{
//	fck_alias(VkFormat, fckc_u32) format;
//	fckc_u16 width;
//	fckc_u16 height;
//
//	VkImage gpu;           // This might be a bit weiiird
//	VkDeviceMemory memory; // ... This can be NULL
//} fck_vk_image;
/*
typedef struct fck_vk_image_info
{
    fck_alias(VkFormat, fckc_u32) format;
    fckc_u16 width;
    fckc_u16 height;
} fck_vk_image_info;

typedef struct fck_vk_image_view
{
    fck_vk_image_info info;
    VkImageView view;
} fck_vk_image_view;

typedef struct fck_vk_image_view_chain
{
    fck_vk_image_info info;

    VkImageView view[FCK_VK_IMAGE_COUNT];
} fck_vk_image_view_chain;

typedef union fck_vk_image_generic {
    fck_vk_image_info info;

    fck_vk_image_view view;
    fck_vk_image_view_chain chain;
} fck_vk_image_generic;*/

// typedef struct fck_vk_image_view
//{
//	VkImageView view;
// }fck_vk_image_view;

typedef struct fck_vk_queues
{
	fck_vk_gpu *gpu;

	fckc_u32 family[FCK_VK_QUEUE_COUNT];
	fckc_u32 primary[FCK_VK_QUEUE_COUNT];

	fck_vk_declare(QueueSubmit);
	fck_vk_declare(QueuePresentKHR);
	fck_vk_declare(GetDeviceQueue);
} fck_vk_queues;

typedef struct fck_vk_driver
{
	fck_vk_gpu *gpu; // does that make sense?

	VkDevice device;

	// Render pass here does not make sense...
	// Render pass here does not make sense...
	fck_vk_declare(CreateDevice);
	fck_vk_declare(GetImageMemoryRequirements);
	fck_vk_declare(GetBufferMemoryRequirements);
	fck_vk_declare(CreateImageView);
	fck_vk_declare(CreateImage);
	fck_vk_declare(DestroyImageView);
	fck_vk_declare(DestroyImage);
	fck_vk_declare(CreateRenderPass);
	fck_vk_declare(DestroyRenderPass);
	fck_vk_declare(BindImageMemory);
	fck_vk_declare(CreateBuffer);
	fck_vk_declare(DestroyBuffer);
	fck_vk_declare(AllocateMemory);
	fck_vk_declare(FreeMemory);
	fck_vk_declare(BindBufferMemory);
	fck_vk_declare(MapMemory);
	fck_vk_declare(UnmapMemory);
	fck_vk_declare(CreateDescriptorSetLayout);
	fck_vk_declare(CreateDescriptorPool);
	fck_vk_declare(UpdateDescriptorSets);
	fck_vk_declare(AllocateDescriptorSets);
	fck_vk_declare(CreateShaderModule);
	fck_vk_declare(CreatePipelineLayout);
	fck_vk_declare(CreateGraphicsPipelines);
	fck_vk_declare(CreatePipelineCache);
	fck_vk_declare(CreateSemaphore);
	fck_vk_declare(DestroySemaphore);
	fck_vk_declare(CreateFence);
	fck_vk_declare(DestroyFence);
	fck_vk_declare(ResetFences);
	fck_vk_declare(WaitForFences);
	fck_vk_declare(GetFenceStatus);
	fck_vk_declare(DeviceWaitIdle);
	fck_vk_declare(CreateFramebuffer);
	fck_vk_declare(DestroyFramebuffer);
} fck_vk_driver;

typedef struct fck_vk_swapchain
{
	fck_vk_driver *driver; // does that make sense?

	VkSwapchainCreateInfoKHR create_info;
	VkFormat format;
	VkSwapchainKHR swapchain;

	VkImageView views[FCK_VK_IMAGE_COUNT];
	fckc_size_t count;

	fck_vk_declare(CreateSwapchainKHR);
	fck_vk_declare(GetSwapchainImagesKHR);
	fck_vk_declare(AcquireNextImageKHR);

} fck_vk_swapchain;

typedef struct fck_vk_command
{
	fck_vk_driver *driver;

	VkCommandPool pool;
	VkCommandBuffer buffer[FCK_VK_IMAGE_COUNT];
	// Present, transfer, copy??? We will see!

	fck_vk_declare(CreateCommandPool);
	fck_vk_declare(DestroyCommandPool);
	fck_vk_declare(AllocateCommandBuffers);
	fck_vk_declare(FreeCommandBuffers);
	fck_vk_declare(BeginCommandBuffer);
	fck_vk_declare(EndCommandBuffer);
	fck_vk_declare(ResetCommandBuffer);
	fck_vk_declare(CmdBindPipeline);
	fck_vk_declare(CmdSetViewport);
	fck_vk_declare(CmdSetScissor);
	fck_vk_declare(CmdSetLineWidth);
	fck_vk_declare(CmdSetDepthBias);
	fck_vk_declare(CmdSetBlendConstants);
	fck_vk_declare(CmdSetDepthBounds);
	fck_vk_declare(CmdSetStencilCompareMask);
	fck_vk_declare(CmdSetStencilWriteMask);
	fck_vk_declare(CmdSetStencilReference);
	fck_vk_declare(CmdBindDescriptorSets);
	fck_vk_declare(CmdBindIndexBuffer);
	fck_vk_declare(CmdBindVertexBuffers);
	fck_vk_declare(CmdDraw);
	fck_vk_declare(CmdDrawIndexed);
	fck_vk_declare(CmdDrawIndirect);
	fck_vk_declare(CmdDrawIndexedIndirect);
	fck_vk_declare(CmdDispatch);
	fck_vk_declare(CmdDispatchIndirect);
	fck_vk_declare(CmdCopyBuffer);
	fck_vk_declare(CmdCopyImage);
	fck_vk_declare(CmdBlitImage);
	fck_vk_declare(CmdCopyBufferToImage);
	fck_vk_declare(CmdCopyImageToBuffer);
	fck_vk_declare(CmdUpdateBuffer);
	fck_vk_declare(CmdFillBuffer);
	fck_vk_declare(CmdClearColorImage);
	fck_vk_declare(CmdClearDepthStencilImage);
	fck_vk_declare(CmdClearAttachments);
	fck_vk_declare(CmdResolveImage);
	fck_vk_declare(CmdSetEvent);
	fck_vk_declare(CmdResetEvent);
	fck_vk_declare(CmdWaitEvents);
	fck_vk_declare(CmdPipelineBarrier);
	fck_vk_declare(CmdBeginQuery);
	fck_vk_declare(CmdEndQuery);
	fck_vk_declare(CmdResetQueryPool);
	fck_vk_declare(CmdWriteTimestamp);
	fck_vk_declare(CmdCopyQueryPoolResults);
	fck_vk_declare(CmdPushConstants);
	fck_vk_declare(CmdBeginRenderPass);
	fck_vk_declare(CmdNextSubpass);
	fck_vk_declare(CmdEndRenderPass);
	fck_vk_declare(CmdExecuteCommands);
} fck_vk_command;

typedef struct fck_mvp
{
	fckc_f32 model[4][4];
	fckc_f32 view[4][4];
	fckc_f32 projection[4][4];
} fck_mvp;

typedef struct fck_vertex
{
	float position[3];
	float color[3];
} fck_vertex;

typedef struct fck_vk_framebuffer
{
	VkFramebuffer values[FCK_VK_IMAGE_COUNT];
	fckc_size_t count;
} fck_vk_framebuffer;

typedef struct fck_vk_runtime
{
	fck_mvp mvp;

	fck_window window;
	VkSurfaceKHR surface;
	VkRenderPass render_pass;

	fckc_size_t index;

	sht_elements vertices;
	sht_elements indices;

	sht_buffer uniform[FCK_VK_IMAGE_COUNT];
	VkDescriptorSet descs[FCK_VK_IMAGE_COUNT];

	VkFence wait_fences[FCK_VK_IMAGE_COUNT];
	VkSemaphore graphics_completed[FCK_VK_IMAGE_COUNT];
	VkSemaphore presentation_completed[FCK_VK_IMAGE_COUNT];

	fck_vk_framebuffer framebuffer;
	// VkFramebuffer framebuffers[FCK_VK_IMAGE_COUNT];
	sht_image depth;
	sht_image_view depth_view;
} fck_vk_runtime;

typedef struct fck_vk_graphics_pipeline
{
	VkPipelineLayout layout;
	VkPipelineCache cache;
	VkPipeline pipeline;
} fck_vk_graphics_pipeline;

VkResult fck_vk_report(VkResult result, const char *msg)
{
	if (fck_vk_sucess(result))
	{
		return result;
	}

	os->io->log("Error: %s - %s", string_VkResult(result), msg);
	return result;
}

// This should be a crash, not assert. If we cannot get further we are fucked
#define fck_vk_assert fck_assert
#define fck_vk_report_defner(fck_vk_report_func) fck_vk_report_func
#define fck_vk_report_(vk_result, func) fck_vk_report(vk_result, func)
#define fck_vk_error(vk_result) fck_vk_failure(fck_vk_report((vk_result), fck_vk_report_defner(__func__)))
#define fck_vk_crash(vk_result) fck_vk_assert(fck_vk_sucess(fck_vk_report_((vk_result), fck_vk_report_defner(__func__))))

// Stolen from good ol VK, maybe it is better... probs
// Find a memory in `memoryTypeBitsRequirement` that includes all of `requiredProperties`
VkBool32 fck_vk_query_memory_type_index(const VkPhysicalDeviceMemoryProperties *properties, uint32_t mem_type_bits,
                                        VkMemoryPropertyFlags flags, uint32_t *type_index)
{
	const uint32_t count = properties->memoryTypeCount;
	for (uint32_t index = 0; index < count; ++index)
	{
		const uint32_t memory_type_bits = (1 << index);
		const bool is_required = mem_type_bits & memory_type_bits;

		const VkMemoryPropertyFlags props = properties->memoryTypes[index].propertyFlags;
		const bool has_requirements = (props & flags) == flags;

		if (is_required && has_requirements)
		{
			*type_index = index;
			return VK_TRUE;
		}
	}

	// failed to find memory type
	return VK_FALSE;
}

VkResult sht_vk_memory_init(sht_memory *mem, struct sht_vk_driver *driver, fckc_size_t size);

typedef struct fck_vk
{
	fck_vk_instance vk;
	fck_vk_platform platform;
	fck_vk_gpu gpu;
	fck_vk_queues queues;
	fck_vk_driver driver;
	fck_vk_swapchain swapchain;
	fck_vk_command command;

	sht_memory memory;

	fck_vk_graphics_pipeline graphic_pipeline;

	fck_vk_runtime runtime;
} fck_vk;

typedef enum fck_macos_result
{
	FCK_MACOS_RESULT_CONTINUE,
	FCK_MACOS_RESULT_DONE,
} fck_macos_result;

typedef struct fck_macos_application
{
	fck_vk vk;
	fck_event_channel event_channel;
} fck_macos_application;

VkPhysicalDevice fck_vk_physical_device_by_name(fck_vk_gpu *gpu, const char *target)
{
	VkPhysicalDevice phy_devices[16]; // There is no fucking way...
	fckc_u32 phy_device_count = fck_arraysize(phy_devices);
	if (fck_vk_error(gpu->EnumeratePhysicalDevices(gpu->vk->instance, &phy_device_count, phy_devices)))
	{
	}
	// FIND BY NAME:
	for (fckc_u32 index = 0; index < phy_device_count; index++)
	{
		VkPhysicalDevice physical_device = phy_devices[index];
		VkPhysicalDeviceProperties props = {0};
		gpu->GetPhysicalDeviceProperties(physical_device, &props);
		if (os->str->unsafe->cmp(props.deviceName, target) == 0)
		{
			return physical_device;
		}
	}
	return VK_NULL_HANDLE;
}

VkResult fck_vk_instance_init(fck_vk_instance *vk)
{
	VkApplicationInfo appInfo;
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pNext = NULL;
	appInfo.pApplicationName = NULL;
	appInfo.applicationVersion = 0;
	appInfo.pEngineName = "sht-vk";
	appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
	appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);

	fck_vk_load_function(vk, CreateInstance);
	fck_vk_load_function(vk, DestroyInstance);

	VkInstanceCreateInfo instance_create_info;
	const char *instance_extension_names[16];
	fckc_size_t instance_extension_count = 0;
	instance_create_info.flags = 0;

	static const char *layer_names[] = {"VK_LAYER_KHRONOS_validation"};

	instance_extension_names[instance_extension_count++] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
	instance_create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

	instance_extension_names[instance_extension_count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
	instance_extension_names[instance_extension_count++] = VK_KHR_SURFACE_EXTENSION_NAME;
	instance_extension_names[instance_extension_count++] = VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME;
	instance_extension_names[instance_extension_count++] = VK_EXT_METAL_SURFACE_EXTENSION_NAME;
	instance_extension_names[instance_extension_count++] = VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME;

	instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_create_info.pNext = NULL;
	instance_create_info.pApplicationInfo = &appInfo;
	instance_create_info.ppEnabledLayerNames = layer_names;
	instance_create_info.enabledExtensionCount = instance_extension_count;
	instance_create_info.ppEnabledExtensionNames = instance_extension_names;
	instance_create_info.enabledLayerCount = fck_arraysize(layer_names);

	return fck_vk_error(vk->CreateInstance(&instance_create_info, NULL, &vk->instance));
}

VkResult fck_vk_gpu_init(fck_vk_gpu *gpu, fck_vk_instance *vk)
{
	gpu->vk = vk;

	fck_vk_load_function(gpu, EnumeratePhysicalDevices);
	fck_vk_load_function(gpu, GetPhysicalDeviceProperties);
	fck_vk_load_function(gpu, GetPhysicalDeviceFeatures);
	fck_vk_load_function(gpu, GetPhysicalDeviceMemoryProperties);
	fck_vk_load_function(gpu, GetPhysicalDeviceQueueFamilyProperties);
	fck_vk_load_function(gpu, GetPhysicalDeviceFormatProperties);

	fck_vk_load_function(gpu, EnumerateDeviceExtensionProperties);
	fck_vk_load_function(gpu, GetPhysicalDeviceSurfaceSupportKHR);
	fck_vk_load_function(gpu, GetPhysicalDeviceSurfacePresentModesKHR);
	fck_vk_load_function(gpu, GetPhysicalDeviceSurfaceFormatsKHR);
	fck_vk_load_function(gpu, GetPhysicalDeviceSurfaceCapabilitiesKHR);

	return VK_SUCCESS;
}

VkBool32 fck_vk_gpu_select(fck_vk_gpu *gpu, const char *name)
{
	gpu->device = fck_vk_physical_device_by_name(gpu, name);
	if (gpu->device == VK_NULL_HANDLE)
	{
		return VK_FALSE;
	}
	return VK_TRUE;
}

VkResult fck_vk_surface_size(fck_vk_gpu *gpu, fck_vk_runtime *runtime, VkExtent2D *extent)
{
	VkSurfaceCapabilitiesKHR surfaceCaps;
	VkResult result = fck_vk_error(gpu->GetPhysicalDeviceSurfaceCapabilitiesKHR(gpu->device, runtime->surface, &surfaceCaps));
	if (result != VK_SUCCESS)
	{
		return result;
	}

	int width;
	int height;
	if (!os->win->size(runtime->window, &width, &height))
	{
		return VK_NOT_READY;
	}

	// If width (and height) equals the special value 0xFFFFFFFF, the size of the surface will be set by the swapchain
	if (surfaceCaps.currentExtent.width == (uint32_t)-1)
	{
		// If the surface size is undefined, the size is set to the size of the images requested
		extent->width = width;
		extent->height = height;
	}
	else
	{
		// If the surface size is defined, the swap chain size must match
		// swapchainExtent = surfaceCaps.currentExtent;
		extent->width = surfaceCaps.currentExtent.width;
		extent->height = surfaceCaps.currentExtent.height;
	}
	return VK_SUCCESS;
}

VkResult fck_vk_queues_init(fck_vk_queues *queues, fck_vk_gpu *gpu, VkSurfaceKHR surface)
{
	queues->gpu = gpu;

	fck_vk_load_function(queues, QueueSubmit);
	fck_vk_load_function(queues, QueuePresentKHR);
	fck_vk_load_function(queues, GetDeviceQueue);

	VkQueueFamilyProperties queue_family_properties[8];
	fckc_u32 queue_family_capacity = fck_arraysize(queue_family_properties);
	fckc_u32 queue_family_count;
	gpu->GetPhysicalDeviceQueueFamilyProperties(gpu->device, &queue_family_count, NULL);
	fck_assert(queue_family_count <= queue_family_capacity);
	fck_assert(queue_family_count >= 1);

	gpu->GetPhysicalDeviceQueueFamilyProperties(gpu->device, &queue_family_count, queue_family_properties);
	fck_assert(queue_family_count >= 1);

	const fckc_u32 invalid_queue_family = (fckc_u32)(-1);
	queues->family[FCK_VK_QUEUE_GRAPHIC] = invalid_queue_family;
	queues->family[FCK_VK_QUEUE_TRANSFER] = invalid_queue_family;
	queues->family[FCK_VK_QUEUE_PRESENT] = invalid_queue_family;

	for (fckc_size_t i = 0; i < queue_family_count; i++)
	{
		if (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			queues->family[FCK_VK_QUEUE_GRAPHIC] = i;
			break;
		}
	}
	fck_assert(queues->family[FCK_VK_QUEUE_GRAPHIC] != invalid_queue_family);

	for (unsigned int i = 0; i < queue_family_count; i++)
	{
		if (queue_family_properties[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
		{
			queues->family[FCK_VK_QUEUE_TRANSFER] = i;
			break;
		}
	}
	fck_assert(queues->family[FCK_VK_QUEUE_TRANSFER] != invalid_queue_family);

	for (unsigned int i = 0; i < queue_family_count; i++)
	{
		VkBool32 supports_present;
		VkResult result = fck_vk_error(gpu->GetPhysicalDeviceSurfaceSupportKHR(gpu->device, i, surface, &supports_present));
		if (fck_vk_failure(result))
		{
			supports_present = VK_FALSE;
		}
		if (supports_present)
		{
			queues->family[FCK_VK_QUEUE_PRESENT] = i;
			break;
		}
	}
	fck_assert(queues->family[FCK_VK_QUEUE_PRESENT] != invalid_queue_family);
	return VK_SUCCESS;
};

fckc_u32 fck_vk_queues_get_family(fck_vk_queues const *queues, fck_vk_queue_type type)
{
	return queues->family[type];
}

VkQueue fck_vk_queues_get_primary(fck_vk_queues const *queues, VkDevice device, fck_vk_queue_type type)
{
	VkQueue queue;
	queues->GetDeviceQueue(device, queues->family[type], queues->primary[type], &queue);
	return queue;
}

VkResult fck_vk_driver_init(fck_vk_driver *driver, fck_vk_queues *queues)
{
	driver->gpu = queues->gpu;

	fck_vk_load_function(driver, CreateDevice);
	fck_vk_load_function(driver, CreateShaderModule);
	fck_vk_load_function(driver, CreateImage);
	fck_vk_load_function(driver, DestroyImageView);
	fck_vk_load_function(driver, DestroyImage);
	fck_vk_load_function(driver, GetImageMemoryRequirements);
	fck_vk_load_function(driver, AllocateMemory);
	fck_vk_load_function(driver, FreeMemory);
	fck_vk_load_function(driver, BindImageMemory);
	fck_vk_load_function(driver, CreateImageView);
	fck_vk_load_function(driver, CreateBuffer);
	fck_vk_load_function(driver, DestroyBuffer);
	fck_vk_load_function(driver, CreateRenderPass);
	fck_vk_load_function(driver, GetBufferMemoryRequirements);
	fck_vk_load_function(driver, BindBufferMemory);
	fck_vk_load_function(driver, MapMemory);
	fck_vk_load_function(driver, UnmapMemory);
	fck_vk_load_function(driver, CreateSemaphore);
	fck_vk_load_function(driver, DestroySemaphore);
	fck_vk_load_function(driver, CreateFence);

	fck_vk_load_function(driver, CreateDescriptorSetLayout);
	fck_vk_load_function(driver, CreateDescriptorPool);
	fck_vk_load_function(driver, UpdateDescriptorSets);
	fck_vk_load_function(driver, AllocateDescriptorSets);

	fck_vk_load_function(driver, CreatePipelineLayout);
	fck_vk_load_function(driver, CreateGraphicsPipelines);
	fck_vk_load_function(driver, CreatePipelineCache);

	// The good shit
	fck_vk_load_function(driver, CreateFence);
	fck_vk_load_function(driver, DestroyFence);
	fck_vk_load_function(driver, ResetFences);
	fck_vk_load_function(driver, WaitForFences);
	fck_vk_load_function(driver, GetFenceStatus);

	fck_vk_load_function(driver, DeviceWaitIdle);

	fck_vk_load_function(driver, CreateFramebuffer);
	fck_vk_load_function(driver, DestroyFramebuffer);

	VkPhysicalDeviceProperties properties;
	VkPhysicalDeviceFeatures features;
	VkPhysicalDeviceMemoryProperties memory_properties;
	driver->gpu->GetPhysicalDeviceProperties(driver->gpu->device, &properties);
	driver->gpu->GetPhysicalDeviceFeatures(driver->gpu->device, &features);
	driver->gpu->GetPhysicalDeviceMemoryProperties(driver->gpu->device, &memory_properties);

	float queue_priorities[1] = {0.0};
	VkDeviceQueueCreateInfo queue_info = {};
	queue_info.queueFamilyIndex = fck_vk_queues_get_family(queues, FCK_VK_QUEUE_GRAPHIC);
	queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_info.pNext = NULL;
	queue_info.queueCount = 1;
	queue_info.pQueuePriorities = queue_priorities;

	const char *instance_extension_names[] = {"VK_KHR_portability_subset", VK_KHR_SWAPCHAIN_EXTENSION_NAME};
	// instance_extension_names[instance_extension_count++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

	VkDeviceCreateInfo device_info = {};
	// Deprecated
	device_info.enabledLayerCount = 0;
	device_info.ppEnabledLayerNames = NULL;
	// !Depreated

	device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_info.pNext = NULL;
	device_info.queueCreateInfoCount = 1;
	device_info.pQueueCreateInfos = &queue_info;
	device_info.ppEnabledExtensionNames = instance_extension_names;
	device_info.enabledExtensionCount = fck_arraysize(instance_extension_names);
	device_info.pEnabledFeatures = NULL;

	fck_vk_crash(driver->CreateDevice(driver->gpu->device, &device_info, NULL, &driver->device));
	return VK_SUCCESS;
}

VkResult fck_vk_command_init(fck_vk_command *command, fck_vk_driver *driver, fck_vk_queues *queues)
{
	command->driver = driver;

	fck_vk_load_function(command, CreateCommandPool);
	fck_vk_load_function(command, DestroyCommandPool);
	fck_vk_load_function(command, AllocateCommandBuffers);
	fck_vk_load_function(command, FreeCommandBuffers);
	fck_vk_load_function(command, BeginCommandBuffer);
	fck_vk_load_function(command, EndCommandBuffer);
	fck_vk_load_function(command, ResetCommandBuffer);
	fck_vk_load_function(command, CmdBindPipeline);
	fck_vk_load_function(command, CmdSetViewport);
	fck_vk_load_function(command, CmdSetScissor);
	fck_vk_load_function(command, CmdSetLineWidth);
	fck_vk_load_function(command, CmdSetDepthBias);
	fck_vk_load_function(command, CmdSetBlendConstants);
	fck_vk_load_function(command, CmdSetDepthBounds);
	fck_vk_load_function(command, CmdSetStencilCompareMask);
	fck_vk_load_function(command, CmdSetStencilWriteMask);
	fck_vk_load_function(command, CmdSetStencilReference);
	fck_vk_load_function(command, CmdBindDescriptorSets);
	fck_vk_load_function(command, CmdBindIndexBuffer);
	fck_vk_load_function(command, CmdBindVertexBuffers);
	fck_vk_load_function(command, CmdDraw);
	fck_vk_load_function(command, CmdDrawIndexed);
	fck_vk_load_function(command, CmdDrawIndirect);
	fck_vk_load_function(command, CmdDrawIndexedIndirect);
	fck_vk_load_function(command, CmdDispatch);
	fck_vk_load_function(command, CmdDispatchIndirect);
	fck_vk_load_function(command, CmdCopyBuffer);
	fck_vk_load_function(command, CmdCopyImage);
	fck_vk_load_function(command, CmdBlitImage);
	fck_vk_load_function(command, CmdCopyBufferToImage);
	fck_vk_load_function(command, CmdCopyImageToBuffer);
	fck_vk_load_function(command, CmdUpdateBuffer);
	fck_vk_load_function(command, CmdFillBuffer);
	fck_vk_load_function(command, CmdClearColorImage);
	fck_vk_load_function(command, CmdClearDepthStencilImage);
	fck_vk_load_function(command, CmdClearAttachments);
	fck_vk_load_function(command, CmdResolveImage);
	fck_vk_load_function(command, CmdSetEvent);
	fck_vk_load_function(command, CmdResetEvent);
	fck_vk_load_function(command, CmdWaitEvents);
	fck_vk_load_function(command, CmdPipelineBarrier);
	fck_vk_load_function(command, CmdBeginQuery);
	fck_vk_load_function(command, CmdEndQuery);
	fck_vk_load_function(command, CmdResetQueryPool);
	fck_vk_load_function(command, CmdWriteTimestamp);
	fck_vk_load_function(command, CmdCopyQueryPoolResults);
	fck_vk_load_function(command, CmdPushConstants);
	fck_vk_load_function(command, CmdBeginRenderPass);
	fck_vk_load_function(command, CmdNextSubpass);
	fck_vk_load_function(command, CmdEndRenderPass);
	fck_vk_load_function(command, CmdExecuteCommands);

	VkCommandPoolCreateInfo cmd_pool_info = {};
	cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmd_pool_info.pNext = NULL;
	cmd_pool_info.queueFamilyIndex = fck_vk_queues_get_family(queues, FCK_VK_QUEUE_GRAPHIC);
	cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	fck_vk_crash(command->CreateCommandPool(driver->device, &cmd_pool_info, NULL, &command->pool));

	VkCommandBufferAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.pNext = NULL;
	alloc_info.commandPool = command->pool;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandBufferCount = FCK_VK_IMAGE_COUNT;

	return fck_vk_error(command->AllocateCommandBuffers(driver->device, &alloc_info, command->buffer));
}

VkResult fck_vk_swapchain_resize(fck_vk_swapchain *swapchain, VkExtent2D extent)
{
	// 4 should be plenty, we do not need to hug the images, only the views, but we will see...
	VkImage swapchain_images[FCK_VK_IMAGE_COUNT];
	if (swapchain->swapchain != VK_NULL_HANDLE)
	{
		fckc_u32 image_count = fck_arraysize(swapchain_images);
		VkResult result =
			fck_vk_error(swapchain->GetSwapchainImagesKHR(swapchain->driver->device, swapchain->swapchain, &image_count, swapchain_images));
		for (uint32_t i = 0; i < image_count; i++)
		{
			swapchain->driver->DestroyImageView(swapchain->driver->device, swapchain->views[i], NULL);
		}
	}

	// Resize
	swapchain->create_info.imageExtent = extent;
	swapchain->create_info.oldSwapchain = swapchain->swapchain;

	VkResult result =
		fck_vk_error(swapchain->CreateSwapchainKHR(swapchain->driver->device, &swapchain->create_info, NULL, &swapchain->swapchain));
	if (result != VK_SUCCESS)
	{
		return result;
	}
	// swapchain_image_view_count = fck_arraysize(swapchain_images);
	// In any case, we can query them, in other cases we have views.
	fckc_u32 image_count = fck_arraysize(swapchain_images);
	result =
		fck_vk_error(swapchain->GetSwapchainImagesKHR(swapchain->driver->device, swapchain->swapchain, &image_count, swapchain_images));
	if (result != VK_SUCCESS)
	{
		return result;
	}
	swapchain->count = image_count;

	for (uint32_t i = 0; i < image_count; i++)
	{
		VkImageViewCreateInfo color_image_view = {};
		color_image_view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		color_image_view.pNext = NULL;
		color_image_view.flags = 0;
		color_image_view.image = swapchain_images[i];
		color_image_view.viewType = VK_IMAGE_VIEW_TYPE_2D;
		color_image_view.format = swapchain->format;
		color_image_view.components.r = VK_COMPONENT_SWIZZLE_R;
		color_image_view.components.g = VK_COMPONENT_SWIZZLE_G;
		color_image_view.components.b = VK_COMPONENT_SWIZZLE_B;
		color_image_view.components.a = VK_COMPONENT_SWIZZLE_A;
		color_image_view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		color_image_view.subresourceRange.baseMipLevel = 0;
		color_image_view.subresourceRange.levelCount = 1;
		color_image_view.subresourceRange.baseArrayLayer = 0;
		color_image_view.subresourceRange.layerCount = 1;

		result = fck_vk_error(swapchain->driver->CreateImageView(swapchain->driver->device, &color_image_view, NULL, &swapchain->views[i]));
		if (result != VK_SUCCESS)
		{
			return result;
		}
	}

	return VK_SUCCESS;
}

VkResult fck_vk_swapchain_init(fck_vk_swapchain *swapchain, fck_vk_driver *driver, fck_vk_queues *queues, VkSurfaceKHR surface,
                               VkExtent2D extent)
{
	fck_vk_gpu *gpu = driver->gpu;
	swapchain->driver = driver;

	fck_vk_load_function(swapchain, CreateSwapchainKHR);
	fck_vk_load_function(swapchain, GetSwapchainImagesKHR);
	fck_vk_load_function(swapchain, AcquireNextImageKHR);

	// Get the list of VkFormats that are supported:
	uint32_t format_count;
	fck_vk_crash(gpu->GetPhysicalDeviceSurfaceFormatsKHR(gpu->device, surface, &format_count, NULL));

	VkSurfaceFormatKHR *formats = (VkSurfaceFormatKHR *)kll_malloc(kll_heap, format_count * sizeof(VkSurfaceFormatKHR));
	fck_vk_crash(gpu->GetPhysicalDeviceSurfaceFormatsKHR(gpu->device, surface, &format_count, formats));
	fck_assert(format_count >= 1); // That would be fucking weird, lol

	// If the format list includes just one entry of VK_FORMAT_UNDEFINED,
	// the surface has no preferred format.  Otherwise, at least one
	// supported format will be returned.
	if (format_count == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
	{
		// Does that actually happen?
		swapchain->format = VK_FORMAT_B8G8R8A8_UNORM;
	}
	else
	{
		swapchain->format = formats[0].format;
	}
	kll_free(kll_heap, formats);

	VkSurfaceCapabilitiesKHR surface_capabilities;
	fck_vk_crash(gpu->GetPhysicalDeviceSurfaceCapabilitiesKHR(gpu->device, surface, &surface_capabilities));

	VkPresentModeKHR present_modes[16];
	uint32_t present_modes_capacity = fck_arraysize(present_modes);
	uint32_t present_modes_count;
	fck_vk_crash(gpu->GetPhysicalDeviceSurfacePresentModesKHR(gpu->device, surface, &present_modes_count, NULL));
	fck_assert(present_modes_count <= present_modes_capacity);
	fck_assert(present_modes_count >= 1);

	fck_vk_crash(gpu->GetPhysicalDeviceSurfacePresentModesKHR( //
		gpu->device,                                           //
		surface,                                               //
		&present_modes_count,                                  //
		present_modes)                                         //
	);
	fck_assert(present_modes_count >= 1);

	// The FIFO present mode is guaranteed by the spec to be supported
	VkPresentModeKHR swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR; // we can also use queried present modes

	// Determine the number of VkImage's to use in the swap chain.
	// We need to acquire only 1 presentable image at at time.
	// Asking for minImageCount images ensures that we can acquire
	// 1 presentable image as long as we present it before attempting
	// to acquire another.
	uint32_t desiredNumberOfSwapChainImages = surface_capabilities.minImageCount;

	VkSurfaceTransformFlagBitsKHR pre_transform;
	if (surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
	{
		pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	}
	else
	{
		pre_transform = surface_capabilities.currentTransform;
	}

	// Find a supported composite alpha mode - one of these is guaranteed to be set
	VkCompositeAlphaFlagBitsKHR alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	VkCompositeAlphaFlagBitsKHR alpha_flags[4] = {
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
	};
	for (uint32_t i = 0; i < sizeof(alpha_flags) / sizeof(alpha_flags[0]); i++)
	{
		if (surface_capabilities.supportedCompositeAlpha & alpha_flags[i])
		{
			alpha = alpha_flags[i];
			break;
		}
	}

	VkSwapchainCreateInfoKHR swapchain_create_info = {};
	swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_create_info.pNext = NULL;
	swapchain_create_info.surface = surface;
	swapchain_create_info.minImageCount = desiredNumberOfSwapChainImages;
	swapchain_create_info.imageFormat = swapchain->format;
	swapchain_create_info.imageExtent = extent;
	swapchain_create_info.preTransform = pre_transform;
	swapchain_create_info.compositeAlpha = alpha;
	swapchain_create_info.imageArrayLayers = 1;
	swapchain_create_info.presentMode = swapchain_present_mode;
	swapchain_create_info.oldSwapchain = VK_NULL_HANDLE;
	swapchain_create_info.clipped = true;
	swapchain_create_info.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchain_create_info.queueFamilyIndexCount = 0;
	swapchain_create_info.pQueueFamilyIndices = NULL;
	swapchain_create_info.oldSwapchain = swapchain->swapchain;
	swapchain->swapchain = VK_NULL_HANDLE;

	uint32_t queue_family_indices[2] = {
		(uint32_t)fck_vk_queues_get_family(queues, FCK_VK_QUEUE_GRAPHIC),
		(uint32_t)fck_vk_queues_get_family(queues, FCK_VK_QUEUE_PRESENT),
	};
	if (queue_family_indices[0] != queue_family_indices[1])
	{
		// If the graphics and present queues are from different queue families,
		// we either have to explicitly transfer ownership of images between
		// the queues, or we have to create the swapchain with imageSharingMode
		// as VK_SHARING_MODE_CONCURRENT
		swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchain_create_info.queueFamilyIndexCount = 2;
		swapchain_create_info.pQueueFamilyIndices = queue_family_indices;
	}
	swapchain->create_info = swapchain_create_info;

	return fck_vk_swapchain_resize(swapchain, extent);
}

typedef enum fck_memory_access_operation
{
	FCK_LOAD = 0,
	FCK_STORE = 0,
	FCK_DONT_CARE = 1, // Maybe, discard?
	FCK_CLEAR = 2,
} fck_memory_access_operation;

typedef enum fck_layout_type
{
	FCK_LAYOUT_UNDEFINED = 0,
	FCK_LAYOUT_GENERAL = 1,
	FCK_LAYOUT_COLOR_ATTACHMENT = 2,
	FCK_LAYOUT_DEPTH_STENCIL_ATTACHMENT = 3,
	FCK_LAYOUT_DEPTH_STENCIL_READ_ONLY = 4,
	FCK_LAYOUT_SHADER_READ_ONLY = 5,
	// FCK_LAYOUT_TRANSFER_SOURCE = 6,
	// FCK_LAYOUT_TRANSFER_TARGET = 7,
	// FCK_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT = 1000117000,
	// FCK_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY = 1000117001,
	// FCK_LAYOUT_DEPTH_ATTACHMENT = 1000241000,
	// FCK_LAYOUT_DEPTH_READ_ONLY = 1000241001,
	// FCK_LAYOUT_STENCIL_ATTACHMENT = 1000241002,
	// FCK_LAYOUT_STENCIL_READ_ONLY = 1000241003,
	// FCK_LAYOUT_READ_ONLY = 1000314000,
	// FCK_LAYOUT_ATTACHMENT = 1000314001,
	// FCK_LAYOUT_RENDERING_LOCAL_READ = 1000232000,
	FCK_LAYOUT_PRESENT = 1000001002,
} fck_layout_type;

typedef enum fck_stage_flags
{
	FCK_STAGE_TOP_OF_PIPE = 0x00000001,
	FCK_STAGE_DRAW_INDIRECT = 0x00000002,
	FCK_STAGE_VERTEX_INPUT = 0x00000004,
	FCK_STAGE_VERTEX_SHADER = 0x00000008,
	FCK_STAGE_TESSELLATION_CONTROL_SHADER = 0x00000010,
	FCK_STAGE_TESSELLATION_EVALUATION_SHADER = 0x00000020,
	FCK_STAGE_GEOMETRY_SHADER = 0x00000040,
	FCK_STAGE_FRAGMENT_SHADER = 0x00000080,
	FCK_STAGE_EARLY_FRAGMENT_TESTS = 0x00000100,
	FCK_STAGE_LATE_FRAGMENT_TESTS = 0x00000200,
	FCK_STAGE_COLOR_ATTACHMENT_OUTPUT = 0x00000400,
	FCK_STAGE_COMPUTE_SHADER = 0x00000800,
	FCK_STAGE_TRANSFER = 0x00001000,
	FCK_STAGE_BOTTOM_OF_PIPE = 0x00002000,
	FCK_STAGE_HOST = 0x00004000,
	FCK_STAGE_ALL_GRAPHICS = 0x00008000,
	FCK_STAGE_ALL_COMMANDS = 0x00010000,
} fck_stage_flags;

typedef enum fck_access_flags
{
	FCK_ACCESS_INDIRECT_COMMAND_READ = 0x00000001,
	FCK_ACCESS_INDEX_READ = 0x00000002,
	FCK_ACCESS_VERTEX_ATTRIBUTE_READ = 0x00000004,
	FCK_ACCESS_UNIFORM_READ = 0x00000008,
	FCK_ACCESS_INPUT_ATTACHMENT_READ = 0x00000010,
	FCK_ACCESS_SHADER_READ = 0x00000020,
	FCK_ACCESS_SHADER_WRITE = 0x00000040,
	FCK_ACCESS_COLOR_ATTACHMENT_READ = 0x00000080,
	FCK_ACCESS_COLOR_ATTACHMENT_WRITE = 0x00000100,
	FCK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ = 0x00000200,
	FCK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE = 0x00000400,
	FCK_ACCESS_TRANSFER_READ = 0x00000800,
	FCK_ACCESS_TRANSFER_WRITE = 0x00001000,
	FCK_ACCESS_HOST_READ = 0x00002000,
	FCK_ACCESS_HOST_WRITE = 0x00004000,
	FCK_ACCESS_MEMORY_READ = 0x00008000,
	FCK_ACCESS_MEMORY_WRITE = 0x00010000,
} fck_access_flags;

typedef enum fck_format
{
	FCK_FORMAT_UNDEFINED = 0,
	FCK_FORMAT_R8G8B8A8_UNORM = 1,
	FCK_FORMAT_R8G8B8A8_SRGB = 2,
	FCK_FORMAT_B8G8R8A8_UNORM = 3,
	FCK_FORMAT_B8G8R8A8_SRGB = 4,
	FCK_FORMAT_D16_UNORM = 5,

	R32G32B32_SFLOAT = 420,
} fck_format;

typedef struct fck_vk_render_pass
{
	VkRenderPass handle;
} fck_vk_render_pass;

typedef struct fck_attachment
{
	fck_format format;
} fck_attachment;

typedef enum fck_topology
{
	FCK_TOPOLOGY_POINT_LIST = 0,
	FCK_TOPOLOGY_LINE_LIST = 1,
	FCK_TOPOLOGY_LINE_STRIP = 2,
	FCK_TOPOLOGY_TRIANGLE_LIST = 3,
	FCK_TOPOLOGY_TRIANGLE_STRIP = 4,
	FCK_TOPOLOGY_TRIANGLE_FAN = 5,
} fck_topology;

typedef enum fck_cull
{
	FCK_CULL_NONE = 0x00000000,
	FCK_CULL_FRONT = 0x00000001,
	FCK_CULL_BACK = 0x00000002,
} fck_cull;

typedef enum fck_sample_flags
{
	FCK_SAMPLE_1 = 0x00000001,
	// FCK_SAMPLE_2 = 0x00000002,
	// FCK_SAMPLE_4 = 0x00000004,
	// FCK_SAMPLE_8 = 0x00000008,
	// FCK_SAMPLE_16 = 0x00000010,
	// FCK_SAMPLE_32 = 0x00000020,
	// FCK_SAMPLE_64 = 0x00000040,
} fck_sample_flags;

typedef struct fck_rasterisation_desc
{
	// fck_topology topology;
	// fck_cull cull;
	//  fck_sample_flags samples;
	fck_format depth;
	// fck_format stencil_format;
	fck_format colour;
} fck_rasterisation_desc;

// Render pass
// -> Frame Buffer
// 	-> Graphics Pipeline
// The Graphics Pipeline requires a reference TO the frame buffer and the render pass
/*
fck_vk_framebuffer fck_vk_frame_buffer_create2(fck_vk_swapchain *swapchain, fck_vk_image *colours, fckc_size_t colour_count,
                                               fck_vk_image depth)
{
    VkImageView attachments[2];
    attachments[1] = depth.view;

    VkFramebufferCreateInfo fb_info = {};
    fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fb_info.pNext = NULL;
    fb_info.renderPass = render_pass;
    fb_info.attachmentCount = 2;
    fb_info.pAttachments = attachments;
    fb_info.width = extent.width;
    fb_info.height = extent.height;
    fb_info.layers = 1;

    for (uint32_t index = 0; index < colour_count; index++)
    {
        attachments[0] = colours[index].view;
        fck_vk_crash(swapchain->CreateFramebuffer(swapchain->driver->device, &fb_info, NULL, &swapchain->framebuffers[index]));
    }
}

VkResult fck_vk_render_pass_create2(fck_vk_driver *driver, fck_rasterisation_desc *rasterisation, VkRenderPass *render_pass)
{
    VkAttachmentDescription attachments[2];
    VkAttachmentReference references[2];
    VkSubpassDependency dependencies[2];

    fckc_size_t dependency_count = 0;
    fckc_size_t attachments_count = 0;
    fckc_size_t colour_count = 0;
    if (rasterisation->colour != FCK_FORMAT_UNDEFINED)
    {
        VkAttachmentReference *reference = &references[attachments_count];
        reference->attachment = attachments_count;
        reference->layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription *attachment = &attachments[attachments_count++];
        attachment->format = (VkFormat)rasterisation->colour; // TODO: Make clean translation
        attachment->samples = VK_SAMPLE_COUNT_1_BIT;          // TODO: Make clean translation
        attachment->loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment->initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment->finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkSubpassDependency *dependency = &dependencies[dependency_count++];
        dependency->srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency->dstSubpass = 0;
        dependency->srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency->dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency->srcAccessMask = 0;
        dependency->dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
        dependency->dependencyFlags = 0;

        colour_count = attachments_count;
    }

    if (rasterisation->depth != FCK_FORMAT_UNDEFINED)
    {
        VkAttachmentReference *reference = &references[attachments_count];
        reference->attachment = attachments_count;
        reference->layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription *attachment = &attachments[attachments_count++];
        attachment->format = (VkFormat)rasterisation->depth; // TODO: Make clean translation
        attachment->samples = VK_SAMPLE_COUNT_1_BIT;
        attachment->loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment->storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment->initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment->finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDependency *dependency = &dependencies[dependency_count++];
        dependency->srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency->dstSubpass = 0;
        dependency->srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependency->dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependency->srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependency->dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        dependency->dependencyFlags = 0;
    }

    // Setup a single subpass reference
    VkAttachmentReference *colour_references = &references[0];
    VkAttachmentReference *depth_reference = &references[colour_count];

    VkSubpassDescription subpass_desc = {0};
    subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_desc.colorAttachmentCount = colour_count;
    subpass_desc.pColorAttachments = colour_references;
    subpass_desc.pDepthStencilAttachment = depth_reference;
    subpass_desc.inputAttachmentCount = 0;
    subpass_desc.pInputAttachments = NULL;
    subpass_desc.preserveAttachmentCount = 0;
    subpass_desc.pPreserveAttachments = NULL;
    subpass_desc.pResolveAttachments = NULL;

    // Create the actual renderpass
    VkRenderPassCreateInfo render_pass_create_info = {0};
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.attachmentCount = fck_arraysize(attachments);  // Number of attachments used by this render pass
    render_pass_create_info.pAttachments = attachments;                    // Descriptions of the attachments used by the render pass
    render_pass_create_info.subpassCount = 1;                              // We only use one subpass in this example
    render_pass_create_info.pSubpasses = &subpass_desc;                    // Description of that subpass
    render_pass_create_info.dependencyCount = fck_arraysize(dependencies); // Number of subpass dependencies
    render_pass_create_info.pDependencies = dependencies;                  // Subpass dependencies used by the render pass

    VkResult result = driver->CreateRenderPass(driver->device, &render_pass_create_info, NULL, render_pass);

    if (fck_vk_failure(result))
    {
        return result;
    }

    fck_vk_frame_buffer_create2();
}
*/

VkResult fck_vk_render_pass_create(fck_vk_driver *driver, VkFormat color_format, sht_format depth_format, VkRenderPass *render_pass)
{
	// Descriptors for the attachments used by this renderpass
	VkAttachmentDescription attachments[2] = {0};

	// Color attachment
	attachments[0].format = color_format;                  // Use the color format selected by the swapchain
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;        // We don't use multi sampling in this example
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;   // Clear this attachment at the start of the render pass
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Keep its contents after the render pass is finished (for displaying it)
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;   // We don't use stencil, so don't care for load
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Same for store
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Layout at render pass start. Initial doesn't matter, so we use undefined
	attachments[0].finalLayout =
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // Layout to which the attachment is transitioned when the render pass is finished
	                                     // As we want to present the color buffer to the swapchain, we transition to PRESENT_KHR
	// Depth attachment
	attachments[1].format = sht_vk_format_from_sht_format(depth_format); // A proper depth format is selected in the example base
	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Clear depth at start of first subpass
	attachments[1].storeOp =
		VK_ATTACHMENT_STORE_OP_DONT_CARE; // We don't need depth after render pass has finished (DONT_CARE may result in better performance)
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;   // No stencil
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // No Stencil
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Layout at render pass start. Initial doesn't matter, so we use undefined
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // Transition to depth/stencil attachment

	// Setup attachment references
	VkAttachmentReference color_reference = {0};
	color_reference.attachment = 0;                                    // Attachment 0 is color
	color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Attachment layout used as color during the subpass

	VkAttachmentReference depth_reference = {0};
	depth_reference.attachment = 1;                                            // Attachment 1 is color
	depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // Attachment used as depth/stencil used during the subpass

	// Setup a single subpass reference
	VkSubpassDescription subpass_desc = {0};
	subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass_desc.colorAttachmentCount = 1;                   // Subpass uses one color attachment
	subpass_desc.pColorAttachments = &color_reference;       // Reference to the color attachment in slot 0
	subpass_desc.pDepthStencilAttachment = &depth_reference; // Reference to the depth attachment in slot 1
	subpass_desc.inputAttachmentCount = 0;                   // Input attachments can be used to sample from contents of a previous subpass
	subpass_desc.pInputAttachments = NULL;                   // (Input attachments not used by this example)
	subpass_desc.preserveAttachmentCount = 0; // Preserved attachments can be used to loop (and preserve) attachments through subpasses
	subpass_desc.pPreserveAttachments = NULL; // (Preserve attachments not used by this example)
	subpass_desc.pResolveAttachments =
		NULL; // Resolve attachments are resolved at the end of a sub pass and can be used for e.g. multi sampling

	// Setup subpass dependencies
	// These will add the implicit attachment layout transitions specified by the attachment descriptions
	// The actual usage layout is preserved through the layout specified in the attachment reference
	// Each subpass dependency will introduce a memory and execution dependency between the source and dest subpass described by
	// srcStageMask, dstStageMask, srcAccessMask, dstAccessMask (and dependencyFlags is set)
	// Note: VK_SUBPASS_EXTERNAL is a special constant that refers to all commands executed outside of the actual renderpass)
	VkSubpassDependency dependencies[2] = {0};

	// Does the transition from final to initial layout for the depth an color attachments
	// Depth attachment
	dependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].dstSubpass = 0;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
	dependencies[1].dependencyFlags = 0;
	// Color attachment
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = 0;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
	dependencies[0].dependencyFlags = 0;

	// Create the actual renderpass
	VkRenderPassCreateInfo render_pass_create_info = {0};
	render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_create_info.attachmentCount = fck_arraysize(attachments);  // Number of attachments used by this render pass
	render_pass_create_info.pAttachments = attachments;                    // Descriptions of the attachments used by the render pass
	render_pass_create_info.subpassCount = 1;                              // We only use one subpass in this example
	render_pass_create_info.pSubpasses = &subpass_desc;                    // Description of that subpass
	render_pass_create_info.dependencyCount = fck_arraysize(dependencies); // Number of subpass dependencies
	render_pass_create_info.pDependencies = dependencies;                  // Subpass dependencies used by the render pass

	return driver->CreateRenderPass(driver->device, &render_pass_create_info, NULL, render_pass);
}

void fck_vk_frame_buffer_destroy(fck_vk_driver *driver, fck_vk_framebuffer *framebuffer)
{
	for (uint32_t index = 0; index < framebuffer->count; index++)
	{
		driver->DestroyFramebuffer(driver->device, framebuffer->values[index], NULL);
	}
}

VkResult fck_vk_frame_buffer_create(fck_vk_driver *driver, VkRenderPass render_pass, VkExtent2D extent, sht_image_view *depth,
                                    VkImageView *swapchain_views, fckc_size_t chain_count, fck_vk_framebuffer *framebuffer)
{
	VkImageView attachments[2];
	attachments[1] = (VkImageView)depth->gpu;

	// VkImage swapchain_images[FCK_VK_IMAGE_COUNT];
	//  swapchain_image_view_count = fck_arraysize(swapchain_images);
	//  In any case, we can query them, in other cases we have views.
	VkFramebufferCreateInfo fb_info = {};
	fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fb_info.pNext = NULL;
	fb_info.renderPass = render_pass;
	fb_info.attachmentCount = 2;
	fb_info.pAttachments = attachments;
	fb_info.width = extent.width;
	fb_info.height = extent.height;
	fb_info.layers = 1;

	for (uint32_t index = 0; index < chain_count; index++)
	{
		attachments[0] = swapchain_views[index];
		fck_vk_crash(driver->CreateFramebuffer(driver->device, &fb_info, NULL, &framebuffer->values[index]));
	}
	framebuffer->count = chain_count;
	return VK_SUCCESS;
}

VkResult fck_vk_uniform_buffer_create(sht_memory *mem, void *data, fckc_size_t size, sht_buffer *buffer, fckc_size_t count)
{
	VkBufferCreateInfo buf_info = {};
	buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buf_info.pNext = NULL;
	buf_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	buf_info.size = size;
	buf_info.queueFamilyIndexCount = 0;
	buf_info.pQueueFamilyIndices = NULL;
	buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	buf_info.flags = 0;

	for (fckc_size_t index = 0; index < count; index++)
	{
		sht_buffer *current = buffer + index;
		*current = mem->malloc(mem->bump, &sht_buffer_retained(SHT_BUFFER_USAGE_UNIFORM, size), SHT_MEMORY_CPU);
		memcpy(current->cpu, data, size);
	}
	return VK_SUCCESS;
}

VkSurfaceKHR fck_vk_surface_create(fck_vk_instance *vk, fck_vk_platform *platform, const void *handle)
{
	// This shit in between here has to come from OUTSIDE the render api
	// since we have no control over a window! :)
	// Or maybe it doesn not? We know we are on macos...
	VkMetalSurfaceCreateInfoEXT metal_create_info = (VkMetalSurfaceCreateInfoEXT){
		.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT,
		.pLayer = (const CAMetalLayer *)handle, //
		.flags = 0,
		.pNext = NULL,
	};

	VkSurfaceKHR surface;
	if (fck_vk_error(platform->CreateMetalSurfaceEXT(vk->instance, &metal_create_info, NULL, &surface)))
	{
		return VK_NULL_HANDLE;
	}
	return surface;
}

VkResult fck_vk_platform_init(fck_vk_platform *platform, fck_vk_gpu *gpu, fck_window window, VkSurfaceKHR *out_surface)
{
#if defined(__APPLE__)
	typedef CGRect NSRect;
	typedef CGPoint NSPoint;
	typedef CGSize NSSize;
	typedef void NSString;
	typedef void MTKView;
	typedef void MTLDevice;

	extern MTLDevice *MTLCreateSystemDefaultDevice();

#define objc_msgSend_address ((void *(*)(id, SEL))objc_msgSend)
#define objc_msgSend_string ((NSString * (*)(id, SEL)) objc_msgSend)
#define objc_msgSend_id ((id (*)(id, SEL))objc_msgSend)
#define objc_msgSend_void_id ((void (*)(id, SEL, id))objc_msgSend)

#define NSAlloc(nsclass) objc_msgSend_id((id)(nsclass), sel_registerName("alloc"))
#define NSRelease(nsclass) objc_msgSend_id((id)(nsclass), sel_registerName("release"))

#define name(target_type) sizeof(target_type) ? #target_type : NULL

	fck_vk_load_function(platform, CreateMetalSurfaceEXT);

	int width;
	int height;
	os->win->size(window, &width, &height);

	// TODO: FREE ALL THE DATA! RLEASE IT ANYTHING?!!
	typedef struct fck_metal_context
	{
		MTLDevice *device;
		MTKView *view;
		CAMetalLayer *layer;
	} fck_metal_context;

	fck_metal_context mtl;
	SEL initWithFrame = sel_registerName("initWithFrame:device:");
	mtl.view = NSAlloc(objc_getClass(name(MTKView)));

	mtl.device = MTLCreateSystemDefaultDevice();
	typedef MTKView *(*mtk_view_init_with_frame)(MTKView *, SEL, NSRect, MTLDevice *);
	NSRect rect = (NSRect){.origin = (NSPoint){0, 0}, .size = (NSSize){width, height}};
	mtl.view = ((mtk_view_init_with_frame)objc_msgSend)(mtl.view, initWithFrame, rect, mtl.device);
	mtl.layer = (CAMetalLayer *)objc_msgSend_id(mtl.view, sel_registerName("layer"));

	NSRect r = ((NSRect (*)(id, SEL))objc_msgSend)(mtl.layer, sel_registerName("frame"));

	objc_msgSend_void_id(window.handle, sel_registerName("setContentView:"), mtl.view);
	if (!os->win->is_valid(window))
	{
		return VK_NOT_READY;
	}
	NSString *mtl_device_name = objc_msgSend_string(mtl.device, sel_registerName("name"));
	const char *mtl_device_cstring = (const char *)objc_msgSend_address(mtl_device_name, sel_registerName("UTF8String"));
	if (!fck_vk_gpu_select(gpu, mtl_device_cstring))
	{
		os->io->log("Could not find MTLDevice: %s", mtl_device_cstring);
		return VK_NOT_READY;
	}
	os->io->log("Found shared MTL-VK Device: %s", mtl_device_cstring);
	VkSurfaceKHR surface = fck_vk_surface_create(gpu->vk, platform, (const void *)mtl.layer);
	if (surface == VK_NULL_HANDLE)
	{
		return VK_NOT_READY;
	}
	*out_surface = surface;
	os->io->log("Surface creation: %s", "SUCCESS");
#else
	os->io->log("Platform not implemented");
	return VK_NOT_READY;
#endif

	return VK_SUCCESS;
}

void fck_vk_copy(fck_vk_command *command, VkCommandBuffer copy_commands, sht_buffer *src, sht_buffer *dst)
{
	// Put buffer region copies into command buffer
	VkBufferCopy copy_region;
	copy_region.srcOffset = 0;
	copy_region.dstOffset = 0;

	copy_region.size = fck_min(src->size, dst->size);
	command->CmdCopyBuffer(copy_commands, src->gpu, dst->gpu, 1, &copy_region);
}

void fck_vk_submit_oneshot(fck_vk_driver *driver, fck_vk_queues *queues, fckc_u32 count, VkCommandBuffer *commands)
{
	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = count;
	submit_info.pCommandBuffers = commands;

	VkDevice device = driver->device;
	// Create fence to ensure that the command buffer has finished executing
	VkFenceCreateInfo fence_create_info = {0};
	fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_create_info.flags = 0;
	VkFence fence;
	fck_vk_crash(driver->CreateFence(device, &fence_create_info, NULL, &fence));

	VkQueue queue = fck_vk_queues_get_primary(queues, device, FCK_VK_QUEUE_TRANSFER);

	// Submit to the queue
	fck_vk_crash(queues->QueueSubmit(queue, 1, &submit_info, fence));
	fck_vk_crash(driver->WaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX));

	driver->DestroyFence(device, fence, NULL);
}

VkResult fck_create_vertex_buffer(sht_memory *mem, fck_vk_command *command, fck_vk_queues *queues, sht_elements *vertices,
                                  sht_elements *indices)
{
	// A note on memory management in Vulkan in general:
	//	This is a very complex topic and while it's fine for an example application to small individual memory allocations that is not
	//	what should be done a real-world application, where you should allocate large chunks of memory at once instead.
	fck_vk_driver *driver = command->driver;
	VkDevice device = driver->device;

	// Setup vertices
	fck_vertex vertex_buffer[] = {
		{{1.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}}, {{-1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}}, {{0.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}};
	uint32_t vertex_buffer_size = sizeof(vertex_buffer);

	// Setup indices
	fckc_u32 index_buffer[] = {0, 1, 2};
	uint32_t index_buffer_size = sizeof(index_buffer);

	// Vertex buffer
	sht_buffer vertex_staging_buffer = mem->malloc(mem->temp, &sht_buffer_source(0, vertex_buffer_size), SHT_MEMORY_CPU);
	memcpy(vertex_staging_buffer.cpu, vertex_buffer, vertex_buffer_size);
	vertices->buffer = mem->malloc(mem->bump, &sht_buffer_target(SHT_BUFFER_USAGE_VERTEX, vertex_buffer_size), SHT_MEMORY_GPU);

	// Index buffer
	sht_buffer index_staging_buffer = mem->malloc(mem->temp, &sht_buffer_source(0, index_buffer_size), SHT_MEMORY_CPU);
	memcpy(index_staging_buffer.cpu, index_buffer, index_buffer_size);
	indices->buffer = mem->malloc(mem->bump, &sht_buffer_target(SHT_BUFFER_USAGE_INDEX, index_buffer_size), SHT_MEMORY_GPU);
	VkCommandBuffer copy_commands;

	VkCommandBufferAllocateInfo command_buffer_alloc_info = {};
	command_buffer_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	command_buffer_alloc_info.commandPool = command->pool;
	command_buffer_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	command_buffer_alloc_info.commandBufferCount = 1;
	fck_vk_crash(command->AllocateCommandBuffers(device, &command_buffer_alloc_info, &copy_commands));

	VkCommandBufferBeginInfo command_buffer_begin_info = {};
	command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	fck_vk_crash(command->BeginCommandBuffer(copy_commands, &command_buffer_begin_info));
	fck_vk_copy(command, copy_commands, &vertex_staging_buffer, &vertices->buffer);
	fck_vk_copy(command, copy_commands, &index_staging_buffer, &indices->buffer);
	fck_vk_crash(command->EndCommandBuffer(copy_commands));

	// Create fence to ensure that the command buffer has finished executing
	fck_vk_submit_oneshot(driver, queues, 1, &copy_commands);
	command->FreeCommandBuffers(device, command->pool, 1, &copy_commands);

	indices->count = fck_arraysize(index_buffer);
	vertices->count = fck_arraysize(vertex_buffer);

	mem->free(mem->temp, &vertex_staging_buffer);
	mem->free(mem->temp, &index_staging_buffer);

	// I think this one never fails... whoops
	return VK_SUCCESS;
}

VkResult fck_vk_descriptor_set_layout_create(fck_vk_driver *driver, VkDescriptorSetLayout *descriptor_set_layout)
{
	// We declare that the vertex shader stage is expecting a uniform buffer
	VkDevice device = driver->device;

	VkDescriptorSetLayoutBinding layout_binding = {};
	layout_binding.binding = 0;
	layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layout_binding.descriptorCount = 1;
	layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	layout_binding.pImmutableSamplers = NULL;

	VkDescriptorSetLayoutCreateInfo descriptor_layout_create_info = {};
	descriptor_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptor_layout_create_info.pNext = NULL;
	descriptor_layout_create_info.bindingCount = 1;
	descriptor_layout_create_info.pBindings = &layout_binding;
	return fck_vk_error(driver->CreateDescriptorSetLayout(device, &descriptor_layout_create_info, NULL, descriptor_set_layout));
}

VkResult fck_vk_descriptor_pool_create(fck_vk_driver *driver, VkDescriptorPool *descriptor_pool)
{
	// We need to tell the API the number of max. requested descriptors per type
	// This example only one descriptor type (uniform buffer)
	// We have one buffer (and as such descriptor) per frame
	VkDescriptorPoolSize descriptor_type_counts[] = {[0] = {
														 .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
														 .descriptorCount = FCK_VK_IMAGE_COUNT,
													 }};
	// For additional types you need to add new entries in the type count list
	// E.g. for two combined image samplers :
	// typeCounts[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	// typeCounts[1].descriptorCount = 2;

	// Create the global descriptor pool
	// All descriptors used in this example are allocated from this pool
	VkDescriptorPoolCreateInfo desciptor_pool_create_info;
	desciptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	desciptor_pool_create_info.pNext = NULL;
	desciptor_pool_create_info.flags = 0;
	// Set the max. number of descriptor sets that can be requested from this pool (requesting beyond this limit will result in an error)
	// Our sample will create one set per uniform buffer per frame
	desciptor_pool_create_info.maxSets = FCK_VK_IMAGE_COUNT;
	desciptor_pool_create_info.poolSizeCount = fck_arraysize(descriptor_type_counts);
	desciptor_pool_create_info.pPoolSizes = descriptor_type_counts;

	return fck_vk_error(driver->CreateDescriptorPool(driver->device, &desciptor_pool_create_info, NULL, descriptor_pool));
}

VkResult fck_vk_descriptor_set_create(fck_vk_driver *driver, VkDescriptorPool descriptor_pool, VkDescriptorSetLayout *set_layouts,
                                      fckc_size_t set_layout_count, sht_buffer *buffers, VkDescriptorSet *descriptor_set, fckc_size_t count)
{
	// Allocate one descriptor set per frame from the global descriptor pool
	for (uint32_t i = 0; i < count; i++)
	{
		sht_buffer *buffer = &buffers[i];
		VkDescriptorSetAllocateInfo alloc_info;
		alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		alloc_info.descriptorPool = descriptor_pool;
		alloc_info.pSetLayouts = set_layouts;
		alloc_info.descriptorSetCount = set_layout_count;
		alloc_info.pNext = NULL;
		VkResult result = fck_vk_error(driver->AllocateDescriptorSets(driver->device, &alloc_info, &descriptor_set[i]));
		if (result != VK_SUCCESS)
		{
			return result;
		}

		// Update the descriptor set determining the shader binding points
		// For every binding point used in a shader there needs to be one
		// descriptor set matching that binding point
		VkWriteDescriptorSet write_descriptor_set;

		// The buffer's information is passed using a descriptor info structure
		VkDescriptorBufferInfo buffer_info;
		buffer_info.buffer = buffer->gpu;
		buffer_info.range = sizeof(fck_mvp);

		write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write_descriptor_set.pNext = NULL;
		write_descriptor_set.dstSet = descriptor_set[i];
		write_descriptor_set.dstBinding = 0;
		write_descriptor_set.dstArrayElement = 0;
		write_descriptor_set.descriptorCount = 1;
		write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		write_descriptor_set.pImageInfo = NULL;
		write_descriptor_set.pBufferInfo = &buffer_info;
		write_descriptor_set.pTexelBufferView = NULL;
		driver->UpdateDescriptorSets(driver->device, 1, &write_descriptor_set, 0, NULL);
	}
	return VK_SUCCESS;
}

VkResult fck_vk_graphics_pipeline_create(fck_vk_driver *driver, VkRenderPass render_pass, VkDescriptorSetLayout *set_layouts,
                                         fckc_size_t count, VkPipelineShaderStageCreateInfo *shader_create_infos, fckc_size_t shader_count,
                                         fck_vk_graphics_pipeline *graphic_pipeline)
{
	// Create the pipeline layout that is used to generate the rendering pipelines that are based on this descriptor set layout
	// In a more complex scenario you would have different pipeline layouts for different descriptor set layouts that could be reused
	VkPipelineLayoutCreateInfo pipeline_layout_create_info;
	pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_create_info.pSetLayouts = set_layouts;
	pipeline_layout_create_info.setLayoutCount = count;
	pipeline_layout_create_info.pNext = NULL;
	pipeline_layout_create_info.flags = 0;
	pipeline_layout_create_info.pushConstantRangeCount = 0;
	pipeline_layout_create_info.pPushConstantRanges = NULL;

	VkPipelineLayout pipeline_layout;
	VkResult result = fck_vk_error(driver->CreatePipelineLayout(driver->device, &pipeline_layout_create_info, NULL, &pipeline_layout));
	if (result != VK_SUCCESS)
	{
		return result;
	}
	// Create the graphics pipeline used in this example
	// Vulkan uses the concept of rendering pipelines to encapsulate fixed states, replacing OpenGL's complex state machine
	// A pipeline is then stored and hashed on the GPU making pipeline changes very fast
	// Note: There are still a few dynamic states that are not directly part of the pipeline (but the info that they are used is)

	// Construct the different states making up the pipeline

	// Input assembly state describes how primitives are assembled
	// This pipeline will assemble vertex data as a triangle lists (though we only use one triangle)
	VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info;
	input_assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly_create_info.pNext = NULL;
	input_assembly_create_info.flags = 0;
	input_assembly_create_info.primitiveRestartEnable = VK_FALSE;

	// Rasterization state
	VkPipelineRasterizationStateCreateInfo rasterisation_state_create_info;
	rasterisation_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterisation_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
	rasterisation_state_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterisation_state_create_info.cullMode = VK_CULL_MODE_NONE;
	rasterisation_state_create_info.lineWidth = 1.0f;
	rasterisation_state_create_info.depthBiasEnable = VK_FALSE;
	rasterisation_state_create_info.depthBiasSlopeFactor = 0.0f;
	rasterisation_state_create_info.depthBiasConstantFactor = 0.0f;
	rasterisation_state_create_info.depthBiasClamp = 0.0f;
	rasterisation_state_create_info.pNext = NULL;
	rasterisation_state_create_info.flags = 0;
	rasterisation_state_create_info.depthClampEnable = VK_FALSE;
	rasterisation_state_create_info.rasterizerDiscardEnable = VK_FALSE;

	// Color blend state describes how blend factors are calculated (if used)
	// We need one blend attachment state per color attachment (even if blending is not used)
	VkPipelineColorBlendAttachmentState blend_attachment_state;
	blend_attachment_state.colorWriteMask = 0xf;
	blend_attachment_state.blendEnable = VK_FALSE;
	blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
	blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo color_blend_state_create_info;
	color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blend_state_create_info.pNext = NULL;
	color_blend_state_create_info.attachmentCount = 1;
	color_blend_state_create_info.pAttachments = &blend_attachment_state;
	color_blend_state_create_info.flags = 0;
	color_blend_state_create_info.logicOpEnable = VK_FALSE;
	color_blend_state_create_info.logicOp = VK_LOGIC_OP_CLEAR;
	// Yes, this is ugly lol
	memset(color_blend_state_create_info.blendConstants, 0, sizeof(color_blend_state_create_info.blendConstants));

	// Viewport state sets the number of viewports and scissor used in this pipeline
	// Note: This is actually overridden by the dynamic states (see below)
	VkPipelineViewportStateCreateInfo viewport_state_create_info;
	viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state_create_info.pNext = NULL;
	viewport_state_create_info.viewportCount = 1;
	viewport_state_create_info.scissorCount = 1;
	viewport_state_create_info.flags = 0;
	viewport_state_create_info.pViewports = NULL;
	viewport_state_create_info.pScissors = NULL;

	// Enable dynamic states
	// Most states are baked into the pipeline, but there are still a few dynamic states that can be changed within a command buffer
	// To be able to change these we need do specify which dynamic states will be changed using this pipeline. Their actual states are set
	// later on in the command buffer. For this example we will set the viewport and scissor using dynamic states
	VkDynamicState dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	VkPipelineDynamicStateCreateInfo dynamic_state_create_info;
	dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state_create_info.pNext = NULL;
	dynamic_state_create_info.flags = 0;
	dynamic_state_create_info.pDynamicStates = dynamic_states;
	dynamic_state_create_info.dynamicStateCount = fck_arraysize(dynamic_states);

	// Depth and stencil state containing depth and stencil compare and test operations
	// We only use depth tests and want depth tests and writes to be enabled and compare with less or equal
	VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info;
	depth_stencil_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil_create_info.pNext = NULL;
	depth_stencil_create_info.flags = 0;
	depth_stencil_create_info.minDepthBounds = 0.0f;
	depth_stencil_create_info.maxDepthBounds = 0.0f;
	depth_stencil_create_info.depthTestEnable = VK_TRUE;
	depth_stencil_create_info.depthWriteEnable = VK_TRUE;
	depth_stencil_create_info.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depth_stencil_create_info.depthBoundsTestEnable = VK_FALSE;
	depth_stencil_create_info.back.failOp = VK_STENCIL_OP_KEEP;
	depth_stencil_create_info.back.passOp = VK_STENCIL_OP_KEEP;
	depth_stencil_create_info.back.compareOp = VK_COMPARE_OP_ALWAYS;
	depth_stencil_create_info.back.depthFailOp = VK_STENCIL_OP_ZERO;
	depth_stencil_create_info.back.compareMask = 0;
	depth_stencil_create_info.back.writeMask = 0;
	depth_stencil_create_info.back.reference = 0;
	depth_stencil_create_info.stencilTestEnable = VK_FALSE;
	depth_stencil_create_info.front = depth_stencil_create_info.back;

	// Multi sampling state
	// This example does not make use of multi sampling (for anti-aliasing), the state must still be set and passed to the pipeline
	VkPipelineMultisampleStateCreateInfo multisample_state_create_info;
	multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample_state_create_info.pNext = NULL;
	multisample_state_create_info.flags = 0;
	multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisample_state_create_info.pSampleMask = NULL;
	multisample_state_create_info.sampleShadingEnable = VK_FALSE;
	multisample_state_create_info.minSampleShading = 0.0f;
	multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
	multisample_state_create_info.alphaToOneEnable = VK_FALSE;

	// Vertex input descriptions
	// Specifies the vertex input parameters for a pipeline

	// Vertex input binding
	// This example uses a single vertex input binding at binding point 0 (see vkCmdBindVertexBuffers)
	VkVertexInputBindingDescription vertex_input_binding;
	vertex_input_binding.binding = 0;
	vertex_input_binding.stride = sizeof(fck_vertex);
	vertex_input_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	// Input attribute bindings describe shader attribute locations and memory layouts
	VkVertexInputAttributeDescription vertex_input_attributes[2];
	// These match the following shader layout (see triangle.vert):
	//	layout (location = 0) in vec3 inPos;
	//	layout (location = 1) in vec3 inColor;
	// Attribute location 0: Position
	vertex_input_attributes[0].binding = 0;
	vertex_input_attributes[0].location = 0;
	// Position attribute is three 32 bit signed (SFLOAT) floats (R32 G32 B32)
	vertex_input_attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertex_input_attributes[0].offset = offsetof(fck_vertex, position);
	// Attribute location 1: Color
	vertex_input_attributes[1].binding = 0;
	vertex_input_attributes[1].location = 1;
	// Color attribute is three 32 bit signed (SFLOAT) floats (R32 G32 B32)
	vertex_input_attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertex_input_attributes[1].offset = offsetof(fck_vertex, color);

	// Vertex input state used for pipeline creation
	VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info;
	vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_state_create_info.pNext = NULL;
	vertex_input_state_create_info.flags = 0;
	vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
	vertex_input_state_create_info.pVertexBindingDescriptions = &vertex_input_binding;
	vertex_input_state_create_info.vertexAttributeDescriptionCount = fck_arraysize(vertex_input_attributes);
	vertex_input_state_create_info.pVertexAttributeDescriptions = vertex_input_attributes;

	VkGraphicsPipelineCreateInfo pipeline_create_info;
	pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_create_info.pNext = NULL;
	pipeline_create_info.flags = 0;
	// The layout used for this pipeline (can be shared among multiple pipelines using the same layout)
	pipeline_create_info.layout = pipeline_layout;
	// Renderpass this pipeline is attached to
	pipeline_create_info.renderPass = render_pass;
	pipeline_create_info.subpass = 0;
	pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
	pipeline_create_info.basePipelineIndex = 0;
	// Shaders
	// Set pipeline shader stage info
	pipeline_create_info.stageCount = shader_count;
	pipeline_create_info.pStages = shader_create_infos;

	// Assign the pipeline states to the pipeline creation info structure
	pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
	pipeline_create_info.pInputAssemblyState = &input_assembly_create_info;
	pipeline_create_info.pRasterizationState = &rasterisation_state_create_info;
	pipeline_create_info.pColorBlendState = &color_blend_state_create_info;
	pipeline_create_info.pMultisampleState = &multisample_state_create_info;
	pipeline_create_info.pViewportState = &viewport_state_create_info;
	pipeline_create_info.pDepthStencilState = &depth_stencil_create_info;
	pipeline_create_info.pDynamicState = &dynamic_state_create_info;

	// Create rendering pipeline using the specified states

	VkPipelineCache pipeline_cache;

	VkPipelineCacheCreateInfo pipeline_cache_create_info;
	pipeline_cache_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	pipeline_cache_create_info.pNext = NULL;
	pipeline_cache_create_info.flags = 0;
	pipeline_cache_create_info.initialDataSize = 0;
	pipeline_cache_create_info.pInitialData = NULL;

	fck_vk_crash(driver->CreatePipelineCache(driver->device, &pipeline_cache_create_info, NULL, &pipeline_cache));

	VkPipeline pipeline;
	fck_vk_crash(driver->CreateGraphicsPipelines(driver->device, pipeline_cache, 1, &pipeline_create_info, NULL, &pipeline));

	graphic_pipeline->layout = pipeline_layout;
	graphic_pipeline->cache = pipeline_cache;
	graphic_pipeline->pipeline = pipeline;
	return VK_SUCCESS;
}

VkResult fck_vk_fences_create(fck_vk_driver *driver, VkFence *fences, fckc_size_t count)
{
	VkFenceCreateInfo create_info;
	create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	create_info.pNext = NULL;
	create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	for (fckc_size_t index = 0; index < count; index++)
	{
		VkResult result = fck_vk_error(driver->CreateFence(driver->device, &create_info, NULL, &fences[index]));
		if (result != VK_SUCCESS)
		{
			return result;
		}
	}
	return VK_SUCCESS;
}

VkResult fck_vk_fences_destroy(fck_vk_driver *driver, VkFence *fences, fckc_size_t count)
{
	for (fckc_size_t index = 0; index < count; index++)
	{
		(driver->DestroyFence(driver->device, fences[index], NULL));
		fences[index] = NULL;
	}
	return VK_SUCCESS;
}

VkResult fck_vk_semaphores_create(fck_vk_driver *driver, VkSemaphore *semaphore, fckc_size_t count)
{
	VkSemaphoreCreateInfo create_info;
	create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	create_info.pNext = NULL;
	create_info.flags = 0;
	for (fckc_size_t index = 0; index < count; index++)
	{
		VkResult result = fck_vk_error(driver->CreateSemaphore(driver->device, &create_info, NULL, &semaphore[index]));
		if (result != VK_SUCCESS)
		{
			return result;
		}
	}
	return VK_SUCCESS;
}

VkResult fck_vk_semaphores_destroy(fck_vk_driver *driver, VkSemaphore *semaphore, fckc_size_t count)
{
	for (fckc_size_t index = 0; index < count; index++)
	{
		driver->DestroySemaphore(driver->device, semaphore[index], NULL);
		semaphore[index] = NULL;
	}
	return VK_SUCCESS;
}

VkResult fck_vk_shader_module_load(fck_vk_driver *driver, fck_shader_desc desc, const char *path, VkShaderModule *shader)
{
	fck_shader_compiler compiler = fck_shader_compiler_create();

	fck_file shader_source = os->fs->open(path, "r");

	fckc_size_t size = os->fs->size(shader_source);
	char *text = (char *)kll_malloc(kll_heap, size);
	os->fs->read(shader_source, text, size);

	fck_hlsl_object hlsl = compiler.create_hlsl(&compiler, &desc, text);
	fck_spirv_object spirv = compiler.create_spirv(&compiler, &hlsl.generic);

	VkShaderModuleCreateInfo shader_info = (VkShaderModuleCreateInfo){
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = spirv.generic.souce_byte_size,
		.pCode = (const fckc_u32 *)spirv.generic.source,
	};

	VkResult result = fck_vk_error(driver->CreateShaderModule(driver->device, &shader_info, NULL, shader));

	compiler.destroy(&compiler, &hlsl.generic);
	compiler.destroy(&compiler, &spirv.generic);
	compiler.shutdown(&compiler);
	return result;
}

int main_test(fck_window *window, void *app, int (*tick)(void *));
int fck_macos_app_tick(void *app_state);

fck_macos_result fck_macos_app_init(void **app_state, int argc, char **argv)
{
	fck_macos_application *app = (fck_macos_application *)kll_malloc(kll_heap, sizeof(*app));
	memset(app, 0, sizeof(*app));
	*app_state = app;

	app->event_channel = os->event_channel->create(kll_heap, 64);

	fck_vk_runtime *runtime = &app->vk.runtime;

	runtime->window = os->win->create("fck-vk", 1400, 600);
	{
		// sht_loader *loader = sht_main();
		// sht_instance instance = loader->load(FCK_INSTANCE_VERSION);
		// fck_assert(loader->is_ok(instance));
		//
		// sht_driver driver = instance.vt->start(instance, &runtime->window);
		// fck_assert(instance.vt->is_ok(driver));

		main_test(&runtime->window, app, fck_macos_app_tick);
	}

	return FCK_MACOS_RESULT_CONTINUE;
	fck_vk_crash(fck_vk_instance_init(&app->vk.vk));
	fck_vk_crash(fck_vk_gpu_init(&app->vk.gpu, &app->vk.vk));
	fck_vk_crash(fck_vk_platform_init(&app->vk.platform, &app->vk.gpu, runtime->window, &runtime->surface));

	fck_vk_crash(fck_vk_queues_init(&app->vk.queues, &app->vk.gpu, runtime->surface));
	fck_vk_crash(fck_vk_driver_init(&app->vk.driver, &app->vk.queues));
	fck_vk_crash(sht_vk_memory_init(&app->vk.memory, &app->vk.driver, fck_megabytes(64)));

	VkExtent2D extent;
	fck_vk_crash(fck_vk_surface_size(&app->vk.gpu, &app->vk.runtime, &extent));
	sht_image_configuration config = (sht_image_configuration){
		.format = SHT_FORMAT_D16_UNORM,
		.width = extent.width,
		.height = extent.height,
		.transfer = SHT_TRANSFER_RETAINED,
		.usage = SHT_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT,
	};
	runtime->depth = app->vk.memory.image->create(app->vk.memory.bump, &config, SHT_MEMORY_GPU);
	runtime->depth_view = app->vk.memory.image->view(app->vk.memory.bump, runtime->depth, SHT_FORMAT_UNDEFINED);

	fck_vk_crash(fck_vk_command_init(&app->vk.command, &app->vk.driver, &app->vk.queues));
	fck_vk_crash(fck_vk_swapchain_init(&app->vk.swapchain, &app->vk.driver, &app->vk.queues, runtime->surface, extent));

	VkDescriptorPool descriptor_pool;
	fck_vk_crash(fck_vk_descriptor_pool_create(&app->vk.driver, &descriptor_pool));

	sht_buffer *buffers = runtime->uniform;
	const fckc_size_t count = fck_arraysize(runtime->uniform);
	fck_vk_crash(fck_vk_uniform_buffer_create(&app->vk.memory, &runtime->mvp, sizeof(runtime->mvp), buffers, count));

	fck_vk_crash(fck_vk_render_pass_create(&app->vk.driver, app->vk.swapchain.format, runtime->depth.format, &app->vk.runtime.render_pass));
	fck_vk_crash(fck_vk_frame_buffer_create(&app->vk.driver, app->vk.runtime.render_pass, extent, &app->vk.runtime.depth_view,
	                                        app->vk.swapchain.views, app->vk.swapchain.count, &app->vk.runtime.framebuffer));

	// These two are supposed to be HEAVILY generic and modifiable during/for runtime.
	fck_vk_driver *driver = &app->vk.driver;
	fck_vk_command *command = &app->vk.command;
	fck_vk_queues *queues = &app->vk.queues;
	fck_vk_crash(fck_create_vertex_buffer(&app->vk.memory, command, queues, &runtime->vertices, &runtime->indices));
	fck_vk_semaphores_create(driver, runtime->graphics_completed, fck_arraysize(runtime->graphics_completed));
	fck_vk_semaphores_create(driver, runtime->presentation_completed, fck_arraysize(runtime->presentation_completed));
	fck_vk_fences_create(driver, runtime->wait_fences, fck_arraysize(runtime->wait_fences));

	VkDescriptorSetLayout descriptor_set_layout;
	fck_vk_crash(fck_vk_descriptor_set_layout_create(driver, &descriptor_set_layout));

	// VkDescriptorSet descirptor_sets[count];
	fck_vk_crash(fck_vk_descriptor_set_create(driver, descriptor_pool, &descriptor_set_layout, 1, buffers, runtime->descs, count));

	typedef enum fck_standard_stage
	{
		FCK_STANDARD_STAGE_VERTEX,
		FCK_STANDARD_STAGE_FRAGMENT,
		FCK_STANDARD_STAGE_COUNT,
	} fck_standard_stage;

	VkShaderModule shaders[FCK_STANDARD_STAGE_COUNT];
	{
		const char *vert_path = "/Users/ruthenium/fck/render-vk/test/hlsl/triangle.vert";
		fck_shader_desc vert_desc = (fck_shader_desc){FCK_SHADER_VERTEX, "triangle-vert", "main"};
		fck_vk_shader_module_load(driver, vert_desc, vert_path, &shaders[FCK_STANDARD_STAGE_VERTEX]);

		const char *frag_path = "/Users/ruthenium/fck/render-vk/test/hlsl/triangle.frag";
		fck_shader_desc frag_desc = (fck_shader_desc){FCK_SHADER_FRAGMENT, "triangle-frag", "main"};
		fck_vk_shader_module_load(driver, frag_desc, frag_path, &shaders[FCK_STANDARD_STAGE_FRAGMENT]);
	}

	VkPipelineShaderStageCreateInfo stages[FCK_STANDARD_STAGE_COUNT] = {
		[FCK_STANDARD_STAGE_VERTEX] =
			{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.pNext = NULL,
				.pSpecializationInfo = NULL,
				.stage = VK_SHADER_STAGE_VERTEX_BIT,
				.flags = 0,
				.module = shaders[FCK_STANDARD_STAGE_VERTEX],
				.pName = "main",
			},
		[FCK_STANDARD_STAGE_FRAGMENT] =
			{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.pNext = NULL,
				.pSpecializationInfo = NULL,
				.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
				.flags = 0,
				.module = shaders[FCK_STANDARD_STAGE_FRAGMENT],
				.pName = "main",
			},
	};

	fck_vk_crash(fck_vk_graphics_pipeline_create(driver, app->vk.runtime.render_pass, &descriptor_set_layout, 1, stages,
	                                             fck_arraysize(stages), &app->vk.graphic_pipeline));

	app->vk.memory.reset(app->vk.memory.temp);
	return FCK_MACOS_RESULT_CONTINUE;
}

int fck_macos_app_tick(void *app_state)
{
	fck_macos_application *macos_app = (fck_macos_application *)app_state;

	os->event_channel->pump(macos_app->event_channel);

	void fck_event_log(fck_event * event);

	fck_event events[12];
	fckc_size_t count = os->event_channel->poll(macos_app->event_channel, events, fck_arraysize(events));
	for (fckc_size_t index = 0; index < count; index++)
	{
		// ...
	}

	return FCK_MACOS_RESULT_CONTINUE;
}

void fck_macos_app_quit(void *app_state, fck_macos_result result)
{
}

void fck_vk_resize(fck_vk_swapchain *swapchain, sht_memory *mem, fck_vk_runtime *runtime, VkRenderPass render_pass)
{
	fck_vk_driver *driver = swapchain->driver;
	VkDevice device = driver->device;

	VkExtent2D extent;
	fck_vk_crash(fck_vk_surface_size(driver->gpu, runtime, &extent));

	fck_vk_crash(driver->DeviceWaitIdle(device));
	fck_vk_swapchain_resize(swapchain, extent);
	fck_vk_frame_buffer_destroy(swapchain->driver, &runtime->framebuffer);
	mem->image->discard(mem->bump, &runtime->depth_view);
	mem->image->destroy(mem->bump, &runtime->depth);

	sht_image_configuration config = (sht_image_configuration){
		.format = SHT_FORMAT_D16_UNORM,
		.width = extent.width,
		.height = extent.height,
		.transfer = SHT_TRANSFER_RETAINED,
		.usage = SHT_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT,
	};
	runtime->depth = mem->image->create(mem->bump, &config, SHT_MEMORY_GPU);
	runtime->depth_view = mem->image->view(mem->bump, runtime->depth, SHT_FORMAT_UNDEFINED);
	fck_vk_frame_buffer_create(swapchain->driver, render_pass, extent, &runtime->depth_view, swapchain->views, swapchain->count,
	                           &runtime->framebuffer);

	fck_vk_fences_destroy(driver, runtime->wait_fences, fck_arraysize(runtime->wait_fences));
	fck_vk_semaphores_destroy(driver, runtime->graphics_completed, fck_arraysize(runtime->graphics_completed));
	fck_vk_semaphores_destroy(driver, runtime->presentation_completed, fck_arraysize(runtime->presentation_completed));

	fck_vk_fences_create(driver, runtime->wait_fences, fck_arraysize(runtime->wait_fences));
	fck_vk_semaphores_create(driver, runtime->graphics_completed, fck_arraysize(runtime->graphics_completed));
	fck_vk_semaphores_create(driver, runtime->presentation_completed, fck_arraysize(runtime->presentation_completed));
}

void fck_vk_app_render2(fck_vk_runtime *runtime, sht_memory *mem, fck_vk_swapchain *swapchain, fck_vk_queues *queues,
                        fck_vk_command *command, fck_vk_graphics_pipeline *graphic_pipeline)
{
	fck_vk_driver *driver = swapchain->driver;
	VkDevice device = driver->device;

	fckc_size_t frame_index = runtime->index;

	VkFence *wait_fence = &runtime->wait_fences[frame_index];
	VkResult result = driver->WaitForFences(device, 1, wait_fence, VK_TRUE, UINT64_MAX);
	fck_vk_crash(driver->ResetFences(device, 1, wait_fence));

	VkSemaphore *presentation_completed = &runtime->presentation_completed[frame_index];
	fckc_u32 image_index;
	result = swapchain->AcquireNextImageKHR( //
		device,                              //
		swapchain->swapchain,                //
		UINT64_MAX,                          //
		*presentation_completed,             //
		VK_NULL_HANDLE,                      //
		&image_index                         //
	);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || (result == VK_SUBOPTIMAL_KHR))
	{
		fck_vk_resize(swapchain, mem, runtime, runtime->render_pass);
		return;
	}
	if (result != VK_SUCCESS)
	{
		return;
	}

	fck_mvp mvp;

	static float x = 0.0f;
	x = x + 0.001f;

	float mvp_model[4][4] = {
		{1.0f, 0.0f, 0.0f, 0.0f},
		{0.0f, 1.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 1.0f, 0.0f},
		{x, 0.0f, 0.0f, 1.0f},
	};
	float mvp_view[4][4] = {
		{1.0f, 0.0f, 0.0f, 0.0f},
		{0.0f, 1.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 1.0f, 0.0f},
		{0.0f, 0.0f, 0.0f, 1.0f},
	};
	float mvp_projection[4][4] = {
		{0.666667f, 0.0f, 0.0f, 0.0f},
		{0.0f, 0.666667f, 0.0f, 0.0f},
		{0.0f, 0.0f, -1.0f, 0.0f},
		{0.0f, 0.0f, 0.0f, 1.0f},
	};

	// Then set:
	memcpy(mvp.projection, mvp_projection, sizeof(mvp_projection));
	memcpy(mvp.view, mvp_view, sizeof(mvp_view));
	memcpy(mvp.model, mvp_model, sizeof(mvp_model));

	memcpy(runtime->uniform[frame_index].cpu, &mvp, sizeof(mvp));

	const VkCommandBuffer command_buffer = command->buffer[runtime->index];
	command->ResetCommandBuffer(command_buffer, 0);

	VkCommandBufferBeginInfo cmd_buffer_info = {};
	cmd_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	fck_vk_crash(command->BeginCommandBuffer(command_buffer, &cmd_buffer_info));

	// Set clear values for all framebuffer attachments with loadOp set to clear
	// We use two attachments (color and depth) that are cleared at the start of the subpass and as such we need to set clear values for
	// both
	VkClearValue clearValues[2];
	clearValues[0] = (VkClearValue){.color = {0.0f, 0.0f, 0.2f, 1.0f}};
	clearValues[1] = (VkClearValue){.depthStencil = {1.0f, 0}};

	VkFramebuffer framebuffer = runtime->framebuffer.values[image_index];

	VkExtent2D extent;
	fck_vk_crash(fck_vk_surface_size(driver->gpu, runtime, &extent));

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.pNext = NULL;
	renderPassBeginInfo.renderPass = runtime->render_pass;
	renderPassBeginInfo.renderArea.offset.x = 0;
	renderPassBeginInfo.renderArea.offset.y = 0;
	renderPassBeginInfo.renderArea.extent = extent;
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.pClearValues = clearValues;
	renderPassBeginInfo.framebuffer = framebuffer;

	// Start the first sub pass specified in our default render pass setup by the base class
	// This will clear the color and depth attachment
	command->CmdBeginRenderPass(command_buffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	// Update dynamic viewport state
	VkViewport viewport = {};
	viewport.height = (float)extent.height;
	viewport.width = (float)extent.width;
	viewport.minDepth = (float)0.0f;
	viewport.maxDepth = (float)1.0f;
	command->CmdSetViewport(command_buffer, 0, 1, &viewport);
	// Update dynamic scissor state
	VkRect2D scissor = {};
	scissor.extent.width = extent.width;
	scissor.extent.height = extent.height;
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	command->CmdSetScissor(command_buffer, 0, 1, &scissor);
	// Bind descriptor set for the current frame's   buffer, so the shader uses the data from that buffer for this draw
	command->CmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphic_pipeline->layout, 0, 1,
	                               &runtime->descs[frame_index], 0, NULL);
	// Bind the rendering pipeline
	// The pipeline (state object) contains all states of the rendering pipeline, binding it will set all the states specified at
	// pipeline creation time
	command->CmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphic_pipeline->pipeline);
	// Bind triangle vertex buffer (contains position and colors)
	VkDeviceSize offsets[1] = {0};

	VkBuffer vertex_buffer = (VkBuffer)runtime->vertices.buffer.gpu;
	command->CmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer, offsets);
	// Bind triangle index buffer
	command->CmdBindIndexBuffer(command_buffer, runtime->indices.buffer.gpu, 0, VK_INDEX_TYPE_UINT32);

	// Draw indexed triangle
	command->CmdDrawIndexed(command_buffer, runtime->indices.count, 1, 0, 0, 0);
	command->CmdEndRenderPass(command_buffer);
	// Ending the render pass will add an implicit barrier transitioning the frame buffer color attachment to
	// VK_IMAGE_LAYOUT_PRESENT_SRC_KHR for presenting it to the windowing system
	fck_vk_crash(command->EndCommandBuffer(command_buffer));

	// Submit the command buffer to the graphics queue

	// Pipeline stage at which the queue submission will wait (via pWaitSemaphores)
	VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	// The submit info structure specifies a command buffer queue submission batch
	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pWaitDstStageMask = &waitStageMask; // Pointer to the list of pipeline stages that the semaphore waits will occur at
	submit_info.pCommandBuffers = &command_buffer;  // Command buffers(s) to execute in this batch (submission)
	submit_info.commandBufferCount = 1;             // We submit a single command buffer

	//
	VkSemaphore *graphics_completed = &runtime->graphics_completed[frame_index];

	// Semaphore to wait upon before the submitted command buffer starts executing
	submit_info.pWaitSemaphores = presentation_completed;
	submit_info.waitSemaphoreCount = 1;
	// Semaphore to be signaled when command buffers have completed
	submit_info.pSignalSemaphores = graphics_completed;
	submit_info.signalSemaphoreCount = 1;

	// Submit to the graphics queue passing a wait fence
	VkQueue graphic_queue = fck_vk_queues_get_primary(queues, device, FCK_VK_QUEUE_GRAPHIC);
	fck_vk_crash(queues->QueueSubmit(graphic_queue, 1, &submit_info, *wait_fence));

	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = graphics_completed;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &swapchain->swapchain;
	present_info.pImageIndices = &image_index;

	VkQueue present_queue = fck_vk_queues_get_primary(queues, device, FCK_VK_QUEUE_PRESENT);
	result = queues->QueuePresentKHR(present_queue, &present_info);

	// Select the next frame to render to, based on the max. no. of concurrent frames
	runtime->index = (runtime->index + 1) % FCK_VK_IMAGE_COUNT;

	// I do not think this would happen, and even if, we can just move on to the next frame. Whatever we did before is already invalid
	// if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR))
	//{
	//	fck_vk_resize(swapchain, runtime, graphic_pipeline->render_pass);
	//	return;
	//}
	// else if (result != VK_SUCCESS)
	//{
	//	// ?????
	//	return;
	//}
}

void fck_macos_app_render(void *app_state)
{
	return;
	fck_macos_application *app = (fck_macos_application *)app_state;
	fck_vk_driver *driver = &app->vk.driver;
	fck_vk_queues *queues = &app->vk.queues;
	fck_vk_swapchain *sc = &app->vk.swapchain;
	fck_vk_runtime *runtime = &app->vk.runtime;
	fck_vk_command *command = &app->vk.command;
	sht_memory *memory = &app->vk.memory;
	fck_vk_graphics_pipeline *pipeline = &app->vk.graphic_pipeline;

	fck_vk_app_render2(runtime, memory, sc, queues, command, pipeline);
}

int main(int argc, char *argv[])
{
	struct fck_app_api *app = NULL;
	fck_macos_result result = fck_macos_app_init((void **)&app, argc, argv);
	// Make this work without touching the core loop!
	for (;;)
	{
		result = fck_macos_app_tick(app);
		if (result != FCK_MACOS_RESULT_CONTINUE)
		{
			break;
		}
		fck_macos_app_render(app);
	}

	fck_macos_app_quit(app, result);
	return 0;
}
