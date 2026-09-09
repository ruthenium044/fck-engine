
#include "sht_render.h"
#include "fckc_math.h"
#include "sht_vk.internal.h"

#include <fck_os.h>
#include <fckc_assert.h>

#include <string.h>

#include <fck_hash.h>
#include <kll.h>
#include <kll_malloc.h>

#include <fck_apis.h>
#include <fckc_inttypes.h>

#include <fck_shader.h>

#define VK_NO_PROTOTYPES
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

// Maybe inline? Then we can share in static builds with unity builds.
// Meh, I let it happen
static fck_api_registry *apis;

#define sht_invalidate(pointer_to_value) memset((pointer_to_value), 0x00, sizeof(*(pointer_to_value)))

static sht_bss_vt               bss;
static sht_command_buffer_vt    sht_command_buffer_vt_api;
static sht_graphics_pipeline_vt sht_graphics_pipeline_vt_api;
static sht_render_pass_vt       sht_render_pass_vt_api;
static sht_swapchain_vt         sht_swapchain_vt_api;
static sht_driver_vt            sht_driver_vt_api;
static sht_instance_vt          sht_instance_vt_api;
static sht_render_api           sht_loader_api;

static const VkDescriptorType sht_binding_type_to_vk_desc_type[] = {
	[sht_binding_none]           = VK_DESCRIPTOR_TYPE_MAX_ENUM, // Let's fuck things up
	[sht_binding_storage]        = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
	[sht_binding_uniform]        = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
	[sht_binding_readonly_image] = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
};

#define VK_LOG(type, msg, ...) os->io->log("[VK_ALLOC][%s] " msg "", type, ##__VA_ARGS__)

static void *VKAPI_PTR sht_vk_default_allocation(void *userdata, size_t size, size_t alignment, VkSystemAllocationScope scope)
{
	(void)userdata;
	void *ptr = kll_malloc(kll->system, size);
	VK_LOG("ALLOC", "Size: %zu, Align: %zu, Scope: %d -> Addr: %p", size, alignment, scope, ptr);
	return ptr;
}

static void *VKAPI_PTR sht_vk_default_reallocation(void *userdata, void *pOriginal, size_t size, size_t alignment,
                                                   VkSystemAllocationScope scope)
{
	(void)userdata;
	(void)alignment;
	void *ptr = kll_realloc(kll->system, pOriginal, size);
	VK_LOG("REALLOC", "Old: %p, New Size: %zu, Scope: %d -> New Addr: %p", pOriginal, size, scope, ptr);
	return ptr;
}

static void VKAPI_PTR sht_vk_default_free(void *userdata, void *pMemory)
{
	(void)userdata;
	if (pMemory)
	{
		VK_LOG("FREE", "Addr: %p", pMemory);
		kll_free(kll->system, pMemory);
	}
}

static void VKAPI_PTR sht_vk_default_internal_allocation_notification(void *userdata, size_t size, VkInternalAllocationType type,
                                                                      VkSystemAllocationScope scope)
{
	(void)userdata;
	VK_LOG("INT_ALLOC", "Size: %zu, Type: %d, Scope: %d", size, type, scope);
}

static void VKAPI_PTR sht_vk_default_internal_free_notification(void *userdata, size_t size, VkInternalAllocationType type,
                                                                VkSystemAllocationScope scope)
{
	(void)userdata;
	VK_LOG("INT_FREE", "Size: %zu, Type: %d, Scope: %d", size, type, scope);
}

static VkAllocationCallbacks log_allocation_callbacks = {
	.pUserData             = NULL,
	.pfnAllocation         = sht_vk_default_allocation,
	.pfnReallocation       = sht_vk_default_reallocation,
	.pfnFree               = sht_vk_default_free,
	.pfnInternalAllocation = sht_vk_default_internal_allocation_notification,
	.pfnInternalFree       = sht_vk_default_internal_free_notification,
};
static VkAllocationCallbacks *default_allocation_callbacks = NULL;

static VkResult sht_vk_descriptor_pool_create(sht_vk_driver *driver, sht_binding_desc *desc, VkDescriptorPool *descriptor_pool)
{
	// We need to tell the API the number of max. requested descriptors per type
	// This example only one descriptor type (uniform buffer)
	// We have one buffer (and as such descriptor) per frame
	// This part is utterly backward and dumb. lol
	const fckc_u32 set_copies_cacacity = sht_vk_bss_descriptor_set_bind_copies;

	fckc_u32             counts[sht_binding_count]                = {0};
	VkDescriptorPoolSize descriptor_pool_sizes[sht_binding_count] = {0};
	fckc_u32             descriptor_pool_size_count               = 0;

	for (fckc_size_t index = 0; index < desc->count; index++)
	{
		const sht_binding *binding = desc->bindings + index;
		counts[binding->type]      = counts[binding->type] + 1;
	}

	for (fckc_size_t index = 0; index < sht_binding_count; index++)
	{
		const fckc_u32 count = counts[index];
		if (count > 0)
		{
			VkDescriptorPoolSize *pool_size = descriptor_pool_sizes + descriptor_pool_size_count;
			pool_size->type                 = sht_binding_type_to_vk_desc_type[index];
			pool_size->descriptorCount      = set_copies_cacacity * count;
			descriptor_pool_size_count      = descriptor_pool_size_count + 1;
		}
	}
	// For additional types you need to add new entries in the type count list
	// E.g. for two combined image samplers :
	// descriptor_type_counts[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	// descriptor_type_counts[1].descriptorCount = 2;

	// Create the global descriptor pool
	// All descriptors used in this example are allocated from this pool
	VkDescriptorPoolCreateInfo desciptor_pool_create_info;
	desciptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	desciptor_pool_create_info.pNext = NULL;
	desciptor_pool_create_info.flags = 0;
	// Set the max. number of descriptor sets that can be requested from this pool (requesting beyond this limit will result in an error)
	// Our sample will create one set per uniform buffer per frame

	desciptor_pool_create_info.maxSets       = 1 + set_copies_cacacity;
	desciptor_pool_create_info.poolSizeCount = descriptor_pool_size_count;
	desciptor_pool_create_info.pPoolSizes    = descriptor_pool_sizes;

	return sht_vk_error(
		driver->CreateDescriptorPool(driver->device, &desciptor_pool_create_info, default_allocation_callbacks, descriptor_pool));
}

static VkShaderStageFlags sht_vk_shader_stage_flags_from_sht_stage_flags(sht_stage_flags flags)
{
	VkShaderStageFlags result = 0;
	if (sht_test(flags, sht_stage_vertex_shader))
	{
		result = result | VK_SHADER_STAGE_VERTEX_BIT;
	}
	if (sht_test(flags, sht_stage_fragment_shader))
	{
		result = result | VK_SHADER_STAGE_FRAGMENT_BIT;
	}
	return result;
}

static VkResult sht_vk_descriptor_set_layout_create(sht_vk_driver *driver, sht_binding_desc *desc,
                                                    VkDescriptorSetLayout *descriptor_set_layout)
{
	// We declare that the vertex shader stage is expecting a uniform buffer
	VkDevice device = driver->device;

	VkDescriptorSetLayoutBinding bindings[16];
	sht_vk_assert(fck_arraysize(bindings) >= desc->count);

	for (fckc_size_t index = 0; index < desc->count; index++)
	{
		VkDescriptorSetLayoutBinding *binding      = bindings + index;
		const sht_binding            *desc_binding = desc->bindings + index;
		binding->binding                           = desc_binding->id;
		binding->descriptorType                    = sht_binding_type_to_vk_desc_type[desc_binding->type];
		binding->descriptorCount                   = 1;
		binding->stageFlags                        = sht_vk_shader_stage_flags_from_sht_stage_flags(desc_binding->stages);
		binding->pImmutableSamplers                = NULL;
	}

	VkDescriptorSetLayoutCreateInfo descriptor_layout_create_info = {0};
	descriptor_layout_create_info.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptor_layout_create_info.pNext                           = NULL;
	descriptor_layout_create_info.bindingCount                    = desc->count;
	descriptor_layout_create_info.pBindings                       = bindings;
	return sht_vk_error(
		driver->CreateDescriptorSetLayout(device, &descriptor_layout_create_info, default_allocation_callbacks, descriptor_set_layout));
}

static void sht_vk_descriptor_set_update_buffer(sht_vk_driver *driver, VkDescriptorSet set, sht_binding *binding, sht_buffer *buffer,
                                                VkDeviceSize offset)
{
	// The buffer's information is passed using a descriptor info structure
	VkDescriptorBufferInfo buffer_info = {0};
	buffer_info.buffer                 = (VkBuffer)buffer->gpu;
	buffer_info.offset                 = offset;
	buffer_info.range                  = VK_WHOLE_SIZE;

	// Update the descriptor set determining the shader binding points
	// For every binding point used in a shader there needs to be one
	// descriptor set matching that binding point
	const VkDescriptorType type = sht_binding_type_to_vk_desc_type[binding->type];

	VkWriteDescriptorSet write_descriptor_set = {0};
	write_descriptor_set.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_set.pNext                = NULL;
	write_descriptor_set.dstSet               = set;
	write_descriptor_set.dstBinding           = binding->id;
	write_descriptor_set.dstArrayElement      = 0;
	write_descriptor_set.descriptorCount      = 1;
	write_descriptor_set.descriptorType       = type;
	write_descriptor_set.pImageInfo           = NULL;
	write_descriptor_set.pBufferInfo          = &buffer_info;
	write_descriptor_set.pTexelBufferView     = NULL;

	driver->UpdateDescriptorSets(driver->device, 1, &write_descriptor_set, 0, NULL);
}

static void sht_vk_descriptor_set_update_image(sht_vk_driver *driver, VkDescriptorSet set, sht_binding *binding,
                                               const sht_image_view *views, const sht_sampler *samplers, fckc_size_t count)
{
	// The buffer's information is passed using a descriptor info structure
	sht_vk_assert(count <= 32 && "Need more image_infos...");

	VkDescriptorImageInfo image_infos[32]; // For now
	for (fckc_size_t index = 0; index < count; index++)
	{
		VkDescriptorImageInfo *image_info = image_infos + index;
		image_info->imageLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		image_info->imageView             = (VkImageView)views[index].gpu;
		image_info->sampler               = (VkSampler)samplers[index].handle;
	}
	// Update the descriptor set determining the shader binding points
	// For every binding point used in a shader there needs to be one
	// descriptor set matching that binding point
	VkWriteDescriptorSet write_descriptor_set = {0};
	write_descriptor_set.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_set.pNext                = NULL;
	write_descriptor_set.dstSet               = set;
	write_descriptor_set.dstBinding           = binding->id;
	write_descriptor_set.dstArrayElement      = 0;
	write_descriptor_set.descriptorType       = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write_descriptor_set.pImageInfo           = image_infos;
	write_descriptor_set.descriptorCount      = count;
	write_descriptor_set.pBufferInfo          = NULL;
	write_descriptor_set.pTexelBufferView     = NULL;

	driver->UpdateDescriptorSets(driver->device, 1, &write_descriptor_set, 0, NULL);
}

static VkResult sht_vk_descriptor_set_create(sht_vk_driver *driver, VkDescriptorPool descriptor_pool, VkDescriptorSetLayout set_layout,
                                             VkDescriptorSet *descriptor_set, fckc_size_t count)
{
	// Allocate one descriptor set per frame from the global descriptor pool
	for (fckc_u32 i = 0; i < count; i++)
	{
		VkDescriptorSetAllocateInfo alloc_info;
		alloc_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		alloc_info.descriptorPool     = descriptor_pool;
		alloc_info.pSetLayouts        = &set_layout;
		alloc_info.descriptorSetCount = 1;
		alloc_info.pNext              = NULL;
		const VkResult result         = sht_vk_error(driver->AllocateDescriptorSets(driver->device, &alloc_info, &descriptor_set[i]));
		if (result != VK_SUCCESS)
		{
			return result;
		}
	}
	return VK_SUCCESS;
}

static VkFilter sht_sampler_filter_to_vk_filter(sht_filter filter)
{
	switch (filter)
	{
	case sht_filter_nearest:
		return VK_FILTER_NEAREST;
	case sht_filter_linear:
		return VK_FILTER_LINEAR;
	}
	return VK_FILTER_NEAREST;
}

static VkSamplerMipmapMode sht_sampler_filter_to_vk_mipmap_mode(sht_filter filter)
{
	switch (filter)
	{
	case sht_filter_nearest:
		return VK_SAMPLER_MIPMAP_MODE_NEAREST;
	case sht_filter_linear:
		return VK_SAMPLER_MIPMAP_MODE_LINEAR;
	}
	return VK_SAMPLER_MIPMAP_MODE_NEAREST;
}

static sht_sampler sht_driver_create_sampler(sht_driver driver, fck_alias(sht_filter, fckc_u32) filter)
{
	sht_vk_driver *vk_driver = (sht_vk_driver *)driver.handle;
	VkSampler      sampler;

	VkSamplerCreateInfo info     = {0};
	info.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	info.magFilter               = sht_sampler_filter_to_vk_filter(filter);
	info.minFilter               = sht_sampler_filter_to_vk_filter(filter);
	info.addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	info.addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	info.addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	info.anisotropyEnable        = VK_FALSE;
	info.maxAnisotropy           = 0.0f;
	info.borderColor             = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
	info.unnormalizedCoordinates = VK_FALSE;
	info.compareEnable           = VK_FALSE;
	info.compareOp               = VK_COMPARE_OP_NEVER;
	info.mipmapMode              = sht_sampler_filter_to_vk_mipmap_mode(filter);
	info.mipLodBias              = 0.0f;
	info.minLod                  = 0.0f;
	info.maxLod                  = 0.0f;
	vk_driver->CreateSampler(vk_driver->device, &info, default_allocation_callbacks, &sampler);
	return (sht_sampler){.handle = (sht_handle *)sampler};
}

static void sht_driver_destroy_sampler(sht_driver driver, sht_sampler *sampler)
{
	sht_vk_driver *vk_driver = (sht_vk_driver *)driver.handle;
	vk_driver->DestroySampler(vk_driver->device, (VkSampler)sampler->handle, default_allocation_callbacks);
	sht_invalidate(sampler);
}

static VKAPI_ATTR VkBool32 VKAPI_CALL sht_vk_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
                                                            VkDebugUtilsMessageTypeFlagsEXT             type,
                                                            const VkDebugUtilsMessengerCallbackDataEXT *data, void *userdata)
{
	// string_VkObjectType
	(void)userdata;
	(void)type;
	if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
	{
		os->io->log("VK-Info:\t%s", data->pMessage);
	}
	if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		os->io->log("VK-Warn:\t%s", data->pMessage);
	}
	if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{
		os->io->log("VK-Error:\t%s", data->pMessage);
	}

	return VK_FALSE; // Always return false unless you want to trigger a layer failure
}

static VkResult sht_vk_instance_init(sht_vk_instance *vk)
{
	VkApplicationInfo appInfo;
	appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pNext              = NULL;
	appInfo.pApplicationName   = NULL;
	appInfo.applicationVersion = 0;
	appInfo.pEngineName        = "sht-vk";
	appInfo.engineVersion      = VK_MAKE_VERSION(0, 0, 1);
	appInfo.apiVersion         = VK_MAKE_VERSION(1, 0, 0);

	// Wait a fucking second lol
#if defined(_WIN32)
	vk->so = os->so->load("vulkan-1.dll");
#else // We handle unix later
	vk->so = os->so->load("libvulkan.1.dylib");
#endif
	sht_vk_load_function(vk, vk->so, CreateInstance);
	sht_vk_load_function(vk, vk->so, DestroyInstance);

	VkInstanceCreateInfo instance_create_info = {0};
	const char          *instance_extension_names[16];
	fckc_size_t          instance_extension_count = 0;
	instance_create_info.flags                    = 0;

	static const char *layer_names[] = {"VK_LAYER_KHRONOS_validation"};

	// instance_extension_names[instance_extension_count++] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
	// instance_create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
	sht_vk_platform_adjust_instance(&instance_create_info);
	sht_vk_platform_adjust_extensions(instance_extension_names, &instance_extension_count);

	instance_extension_names[instance_extension_count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
	instance_extension_names[instance_extension_count++] = VK_KHR_SURFACE_EXTENSION_NAME;
	instance_extension_names[instance_extension_count++] = VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME;
	instance_extension_names[instance_extension_count++] = VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME;

	instance_create_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_create_info.pApplicationInfo        = &appInfo;
	instance_create_info.ppEnabledLayerNames     = layer_names;
	instance_create_info.enabledExtensionCount   = instance_extension_count;
	instance_create_info.ppEnabledExtensionNames = instance_extension_names;
	instance_create_info.enabledLayerCount       = fck_arraysize(layer_names);

	VkDebugUtilsMessengerCreateInfoEXT createInfo = {0};
	createInfo.sType                              = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
	                             VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT /*|*/
		/* VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT*/;
	createInfo.pfnUserCallback = sht_vk_debug_callback;
	createInfo.pUserData       = NULL;
	instance_create_info.pNext = (const void *)&createInfo;

	os->io->log("%d", vk->CreateInstance ? 1 : 0);

	return sht_vk_error(vk->CreateInstance(&instance_create_info, default_allocation_callbacks, &vk->instance));
}

static VkResult sht_vk_gpu_init(sht_vk_instance *instance, sht_vk_gpu *gpu)
{
	gpu->vk = instance;

	sht_vk_load_function(gpu, instance->so, EnumeratePhysicalDevices);
	sht_vk_load_function(gpu, instance->so, GetPhysicalDeviceProperties);
	sht_vk_load_function(gpu, instance->so, GetPhysicalDeviceFeatures);
	sht_vk_load_function(gpu, instance->so, GetPhysicalDeviceMemoryProperties);
	sht_vk_load_function(gpu, instance->so, GetPhysicalDeviceQueueFamilyProperties);
	sht_vk_load_function(gpu, instance->so, GetPhysicalDeviceFormatProperties);

	sht_vk_load_function(gpu, instance->so, EnumerateDeviceExtensionProperties);
	sht_vk_load_function(gpu, instance->so, GetPhysicalDeviceSurfaceSupportKHR);
	sht_vk_load_function(gpu, instance->so, GetPhysicalDeviceSurfacePresentModesKHR);
	sht_vk_load_function(gpu, instance->so, GetPhysicalDeviceSurfaceFormatsKHR);
	sht_vk_load_function(gpu, instance->so, GetPhysicalDeviceSurfaceCapabilitiesKHR);

	return VK_SUCCESS;
}

static VkResult sht_vk_queues_init(sht_vk_queues *queues, sht_vk_gpu *gpu, VkSurfaceKHR surface)
{
	queues->gpu = gpu;

	sht_vk_load_function(queues, gpu->vk->so, QueueSubmit);
	sht_vk_load_function(queues, gpu->vk->so, QueuePresentKHR);
	sht_vk_load_function(queues, gpu->vk->so, GetDeviceQueue);

	VkQueueFamilyProperties queue_family_properties[8];
	const fckc_u32          queue_family_capacity = fck_arraysize(queue_family_properties);
	fckc_u32                queue_family_count;
	gpu->GetPhysicalDeviceQueueFamilyProperties(gpu->device, &queue_family_count, NULL);
	sht_vk_assert(queue_family_count <= queue_family_capacity);
	sht_vk_assert(queue_family_count >= 1);

	gpu->GetPhysicalDeviceQueueFamilyProperties(gpu->device, &queue_family_count, queue_family_properties);
	sht_vk_assert(queue_family_count >= 1);

	const fckc_u32 invalid_queue_family = (fckc_u32)(-1);
	queues->family[sht_queue_graphic]   = invalid_queue_family;
	queues->family[sht_queue_transfer]  = invalid_queue_family;
	queues->family[sht_queue_present]   = invalid_queue_family;
	queues->family[sht_queue_compute]   = invalid_queue_family;

	// This should do the trick!
	queues->primary[sht_queue_graphic]  = 0;
	queues->primary[sht_queue_transfer] = 0;
	queues->primary[sht_queue_present]  = 0;
	queues->primary[sht_queue_compute]  = 0;

	for (fckc_size_t i = 0; i < queue_family_count; i++)
	{
		if (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			queues->family[sht_queue_graphic] = i;
			break;
		}
	}
	sht_vk_assert(queues->family[sht_queue_graphic] != invalid_queue_family);

	for (fckc_size_t i = 0; i < queue_family_count; i++)
	{
		if (queue_family_properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
		{
			queues->family[sht_queue_compute] = i;
			break;
		}
	}
	sht_vk_assert(queues->family[sht_queue_compute] != invalid_queue_family);

	for (unsigned int i = 0; i < queue_family_count; i++)
	{
		if (queue_family_properties[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
		{
			queues->family[sht_queue_transfer] = i;
			break;
		}
	}
	sht_vk_assert(queues->family[sht_queue_transfer] != invalid_queue_family);

	for (unsigned int i = 0; i < queue_family_count; i++)
	{
		VkBool32       supports_present;
		const VkResult result = sht_vk_error(gpu->GetPhysicalDeviceSurfaceSupportKHR(gpu->device, i, surface, &supports_present));
		if (!sht_vk_success(result))
		{
			supports_present = VK_FALSE;
		}
		if (supports_present)
		{
			queues->family[sht_queue_present] = i;
			break;
		}
	}
	sht_vk_assert(queues->family[sht_queue_present] != invalid_queue_family);
	return VK_SUCCESS;
};

static fckc_u32 sht_vk_queues_get_family(sht_vk_queues const *queues, sht_queue_type type)
{
	return queues->family[type];
}

static VkQueue sht_vk_queues_get_primary(sht_vk_queues const *queues, VkDevice device, sht_queue_type type)
{
	VkQueue queue;
	queues->GetDeviceQueue(device, queues->family[type], queues->primary[type], &queue);
	return queue;
}

// Stolen from good ol VK, maybe it is better... probs
// Find a memory in `memoryTypeBitsRequirement` that includes all of `requiredProperties`
static VkBool32 sht_vk_query_memory_type_index(const VkPhysicalDeviceMemoryProperties *properties, fckc_u32 mem_type_bits,
                                               VkMemoryPropertyFlags flags, fckc_u32 *type_index)
{
	const fckc_u32 count = properties->memoryTypeCount;
	for (fckc_u32 index = 0; index < count; ++index)
	{
		const fckc_u32 memory_type_bits = (1 << index);
		const int      is_required      = mem_type_bits & memory_type_bits;

		const VkMemoryPropertyFlags props            = properties->memoryTypes[index].propertyFlags;
		const int                   has_requirements = (props & flags) == flags;

		if (is_required && has_requirements)
		{
			*type_index = index;
			return VK_TRUE;
		}
	}

	// failed to find memory type
	return VK_FALSE;
}

static VkBufferUsageFlags sht_vk_usage_flags_from_config(sht_buffer_configuration *config)
{
	VkBufferUsageFlags flags = 0;
	if (sht_test(config->transfer, sht_trasnfer_source))
	{
		flags = flags | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	}
	if (sht_test(config->transfer, sht_transfer_target))
	{
		flags = flags | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	}

	if (sht_test(config->usage, sht_buffer_usage_uniform))
	{
		flags = flags | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	}
	if (sht_test(config->usage, sht_buffer_usage_storage))
	{
		flags = flags | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	}
	if (sht_test(config->usage, sht_buffer_usage_index))
	{
		flags = flags | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	}
	if (sht_test(config->usage, sht_buffer_usage_vertex))
	{
		flags = flags | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	}
	if (sht_test(config->usage, sht_buffer_usage_indirect))
	{
		flags = flags | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
	}
	return flags;
}

static VkFormat sht_vk_format_from_sht_format(sht_format format)
{
	switch (format)
	{
	case sht_format_undefined:
		return VK_FORMAT_UNDEFINED;
	case sht_format_r8g8b8a8_unorm:
		return VK_FORMAT_R8G8B8A8_UNORM;
	case sht_format_r8g8b8a8_srgb:
		return VK_FORMAT_R8G8B8A8_SRGB;
	case sht_format_b8g8r8a8_unorm:
		return VK_FORMAT_B8G8R8A8_UNORM;
	case sht_format_b8g8r8a8_srgb:
		return VK_FORMAT_B8G8R8A8_SRGB;
	case sht_format_d16_unorm:
		return VK_FORMAT_D16_UNORM;
	case sht_format_r32g32b32_sfloat:
		return VK_FORMAT_R32G32B32_SFLOAT;
	case sht_format_r32_sfloat:
		return VK_FORMAT_R32_SFLOAT;
	case sht_format_r32_G32_sfloat:
		return VK_FORMAT_R32G32_SFLOAT;
	case sht_format_r32g32b32a32_sfloat:
		return VK_FORMAT_R32G32B32A32_SFLOAT;
	}
	return VK_FORMAT_UNDEFINED;
}

static sht_format sht_vk_format_to_sht_format(VkFormat format)
{
	switch (format)
	{
	case VK_FORMAT_UNDEFINED:
		return sht_format_undefined;
	case VK_FORMAT_R8G8B8A8_UNORM:
		return sht_format_r8g8b8a8_unorm;
	case VK_FORMAT_R8G8B8A8_SRGB:
		return sht_format_r8g8b8a8_srgb;
	case VK_FORMAT_B8G8R8A8_UNORM:
		return sht_format_b8g8r8a8_unorm;
	case VK_FORMAT_B8G8R8A8_SRGB:
		return sht_format_b8g8r8a8_srgb;
	case VK_FORMAT_D16_UNORM:
		return sht_format_d16_unorm;
	case VK_FORMAT_R32G32B32_SFLOAT:
		return sht_format_r32g32b32_sfloat;
	case VK_FORMAT_R32_SFLOAT:
		return sht_format_r32_sfloat;
	case VK_FORMAT_R32G32_SFLOAT:
		return sht_format_r32_G32_sfloat;
	case VK_FORMAT_R32G32B32A32_SFLOAT:
		return sht_format_r32g32b32a32_sfloat;
	default:
		return sht_format_undefined;
	}
}

static VkImageUsageFlags sht_vk_usage_flags_from_image_config(sht_image_configuration *config)
{
	VkBufferUsageFlags flags = 0;
	if (sht_test(config->transfer, sht_trasnfer_source))
	{
		flags = flags | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	}
	if (sht_test(config->transfer, sht_transfer_target))
	{
		flags = flags | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	}

	if (sht_test(config->usage, sht_image_usage_sampled))
	{
		flags = flags | VK_IMAGE_USAGE_SAMPLED_BIT;
	}
	if (sht_test(config->usage, sht_image_usage_storage))
	{
		flags = flags | VK_IMAGE_USAGE_STORAGE_BIT;
	}
	if (sht_test(config->usage, sht_image_usage_color_attachment))
	{
		flags = flags | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	}
	if (sht_test(config->usage, sht_image_usage_depth_stencil_attachment))
	{
		flags = flags | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	}
	if (sht_test(config->usage, sht_image_usage_input_attachment))
	{
		flags = flags | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
	}
	// if (sht_test(config->usage, SHT_IMAGE_USAGE_TRANSIENT_ATTACHMENT))
	//{
	//	flags = flags | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
	// }
	return flags;
}

static int sht_vk_image_has_color_aspect(sht_image_usage_flags usage)
{
	const int result = sht_test(usage, sht_image_usage_sampled) ||          //
	                   sht_test(usage, sht_image_usage_storage) ||          //
	                   sht_test(usage, sht_image_usage_color_attachment) || //
	                   sht_test(usage, sht_image_usage_input_attachment);   //
	return result;
}

static VkImageAspectFlags sht_vk_image_aspect_from_config(sht_image_usage_flags usage)
{
	// TODO: SUPPORT FOR STENCIL!! We could see if the format is two-dimensional? i do not know :/ Need to read specs
	VkImageAspectFlagBits flags = VK_IMAGE_ASPECT_NONE; // NOLINT
	if (sht_test(usage, sht_image_usage_depth_stencil_attachment))
	{
		flags = flags | VK_IMAGE_ASPECT_DEPTH_BIT;
	}

	if (sht_vk_image_has_color_aspect(usage))
	{
		flags = flags | VK_IMAGE_ASPECT_COLOR_BIT;
	}
	return flags;
}

static sht_format sht_vk_image_resolve_format_from_config(sht_image_configuration *config)
{
	if (sht_test(config->usage, sht_image_usage_sampled))
	{
		return sht_format_b8g8r8a8_unorm;
	}
	if (sht_test(config->usage, sht_image_usage_storage))
	{
		return sht_format_b8g8r8a8_unorm;
	}
	if (sht_test(config->usage, sht_image_usage_color_attachment))
	{
		return sht_format_b8g8r8a8_unorm;
	}
	if (sht_test(config->usage, sht_image_usage_depth_stencil_attachment))
	{
		return sht_format_d16_unorm;
	}
	// if (sht_test(config->usage, SHT_IMAGE_USAGE_TRANSIENT_ATTACHMENT))
	//{
	//	return SHT_FORMAT_B8G8R8A8_UNORM;
	// }
	if (sht_test(config->usage, sht_image_usage_input_attachment))
	{
		return sht_format_undefined;
	}

	return sht_format_undefined;
}

static sht_image sht_memory_arena_image_create(sht_memory_arena *mem, const sht_image_configuration *incoming_config,
                                               sht_memory_type memory_type)
{
	sht_vk_driver *driver = (sht_vk_driver *)mem->owner;

	sht_image_configuration  configuration = *incoming_config;
	sht_image_configuration *config        = &configuration;

	if (config->format == sht_format_undefined)
	{
		config->format = sht_vk_image_resolve_format_from_config(config);
		// If format is still undefined, let's pray the render backend wiill help us out!
	}

	const VkImageUsageFlags usage  = sht_vk_usage_flags_from_image_config(config);
	const VkFormat          format = sht_vk_format_from_sht_format(config->format);

	VkFormatProperties props;
	driver->gpu->GetPhysicalDeviceFormatProperties(driver->gpu->device, format, &props);

	VkImageCreateInfo image_info = {0};
	if (sht_test(config->usage, sht_image_usage_depth_stencil_attachment))
	{
		if (props.linearTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			image_info.tiling = VK_IMAGE_TILING_LINEAR;
		}
		else if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
		}
		else
		{
			// Whoops, we screwed. Maybe log, I should do more logging...
			return (sht_image){0};
		}
	}
	else if (sht_test(config->usage, sht_image_usage_color_attachment))
	{
		if (props.linearTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT)
		{
			image_info.tiling = VK_IMAGE_TILING_LINEAR;
		}
		else if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT)
		{
			image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
		}
		else
		{
			// Whoops, we screwed. Maybe log, I should do more logging...
			return (sht_image){0};
		}
	}
	else if (sht_test(config->usage, sht_image_usage_sampled))
	{
		if (props.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)
		{
			image_info.tiling = VK_IMAGE_TILING_LINEAR;
		}
		else if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)
		{
			image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
		}
		else
		{
			// Whoops, we screwed. Maybe log, I should do more logging...
			return (sht_image){0};
		}
	}

	image_info.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.pNext                 = NULL;
	image_info.flags                 = 0;
	image_info.imageType             = VK_IMAGE_TYPE_2D;
	image_info.format                = format;
	image_info.extent.width          = config->width;
	image_info.extent.height         = config->height;
	image_info.extent.depth          = 1;
	image_info.mipLevels             = 1;
	image_info.arrayLayers           = 1;
	image_info.samples               = VK_SAMPLE_COUNT_1_BIT;
	image_info.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage                 = usage;
	image_info.queueFamilyIndexCount = 0;
	image_info.pQueueFamilyIndices   = NULL;
	image_info.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;

	fckc_size_t *offset = &mem->offset[memory_type];

	VkImage image;
	sht_vk_crash(driver->CreateImage(driver->device, &image_info, default_allocation_callbacks, &image));

	VkMemoryRequirements memory_requirements;
	driver->GetImageMemoryRequirements(driver->device, image, &memory_requirements);

	const fckc_size_t capacity = mem->capacity[memory_type];
	if (*offset + memory_requirements.size > capacity)
	{
		*offset = 0;
	}
	*offset = fckc_align(*offset, memory_requirements.alignment);

	VkDeviceMemory memory = (VkDeviceMemory)mem->heaps[memory_type];
	sht_vk_crash(driver->BindImageMemory(driver->device, image, memory, *offset));

	sht_image out;
	out.cpu    = NULL;
	out.heap   = (sht_heap *)memory;
	out.gpu    = image;
	out.width  = config->width;
	out.height = config->height;
	out.format = config->format;
	out.usage  = config->usage;
	if (memory_type == sht_memory_cpu)
	{
		void *cpu;
		out.cpu = (void *)((fckc_u8 *)mem->cpu[sht_memory_cpu] + *offset);
	}

	*offset = *offset + memory_requirements.size;
	return out;
}

static sht_image_view sht_memory_arena_image_view(sht_memory_arena *mem, sht_image image, fck_alias(sht_format, fckc_u32) incoming_format)
{
	if (incoming_format == sht_format_undefined)
	{
		incoming_format = image.format;
	}

	sht_vk_driver *driver = (sht_vk_driver *)mem->owner;

	VkFormat           format      = sht_vk_format_from_sht_format(incoming_format);
	VkImageAspectFlags aspect_mask = sht_vk_image_aspect_from_config(image.usage);

	VkImageViewCreateInfo view_info           = {0};
	view_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_info.pNext                           = NULL;
	view_info.format                          = format;
	view_info.components.r                    = VK_COMPONENT_SWIZZLE_R;
	view_info.components.g                    = VK_COMPONENT_SWIZZLE_G;
	view_info.components.b                    = VK_COMPONENT_SWIZZLE_B;
	view_info.components.a                    = VK_COMPONENT_SWIZZLE_A;
	view_info.subresourceRange.aspectMask     = aspect_mask;
	view_info.subresourceRange.baseMipLevel   = 0;
	view_info.subresourceRange.levelCount     = 1;
	view_info.subresourceRange.baseArrayLayer = 0;
	view_info.subresourceRange.layerCount     = 1;
	view_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
	view_info.flags                           = 0;
	view_info.image                           = (VkImage)image.gpu;

	VkImageView view;
	sht_vk_crash(driver->CreateImageView(driver->device, &view_info, default_allocation_callbacks, &view));

	sht_image_view out;
	out.format = incoming_format;
	out.gpu    = (void *)view;
	out.height = image.height;
	out.width  = image.width;
	return out;
}

static void sht_memory_arena_image_discard(sht_memory_arena *mem, sht_image_view *view)
{
	sht_vk_driver *driver = (sht_vk_driver *)mem->owner;
	driver->DestroyImageView(driver->device, (VkImageView)view->gpu, default_allocation_callbacks);
	view->gpu    = VK_NULL_HANDLE;
	view->format = VK_FORMAT_UNDEFINED;
}

static void sht_memory_arena_image_destroy(sht_memory_arena *mem, sht_image *image)
{
	sht_vk_driver *driver = (sht_vk_driver *)mem->owner;

	driver->DestroyImage(driver->device, (VkImage)image->gpu, default_allocation_callbacks);
	image->gpu    = VK_NULL_HANDLE;
	image->cpu    = NULL;
	image->format = sht_format_undefined;
	image->width  = 0;
	image->height = 0;
	image->heap   = (sht_heap *)VK_NULL_HANDLE;
}

static sht_buffer sht_memory_arena_malloc(sht_memory_arena *mem, sht_buffer_configuration *config, sht_memory_type memory_type)
{
	sht_vk_driver *driver = (sht_vk_driver *)mem->owner;
	VkDevice       device = driver->device;

	VkBufferCreateInfo create_info = {0};
	// We are lazy with this one just because
	create_info.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	create_info.pNext              = NULL;
	create_info.size               = config->size;
	create_info.flags              = (VkBufferCreateFlags)0;

	// OUT OF SCOPE
	create_info.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
	create_info.queueFamilyIndexCount = 0;
	create_info.pQueueFamilyIndices   = NULL;
	// !OUT OF SCOPE

	create_info.usage = sht_vk_usage_flags_from_config(config);

	fckc_size_t *offset = &mem->offset[memory_type];

	VkBuffer buffer;
	sht_vk_crash(driver->CreateBuffer(device, &create_info, default_allocation_callbacks, &buffer));

	VkMemoryRequirements memory_requirements;
	driver->GetBufferMemoryRequirements(device, buffer, &memory_requirements);

	const fckc_size_t capacity = mem->capacity[memory_type];
	if (*offset + config->size > capacity)
	{
		*offset = 0;
	}

	*offset = fckc_align(*offset, memory_requirements.alignment);

	VkDeviceMemory memory = (VkDeviceMemory)mem->heaps[memory_type];
	sht_vk_crash(driver->BindBufferMemory(device, buffer, memory, *offset));

	sht_buffer out;
	out.size = config->size;
	out.cpu  = NULL;
	out.heap = (sht_heap *)memory;
	out.gpu  = buffer;

	if (memory_type == sht_memory_cpu)
	{
		void *cpu;
		out.cpu = (void *)((fckc_u8 *)mem->cpu[sht_memory_cpu] + *offset);
	}

	*offset = *offset + config->size;

	return out;
}

static void sht_memory_arena_free(sht_memory_arena *mem, sht_buffer *buffer)
{
	sht_vk_driver *driver = (sht_vk_driver *)mem->owner;

	driver->DestroyBuffer(driver->device, (VkBuffer)buffer->gpu, default_allocation_callbacks);
	buffer->gpu  = VK_NULL_HANDLE;
	buffer->cpu  = NULL;
	buffer->size = 0;
	buffer->heap = (sht_heap *)VK_NULL_HANDLE;
}

static void sht_memory_arena_reset(sht_memory_arena *mem)
{
	sht_vk_driver *driver = (sht_vk_driver *)mem->owner;

	for (fckc_size_t index = 0; index < sht_memory_count; index++)
	{
		mem->offset[index] = 0;
	}
}

static VkDeviceMemory sht_memory_arena_of(sht_memory_arena *mem, sht_heap *heap)
{
	for (fckc_size_t i = 0; i < fck_arraysize(mem->heaps); i++)
	{
		VkDeviceMemory memory = (VkDeviceMemory)mem->heaps[i];
		if (memory == (VkDeviceMemory)heap)
		{
			return memory;
		}
	}
	return VK_NULL_HANDLE;
}

static VkResult sht_memory_arena_init(sht_memory_arena *mem, sht_vk_driver *driver, fckc_size_t size)
{
	mem->owner = (sht_handle *)driver;
	VkPhysicalDeviceMemoryProperties properties;
	driver->gpu->GetPhysicalDeviceMemoryProperties(driver->gpu->device, &properties);

	const VkMemoryPropertyFlags configs[sht_memory_count] = {
		[sht_memory_gpu] = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		[sht_memory_cpu] = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	};

	VkMemoryAllocateInfo info;
	info.sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	info.pNext          = NULL;
	info.allocationSize = size;

	for (fckc_size_t index = 0; index < fck_arraysize(configs); index++)
	{
		if (!sht_vk_query_memory_type_index(&properties, ~0, configs[index], &info.memoryTypeIndex))
		{
			// Idk...
			sht_vk_crash(VK_ERROR_UNKNOWN);
		}
		VkDeviceMemory device_memory = mem->heaps[index];
		sht_vk_crash(driver->AllocateMemory(driver->device, &info, default_allocation_callbacks, &device_memory));
		mem->heaps[index] = (sht_heap *)device_memory;

		mem->capacity[index] = size;
		mem->offset[index]   = 0;
		mem->cpu[index]      = NULL;
	}

	VkDeviceMemory device_memory = mem->heaps[sht_memory_cpu];
	sht_vk_crash(driver->MapMemory(driver->device, device_memory, 0, VK_WHOLE_SIZE, 0, &mem->cpu[sht_memory_cpu]));
	return VK_SUCCESS;
}

static void sht_memory_arena_destroy(sht_memory_arena *mem)
{
	sht_vk_driver *driver = (sht_vk_driver *)mem->owner;
	for (fckc_size_t index = 0; index < fck_arraysize(mem->heaps); index++)
	{
		VkDeviceMemory device_memory = (VkDeviceMemory)mem->heaps[index];
		driver->FreeMemory(driver->device, device_memory, default_allocation_callbacks);
	}
	sht_invalidate(mem);
}

static sht_memory_arena *sht_vk_memory_of(sht_memory *mem, sht_heap *heap)
{
	for (fckc_size_t index = 0; index < fck_arraysize(mem->objects); index++)
	{
		sht_memory_arena *object = mem->objects + index;
		VkDeviceMemory    memory = sht_memory_arena_of(object, heap);
		if (memory != VK_NULL_HANDLE)
		{
			return object;
		}
	}
	return NULL;
}

static sht_bool32 sht_image_is_ok(sht_image *image)
{
	return image->gpu != NULL;
}

static sht_bool32 sht_buffer_is_ok(sht_buffer buffer)
{
	return buffer.gpu != NULL;
}

static sht_bool32 sht_vk_image_recreate(sht_memory_arena *mem, sht_image *image, sht_extent extent, sht_image_view *views,
                                        fckc_size_t view_count)
{
	// Validate heap, but whatever!

	sht_image_configuration config = (sht_image_configuration){
		.format   = image->format,
		.height   = extent.height,
		.width    = extent.width,
		.usage    = image->usage,
		.transfer = sht_transfer_retained,
	};

	sht_memory_type memory_type = sht_memory_gpu;
	if (image->cpu != NULL)
	{
		memory_type = sht_memory_cpu;
	}

	// Recreate Image
	sht_image next = sht_memory_arena_image_create(mem, &config, memory_type);
	if (!sht_image_is_ok(&next))
	{
		return sht_false;
	}

	// Recreate views
	for (size_t index = 0; index < view_count; index++)
	{
		sht_image_view *view   = views + index;
		sht_format      format = view->format;
		sht_memory_arena_image_discard(mem, view);
		*view = sht_memory_arena_image_view(mem, next, format);
	}
	sht_memory_arena_image_destroy(mem, image);
	*image = next;
	return sht_true;
}

static void sht_vk_memory_destroy(sht_memory *mem)
{
	sht_memory_arena_destroy(mem->bump);
	sht_memory_arena_destroy(mem->temp);
	sht_invalidate(mem);
}

static VkResult sht_vk_memory_init(sht_memory *mem, sht_vk_driver *driver, fckc_size_t size)
{
	static sht_memory_image image_api = {
		.create   = sht_memory_arena_image_create,
		.destroy  = sht_memory_arena_image_destroy,
		.view     = sht_memory_arena_image_view,
		.discard  = sht_memory_arena_image_discard,
		.is_ok    = sht_image_is_ok,
		.recreate = sht_vk_image_recreate,
	};

	VkResult result;
	sht_vk_assert(fck_arraysize(mem->objects) == 2); // NOLINT
	result = sht_vk_error(sht_memory_arena_init(&mem->objects[0], driver, size));
	sht_vk_propagate_on_error(result);
	result = sht_vk_error(sht_memory_arena_init(&mem->objects[1], driver, size));
	sht_vk_propagate_on_error(result);

	mem->bump   = &mem->objects[0];
	mem->temp   = &mem->objects[1];
	mem->of     = sht_vk_memory_of;
	mem->malloc = sht_memory_arena_malloc;
	mem->free   = sht_memory_arena_free;
	mem->reset  = sht_memory_arena_reset;
	mem->is_ok  = sht_buffer_is_ok;
	mem->image  = &image_api;
	return result;
}

static VkResult sht_vk_driver_init(sht_vk_driver *driver, sht_vk_queues *queues)
{
	driver->gpu = queues->gpu;

	sht_vk_load_function(driver, driver->gpu->vk->so, CreateDevice);
	sht_vk_load_function(driver, driver->gpu->vk->so, DestroyDevice);
	sht_vk_load_function(driver, driver->gpu->vk->so, DestroySurfaceKHR);

	sht_vk_load_function(driver, driver->gpu->vk->so, CreateShaderModule);
	sht_vk_load_function(driver, driver->gpu->vk->so, DestroyShaderModule);

	sht_vk_load_function(driver, driver->gpu->vk->so, CreateImage);
	sht_vk_load_function(driver, driver->gpu->vk->so, DestroyImageView);
	sht_vk_load_function(driver, driver->gpu->vk->so, DestroyImage);
	sht_vk_load_function(driver, driver->gpu->vk->so, GetImageMemoryRequirements);
	sht_vk_load_function(driver, driver->gpu->vk->so, AllocateMemory);
	sht_vk_load_function(driver, driver->gpu->vk->so, FreeMemory);
	sht_vk_load_function(driver, driver->gpu->vk->so, BindImageMemory);
	sht_vk_load_function(driver, driver->gpu->vk->so, CreateImageView);
	sht_vk_load_function(driver, driver->gpu->vk->so, CreateBuffer);
	sht_vk_load_function(driver, driver->gpu->vk->so, DestroyBuffer);
	sht_vk_load_function(driver, driver->gpu->vk->so, CreateRenderPass);
	sht_vk_load_function(driver, driver->gpu->vk->so, DestroyRenderPass);

	sht_vk_load_function(driver, driver->gpu->vk->so, GetBufferMemoryRequirements);
	sht_vk_load_function(driver, driver->gpu->vk->so, BindBufferMemory);
	sht_vk_load_function(driver, driver->gpu->vk->so, MapMemory);
	sht_vk_load_function(driver, driver->gpu->vk->so, UnmapMemory);
	sht_vk_load_function(driver, driver->gpu->vk->so, CreateSemaphore);
	sht_vk_load_function(driver, driver->gpu->vk->so, DestroySemaphore);
	sht_vk_load_function(driver, driver->gpu->vk->so, CreateFence);

	sht_vk_load_function(driver, driver->gpu->vk->so, CreateDescriptorSetLayout);
	sht_vk_load_function(driver, driver->gpu->vk->so, CreateDescriptorPool);
	sht_vk_load_function(driver, driver->gpu->vk->so, UpdateDescriptorSets);
	sht_vk_load_function(driver, driver->gpu->vk->so, ResetDescriptorPool);

	sht_vk_load_function(driver, driver->gpu->vk->so, AllocateDescriptorSets);

	sht_vk_load_function(driver, driver->gpu->vk->so, FreeDescriptorSets);
	sht_vk_load_function(driver, driver->gpu->vk->so, DestroyDescriptorPool);
	sht_vk_load_function(driver, driver->gpu->vk->so, DestroyDescriptorSetLayout);

	sht_vk_load_function(driver, driver->gpu->vk->so, CreatePipelineLayout);
	sht_vk_load_function(driver, driver->gpu->vk->so, DestroyPipelineLayout);

	sht_vk_load_function(driver, driver->gpu->vk->so, CreateGraphicsPipelines);
	sht_vk_load_function(driver, driver->gpu->vk->so, CreatePipelineCache);
	sht_vk_load_function(driver, driver->gpu->vk->so, DestroyPipeline);
	sht_vk_load_function(driver, driver->gpu->vk->so, DestroyPipelineCache);

	// The good shit
	sht_vk_load_function(driver, driver->gpu->vk->so, CreateFence);
	sht_vk_load_function(driver, driver->gpu->vk->so, DestroyFence);
	sht_vk_load_function(driver, driver->gpu->vk->so, ResetFences);
	sht_vk_load_function(driver, driver->gpu->vk->so, WaitForFences);
	sht_vk_load_function(driver, driver->gpu->vk->so, GetFenceStatus);

	sht_vk_load_function(driver, driver->gpu->vk->so, DeviceWaitIdle);

	sht_vk_load_function(driver, driver->gpu->vk->so, CreateSampler);
	sht_vk_load_function(driver, driver->gpu->vk->so, DestroySampler);

	sht_vk_load_function(driver, driver->gpu->vk->so, CreateFramebuffer);
	sht_vk_load_function(driver, driver->gpu->vk->so, DestroyFramebuffer);

	VkPhysicalDeviceProperties       properties;
	VkPhysicalDeviceFeatures         features;
	VkPhysicalDeviceMemoryProperties memory_properties;
	driver->gpu->GetPhysicalDeviceProperties(driver->gpu->device, &properties);
	driver->gpu->GetPhysicalDeviceFeatures(driver->gpu->device, &features);
	driver->gpu->GetPhysicalDeviceMemoryProperties(driver->gpu->device, &memory_properties);

	float                   queue_priorities[1] = {0.0};
	VkDeviceQueueCreateInfo queue_info          = {0};
	queue_info.queueFamilyIndex                 = sht_vk_queues_get_family(queues, sht_queue_graphic);
	queue_info.sType                            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_info.pNext                            = NULL;
	queue_info.queueCount                       = 1;
	queue_info.pQueuePriorities                 = queue_priorities;

	fckc_size_t device_extension_count = 0;
	const char *device_extension_names[16];
	device_extension_names[device_extension_count++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

	VkExtensionProperties available_extensions[256];
	fckc_u32              available_extension_count = 0;

	if (driver->gpu->EnumerateDeviceExtensionProperties(driver->gpu->device, NULL, &available_extension_count, NULL))
	{
		os->io->log("Failed to count physical device extension properties");
		available_extension_count = 0;
	}
	available_extension_count = fck_min(available_extension_count, fck_arraysize(available_extensions));

	if (!driver->gpu->EnumerateDeviceExtensionProperties(driver->gpu->device, NULL, &available_extension_count, available_extensions))
	{
		for (fckc_u32 index = 0; index < available_extension_count; ++index)
		{
			if (strcmp(available_extensions[index].extensionName, "VK_KHR_portability_subset") == 0)
			{
				device_extension_names[device_extension_count++] = "VK_KHR_portability_subset";
				break;
			}
		}
	}

	VkDeviceCreateInfo device_info  = {0};
	// Deprecated
	device_info.enabledLayerCount   = 0;
	device_info.ppEnabledLayerNames = NULL;
	// !Depreated

	device_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_info.pNext                   = NULL;
	device_info.queueCreateInfoCount    = 1;
	device_info.pQueueCreateInfos       = &queue_info;
	device_info.ppEnabledExtensionNames = device_extension_names;
	device_info.enabledExtensionCount   = device_extension_count;
	device_info.pEnabledFeatures        = NULL;

	sht_vk_crash(driver->CreateDevice(driver->gpu->device, &device_info, default_allocation_callbacks, &driver->device));
	return VK_SUCCESS;
}

static VkResult sht_vk_command_init(sht_vk_command *command, sht_vk_driver *driver, sht_vk_queues *queues)
{
	command->driver = driver;

	sht_vk_load_function(command, driver->gpu->vk->so, CreateCommandPool);
	sht_vk_load_function(command, driver->gpu->vk->so, DestroyCommandPool);
	sht_vk_load_function(command, driver->gpu->vk->so, AllocateCommandBuffers);
	sht_vk_load_function(command, driver->gpu->vk->so, FreeCommandBuffers);
	sht_vk_load_function(command, driver->gpu->vk->so, BeginCommandBuffer);
	sht_vk_load_function(command, driver->gpu->vk->so, EndCommandBuffer);
	sht_vk_load_function(command, driver->gpu->vk->so, ResetCommandBuffer);
	sht_vk_load_function(command, driver->gpu->vk->so, CmdBindPipeline);
	sht_vk_load_function(command, driver->gpu->vk->so, CmdSetViewport);
	sht_vk_load_function(command, driver->gpu->vk->so, CmdSetScissor);
	sht_vk_load_function(command, driver->gpu->vk->so, CmdSetLineWidth);
	sht_vk_load_function(command, driver->gpu->vk->so, CmdSetDepthBias);
	sht_vk_load_function(command, driver->gpu->vk->so, CmdSetBlendConstants);
	sht_vk_load_function(command, driver->gpu->vk->so, CmdSetDepthBounds);
	sht_vk_load_function(command, driver->gpu->vk->so, CmdSetStencilCompareMask);
	sht_vk_load_function(command, driver->gpu->vk->so, CmdSetStencilWriteMask);
	sht_vk_load_function(command, driver->gpu->vk->so, CmdSetStencilReference);
	sht_vk_load_function(command, driver->gpu->vk->so, CmdBindDescriptorSets);
	sht_vk_load_function(command, driver->gpu->vk->so, CmdBindIndexBuffer);
	sht_vk_load_function(command, driver->gpu->vk->so, CmdBindVertexBuffers);
	sht_vk_load_function(command, driver->gpu->vk->so, CmdDraw);
	sht_vk_load_function(command, driver->gpu->vk->so, CmdDrawIndexed);
	sht_vk_load_function(command, driver->gpu->vk->so, CmdDrawIndirect);
	sht_vk_load_function(command, driver->gpu->vk->so, CmdDrawIndexedIndirect);
	sht_vk_load_function(command, driver->gpu->vk->so, CmdDispatch);
	sht_vk_load_function(command, driver->gpu->vk->so, CmdDispatchIndirect);
	sht_vk_load_function(command, driver->gpu->vk->so, CmdCopyBuffer);
	sht_vk_load_function(command, driver->gpu->vk->so, CmdCopyImage);
	sht_vk_load_function(command, driver->gpu->vk->so, CmdBlitImage);
	sht_vk_load_function(command, driver->gpu->vk->so, CmdCopyBufferToImage);
	sht_vk_load_function(command, driver->gpu->vk->so, CmdCopyImageToBuffer);
	sht_vk_load_function(command, driver->gpu->vk->so, CmdUpdateBuffer);
	sht_vk_load_function(command, driver->gpu->vk->so, CmdFillBuffer);
	sht_vk_load_function(command, driver->gpu->vk->so, CmdClearColorImage);
	sht_vk_load_function(command, driver->gpu->vk->so, CmdClearDepthStencilImage);
	sht_vk_load_function(command, driver->gpu->vk->so, CmdClearAttachments);
	sht_vk_load_function(command, driver->gpu->vk->so, CmdResolveImage);
	sht_vk_load_function(command, driver->gpu->vk->so, CmdSetEvent);
	sht_vk_load_function(command, driver->gpu->vk->so, CmdResetEvent);
	sht_vk_load_function(command, driver->gpu->vk->so, CmdWaitEvents);
	sht_vk_load_function(command, driver->gpu->vk->so, CmdPipelineBarrier);
	sht_vk_load_function(command, driver->gpu->vk->so, CmdBeginQuery);
	sht_vk_load_function(command, driver->gpu->vk->so, CmdEndQuery);
	sht_vk_load_function(command, driver->gpu->vk->so, CmdResetQueryPool);
	sht_vk_load_function(command, driver->gpu->vk->so, CmdWriteTimestamp);
	sht_vk_load_function(command, driver->gpu->vk->so, CmdCopyQueryPoolResults);
	sht_vk_load_function(command, driver->gpu->vk->so, CmdPushConstants);
	sht_vk_load_function(command, driver->gpu->vk->so, CmdBeginRenderPass);
	sht_vk_load_function(command, driver->gpu->vk->so, CmdNextSubpass);
	sht_vk_load_function(command, driver->gpu->vk->so, CmdEndRenderPass);
	sht_vk_load_function(command, driver->gpu->vk->so, CmdExecuteCommands);

	VkCommandPoolCreateInfo cmd_pool_info = {0};
	cmd_pool_info.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmd_pool_info.pNext                   = NULL;
	cmd_pool_info.queueFamilyIndex        = sht_vk_queues_get_family(queues, sht_queue_graphic);
	cmd_pool_info.flags                   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	sht_vk_crash(command->CreateCommandPool(driver->device, &cmd_pool_info, default_allocation_callbacks, &command->pool));

	VkCommandBufferAllocateInfo alloc_info = {0};
	alloc_info.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.pNext                       = NULL;
	alloc_info.commandPool                 = command->pool;
	alloc_info.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandBufferCount          = sht_frame_count;

	return sht_vk_error(command->AllocateCommandBuffers(driver->device, &alloc_info, command->buffers));
}

static void sht_vk_command_destroy(sht_vk_command *command)
{
	command->FreeCommandBuffers(command->driver->device, command->pool, fck_arraysize(command->buffers), command->buffers);
	command->DestroyCommandPool(command->driver->device, command->pool, default_allocation_callbacks);
	sht_invalidate(command);
}

static VkResult sht_vk_fences_create(sht_vk_driver *driver, VkFence *fences, fckc_size_t count)
{
	VkFenceCreateInfo create_info;
	create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	create_info.pNext = NULL;
	create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	for (fckc_size_t index = 0; index < count; index++)
	{
		const VkResult result =
			sht_vk_error(driver->CreateFence(driver->device, &create_info, default_allocation_callbacks, &fences[index]));
		if (result != VK_SUCCESS)
		{
			return result;
		}
	}
	return VK_SUCCESS;
}

static VkResult sht_vk_fences_destroy(sht_vk_driver *driver, VkFence *fences, fckc_size_t count)
{
	for (fckc_size_t index = 0; index < count; index++)
	{
		VkFence *fence = fences + index;
		(driver->DestroyFence(driver->device, *fence, default_allocation_callbacks));
		*fence = NULL;
	}
	return VK_SUCCESS;
}

static VkResult sht_vk_semaphores_create(sht_vk_driver *driver, VkSemaphore *semaphore, fckc_size_t count)
{
	VkSemaphoreCreateInfo create_info;
	create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	create_info.pNext = NULL;
	create_info.flags = 0;
	for (fckc_size_t index = 0; index < count; index++)
	{
		const VkResult result =
			sht_vk_error(driver->CreateSemaphore(driver->device, &create_info, default_allocation_callbacks, &semaphore[index]));
		if (result != VK_SUCCESS)
		{
			return result;
		}
	}
	return VK_SUCCESS;
}

static VkResult sht_vk_semaphores_destroy(sht_vk_driver *driver, VkSemaphore *semaphores, fckc_size_t count)
{
	for (fckc_size_t index = 0; index < count; index++)
	{
		VkSemaphore *semaphore = semaphores + index;

		driver->DestroySemaphore(driver->device, *semaphore, default_allocation_callbacks);
		*semaphore = NULL;
	}
	return VK_SUCCESS;
}

static VkResult sht_vk_common_sync_resources_create(sht_vk_common_sync_resources *sync, sht_vk_driver *driver)
{
	memset(sync, 0, sizeof(*sync));
	sync->index = 0;

	sht_vk_crash(sht_vk_semaphores_create(driver, sync->graphics_completed, fck_arraysize(sync->graphics_completed)));
	sht_vk_crash(sht_vk_semaphores_create(driver, sync->presentation_completed, fck_arraysize(sync->presentation_completed)));
	sht_vk_crash(sht_vk_fences_create(driver, sync->wait_fences, fck_arraysize(sync->wait_fences)));
	return VK_SUCCESS;
}

static void sht_vk_common_sync_resources_destroy(sht_vk_common_sync_resources *sync, sht_vk_driver *driver)
{
	sht_vk_fences_destroy(driver, sync->wait_fences, fck_arraysize(sync->wait_fences));
	sht_vk_semaphores_destroy(driver, sync->graphics_completed, fck_arraysize(sync->graphics_completed));
	sht_vk_semaphores_destroy(driver, sync->presentation_completed, fck_arraysize(sync->presentation_completed));
	sht_invalidate(sync);
}

static void sht_vk_swapchain_destroy(sht_vk_swapchain *swapchain)
{
	sht_vk_driver *driver = swapchain->driver;

	sht_vk_common_sync_resources_destroy(&swapchain->sync, driver);

	if (swapchain->swapchain != VK_NULL_HANDLE)
	{
		for (fckc_u32 i = 0; i < swapchain->count; i++)
		{
			driver->DestroyImageView(driver->device, swapchain->views[i], default_allocation_callbacks);
		}
	}

	swapchain->DestroySwapchainKHR(driver->device, swapchain->swapchain, default_allocation_callbacks);

	// swapchain.images -> Not allocated by us
	sht_invalidate(swapchain);
}

static VkResult sht_vk_swapchain_resize(sht_vk_swapchain *swapchain, sht_vk_driver *driver, VkExtent2D extent)
{
	// Resize
	sht_vk_swapchain old         = *swapchain;
	swapchain->info.imageExtent  = extent;
	swapchain->info.oldSwapchain = old.swapchain;

	VkResult result =
		sht_vk_error(swapchain->CreateSwapchainKHR(driver->device, &swapchain->info, default_allocation_callbacks, &swapchain->swapchain));
	sht_vk_crash(sht_vk_common_sync_resources_create(&swapchain->sync, driver));

	if (result != VK_SUCCESS)
	{
		return result;
	}

	if (old.swapchain != VK_NULL_HANDLE)
	{
		sht_vk_swapchain_destroy(&old);
	}

	// 4 should be plenty, we do not need to hug the images, only the views, but we will see...
	// swapchain_image_view_count = fck_arraysize(swapchain_images);
	// In any case, we can query them, in other cases we have views.
	result = sht_vk_error(swapchain->GetSwapchainImagesKHR(driver->device, swapchain->swapchain, &swapchain->count, NULL));
	result = sht_vk_error(swapchain->GetSwapchainImagesKHR(driver->device, swapchain->swapchain, &swapchain->count, swapchain->images));
	if (result != VK_SUCCESS)
	{
		return result;
	}

	for (fckc_u32 i = 0; i < swapchain->count; i++)
	{
		VkImageViewCreateInfo color_image_view           = {0};
		color_image_view.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		color_image_view.pNext                           = NULL;
		color_image_view.flags                           = 0;
		color_image_view.image                           = swapchain->images[i];
		color_image_view.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
		color_image_view.format                          = swapchain->info.imageFormat;
		color_image_view.components.r                    = VK_COMPONENT_SWIZZLE_R;
		color_image_view.components.g                    = VK_COMPONENT_SWIZZLE_G;
		color_image_view.components.b                    = VK_COMPONENT_SWIZZLE_B;
		color_image_view.components.a                    = VK_COMPONENT_SWIZZLE_A;
		color_image_view.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		color_image_view.subresourceRange.baseMipLevel   = 0;
		color_image_view.subresourceRange.levelCount     = 1;
		color_image_view.subresourceRange.baseArrayLayer = 0;
		color_image_view.subresourceRange.layerCount     = 1;

		result =
			sht_vk_error(driver->CreateImageView(driver->device, &color_image_view, default_allocation_callbacks, &swapchain->views[i]));
		if (result != VK_SUCCESS)
		{
			return result;
		}
	}
	return result;
}

static VkResult sht_vk_swapchain_init(sht_vk_swapchain *swapchain, sht_vk_driver *driver, VkSurfaceKHR surface, VkExtent2D extent)
{
	swapchain->driver = driver;
	sht_vk_load_function(swapchain, driver->gpu->vk->so, CreateSwapchainKHR);
	sht_vk_load_function(swapchain, driver->gpu->vk->so, GetSwapchainImagesKHR);
	sht_vk_load_function(swapchain, driver->gpu->vk->so, AcquireNextImageKHR);
	sht_vk_load_function(swapchain, driver->gpu->vk->so, DestroySwapchainKHR);

	swapchain->surface = surface;

	sht_vk_gpu    *gpu    = driver->gpu;
	sht_vk_queues *queues = &gpu->queues;

	// Get the list of VkFormats that are supported:
	fckc_u32 format_count;
	sht_vk_crash(gpu->GetPhysicalDeviceSurfaceFormatsKHR(gpu->device, surface, &format_count, NULL));

	VkSurfaceFormatKHR *formats = (VkSurfaceFormatKHR *)kll_malloc(kll->system, format_count * sizeof(VkSurfaceFormatKHR));
	sht_vk_crash(gpu->GetPhysicalDeviceSurfaceFormatsKHR(gpu->device, surface, &format_count, formats));
	sht_vk_assert(format_count >= 1); // That would be fucking weird, lol

	VkFormat format = VK_FORMAT_UNDEFINED;
	// If the format list includes just one entry of VK_FORMAT_UNDEFINED,
	// the surface has no preferred format.  Otherwise, at least one
	// supported format will be returned.
	if (format_count == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
	{
		// Does that actually happen?
		format = VK_FORMAT_B8G8R8A8_UNORM;
	}
	else
	{
		format = formats[0].format;
	}
	kll_free(kll->system, formats);

	VkSurfaceCapabilitiesKHR surface_capabilities;
	sht_vk_crash(gpu->GetPhysicalDeviceSurfaceCapabilitiesKHR(gpu->device, surface, &surface_capabilities));

	VkPresentModeKHR present_modes[16];
	const fckc_u32   present_modes_capacity = fck_arraysize(present_modes);
	fckc_u32         present_modes_count;
	sht_vk_crash(gpu->GetPhysicalDeviceSurfacePresentModesKHR(gpu->device, surface, &present_modes_count, NULL));
	sht_vk_assert(present_modes_count <= present_modes_capacity);
	sht_vk_assert(present_modes_count >= 1);

	sht_vk_crash(gpu->GetPhysicalDeviceSurfacePresentModesKHR( //
		gpu->device,                                           //
		surface,                                               //
		&present_modes_count,                                  //
		present_modes)                                         //
	);
	sht_vk_assert(present_modes_count >= 1);

	int available = 0;
	for (fckc_size_t index = 0; index < present_modes_count; index++)
	{
		const VkPresentModeKHR present_mode = present_modes[index];
		if (present_mode < 16)
		{
			// Some arbitrary value - The extensions have very high bits set
			available = available | (1 << present_mode);
		}
	}

	// TODO: Let higher-level configure this:
	// enum present_mode { prevent_tearing, asap, etc. }
	// The FIFO present mode is guaranteed by the spec to be supported
	// FIFO can also be the most sluggish one (at least on my device) We need to find a good selection method here!
	VkPresentModeKHR swapchain_present_mode        = VK_PRESENT_MODE_IMMEDIATE_KHR; // we can also use queried present modes
	fckc_u32         desired_swapchain_image_count = surface_capabilities.minImageCount;
	if (sht_test(available, (1 << VK_PRESENT_MODE_MAILBOX_KHR)))
	{
		if (surface_capabilities.minImageCount < 3)
		{
			if (surface_capabilities.maxImageCount > 3)
			{
				desired_swapchain_image_count = surface_capabilities.minImageCount + 1;
				swapchain_present_mode        = VK_PRESENT_MODE_MAILBOX_KHR;
			}
		}
	}

	// Determine the number of VkImage's to use in the swap chain.
	// We need to acquire only 1 presentable image at at time.
	// Asking for minImageCount images ensures that we can acquire
	// 1 presentable image as long as we present it before attempting
	// to acquire another.
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
	VkCompositeAlphaFlagBitsKHR       alpha          = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	const VkCompositeAlphaFlagBitsKHR alpha_flags[4] = {
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
	};
	for (fckc_u32 i = 0; i < fck_arraysize(alpha_flags); i++)
	{
		if (surface_capabilities.supportedCompositeAlpha & alpha_flags[i])
		{
			alpha = alpha_flags[i];
			break;
		}
	}

	VkSwapchainCreateInfoKHR swapchain_create_info = {0};
	swapchain_create_info.sType                    = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_create_info.pNext                    = NULL;
	swapchain_create_info.surface                  = surface;
	swapchain_create_info.minImageCount            = desired_swapchain_image_count;
	swapchain_create_info.imageFormat              = format;
	swapchain_create_info.imageExtent              = extent;
	swapchain_create_info.preTransform             = pre_transform;
	swapchain_create_info.compositeAlpha           = alpha;
	swapchain_create_info.imageArrayLayers         = 1;
	swapchain_create_info.presentMode              = swapchain_present_mode;
	swapchain_create_info.oldSwapchain             = VK_NULL_HANDLE;
	swapchain_create_info.clipped                  = VK_TRUE;
	swapchain_create_info.imageColorSpace          = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	swapchain_create_info.imageUsage               = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	swapchain_create_info.imageSharingMode         = VK_SHARING_MODE_EXCLUSIVE;
	swapchain_create_info.queueFamilyIndexCount    = 0;
	swapchain_create_info.pQueueFamilyIndices      = NULL;
	swapchain_create_info.oldSwapchain             = swapchain->swapchain;
	swapchain->swapchain                           = VK_NULL_HANDLE;

	fckc_u32 queue_family_indices[2] = {
		(fckc_u32)sht_vk_queues_get_family(queues, sht_queue_graphic),
		(fckc_u32)sht_vk_queues_get_family(queues, sht_queue_present),
	};
	if (queue_family_indices[0] != queue_family_indices[1])
	{
		// If the graphics and present queues are from different queue families,
		// we either have to explicitly transfer ownership of images between
		// the queues, or we have to create the swapchain with imageSharingMode
		// as VK_SHARING_MODE_CONCURRENT
		swapchain_create_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
		swapchain_create_info.queueFamilyIndexCount = 2;
		swapchain_create_info.pQueueFamilyIndices   = queue_family_indices;
	}
	swapchain->info  = swapchain_create_info;
	swapchain->count = 0;
	return sht_vk_swapchain_resize(swapchain, driver, extent);
}

static VkAttachmentLoadOp sht_vk_load_op_from_op(sht_memory_access_operation op)
{
	switch (op)
	{
	case sht_load:
		return VK_ATTACHMENT_LOAD_OP_LOAD;
	case sht_dont_care:
		return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	case sht_clear:
		return VK_ATTACHMENT_LOAD_OP_CLEAR;
	}
	return VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
}

static VkAttachmentStoreOp sht_vk_store_op_from_op(sht_memory_access_operation op)
{
	switch (op)
	{
	case sht_store:
		return VK_ATTACHMENT_STORE_OP_STORE;
	case sht_dont_care:
	case sht_clear:
		return VK_ATTACHMENT_STORE_OP_DONT_CARE;
	}
	return VK_ATTACHMENT_STORE_OP_MAX_ENUM;
}

static sht_vk_instance *sht_instance_to_vk(sht_instance instance)
{
	return (sht_vk_instance *)instance.handle;
}

static sht_vk_driver *sht_driver_to_vk(sht_driver driver)
{
	return (sht_vk_driver *)driver.handle;
}

static VkResult sht_vk_surface_size(sht_vk_gpu *gpu, VkSurfaceKHR surface, VkExtent2D *extent)
{
	VkSurfaceCapabilitiesKHR surfaceCaps;
	const VkResult           result = sht_vk_error(gpu->GetPhysicalDeviceSurfaceCapabilitiesKHR(gpu->device, surface, &surfaceCaps));
	if (result != VK_SUCCESS)
	{
		return result;
	}

	// Hmm... I am unsure
	if (surfaceCaps.currentExtent.width == (fckc_u32)-1)
	{
		extent->width  = surfaceCaps.maxImageExtent.width;
		extent->height = surfaceCaps.maxImageExtent.height;
	}
	else
	{
		extent->width  = surfaceCaps.currentExtent.width;
		extent->height = surfaceCaps.currentExtent.height;
	}
	return VK_SUCCESS;
}

static sht_extent sht_swapchain_extent(sht_swapchain swapchain)
{
	sht_vk_swapchain *sc     = (sht_vk_swapchain *)swapchain.handle;
	VkExtent2D        extent = (VkExtent2D){.width = 0, .height = 0};
	sht_vk_error(sht_vk_surface_size(sc->driver->gpu, sc->info.surface, &extent));
	return (sht_extent){.width = (fckc_f32)extent.width, .height = (fckc_f32)extent.height};
}

static sht_image_view sht_swapchain_get_view(sht_swapchain swapchain, fckc_u32 index)
{
	sht_vk_swapchain *sc = (sht_vk_swapchain *)swapchain.handle;
	if (index > sc->count)
	{
		return (sht_image_view){0};
	}

	VkImageView image_view = sc->views[index];

	const sht_extent extent = sht_swapchain_extent(swapchain);
	return (sht_image_view){
		.format = sht_vk_format_to_sht_format(sc->info.imageFormat),
		.gpu    = image_view,
		.height = extent.height,
		.width  = extent.width,
	};
}

static sht_vk_descriptor_pool_storage_key sht_vk_descriptor_pool_storage_create(sht_vk_driver                  *driver,
                                                                                sht_vk_descriptor_pool_storage *storage,
                                                                                sht_binding_desc               *desc)
{
	const fckc_size_t at                        = storage->count;
	storage->count                              = storage->count + 1;
	sht_vk_descriptor_pool_storage_entry *entry = storage->entries + at;
	entry->ref_count                            = entry->ref_count + 1;

	for (fckc_size_t index = 0; index < fck_arraysize(entry->dynamic_pools); index++)
	{
		sht_vk_crash(sht_vk_descriptor_pool_create(driver, desc, entry->dynamic_pools + index));
	}
	sht_vk_crash(sht_vk_descriptor_pool_create(driver, desc, &entry->constant_pool));

	sht_vk_crash(sht_vk_descriptor_set_layout_create(driver, desc, &entry->layout));

	VkPipelineLayoutCreateInfo pipeline_layout_create_info;
	pipeline_layout_create_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_create_info.pSetLayouts            = &entry->layout;
	pipeline_layout_create_info.setLayoutCount         = 1;
	pipeline_layout_create_info.pNext                  = NULL;
	pipeline_layout_create_info.flags                  = 0;
	pipeline_layout_create_info.pushConstantRangeCount = 0;
	pipeline_layout_create_info.pPushConstantRanges    = NULL;

	sht_vk_crash(
		driver->CreatePipelineLayout(driver->device, &pipeline_layout_create_info, default_allocation_callbacks, &entry->pipeline_layout));

	return (sht_vk_descriptor_pool_storage_key){.entry = storage->entries + at};
}

static sht_vk_descriptor_pool_storage_entry *sht_vk_descriptor_pool_storage_key_resolve(sht_vk_descriptor_pool_storage_key key)
{
	return key.entry;
}

static sht_bool32 sht_vk_descriptor_pool_storage_key_is_ok(sht_vk_descriptor_pool_storage_key key)
{
	return key.entry != NULL;
}

static void sht_vk_descriptor_pool_storage_key_release(sht_vk_driver *driver, sht_vk_descriptor_pool_storage_key *key)
{
	key->entry->ref_count = key->entry->ref_count - 1;
	if (key->entry->ref_count == 0)
	{
		// Or we defer it and call it a GC huehuehuehue
		driver->DestroyPipelineLayout(driver->device, key->entry->pipeline_layout, default_allocation_callbacks);
		key->entry->pipeline_layout = VK_NULL_HANDLE;
		driver->DestroyDescriptorSetLayout(driver->device, key->entry->layout, default_allocation_callbacks);
		key->entry->layout = VK_NULL_HANDLE;
		for (fckc_size_t index = 0; index < fck_arraysize(key->entry->dynamic_pools); index++)
		{
			driver->DestroyDescriptorPool(driver->device, key->entry->dynamic_pools[index], default_allocation_callbacks);
			key->entry->dynamic_pools[index] = VK_NULL_HANDLE;
		}
		driver->DestroyDescriptorPool(driver->device, key->entry->constant_pool, default_allocation_callbacks);
		key->entry->constant_pool = VK_NULL_HANDLE;
	}
	key->entry = NULL;
}

static void sht_vk_bss_destroy(sht_vk_bss *vk_bss, sht_vk_driver *driver)
{
	sht_vk_descriptor_pool_storage_key_release(driver, &vk_bss->pool_storage_key);

	for (fckc_size_t index = 0; index < fck_arraysize(vk_bss->buffer_backends); index++)
	{
		sht_bss_buffer_backends *backend = vk_bss->buffer_backends + index;
		for (fckc_size_t id = 0; id < fck_arraysize(backend->buffers); id++)
		{
			sht_buffer *buffer = backend->buffers + id;
			if (driver->memory.is_ok(*buffer))
			{
				driver->memory.free(driver->memory.bump, buffer);
			}
		}
	}
}

static void sht_bss_destroy(sht_bss *bss)
{
	sht_vk_bss    *vk_bss = (sht_vk_bss *)bss->handle;
	sht_vk_driver *driver = (sht_vk_driver *)bss->owner;

	/* Let's do this when refcount hits 0
	sht_vk_descriptor_pool_storage_key_release(driver, &vk_bss->pool_storage_key);

	for (fckc_size_t index = 0; index < fck_arraysize(vk_bss->buffer_backends); index++)
	{
	    sht_bss_buffer_backends *backend = vk_bss->buffer_backends + index;
	    for (fckc_size_t id = 0; id < fck_arraysize(backend->buffers); id++)
	    {
	        sht_buffer *buffer = backend->buffers + id;
	        if (driver->memory.is_ok(*buffer))
	        {
	            driver->memory.free(driver->memory.bump, buffer);
	        }
	    }
	}*/

	vk_bss->refcount = vk_bss->refcount - 1;
	if (vk_bss->refcount < 0)
	{
		sht_vk_bss_destroy(vk_bss, driver);
	}
	// Only free if VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT is set!
	// for (fckc_size_t index = 0; index < fck_arraysize(vk_bss->sets); index++)
	//{
	//	VkDescriptorSet *set = vk_bss->sets + index;
	//	driver->FreeDescriptorSets(driver->device, vk_bss->pool, 1, set);
	//}

	sht_invalidate(bss);
	// sht_invalidate(vk_bss);
}

static sht_bss sht_bss_create(sht_driver driver, sht_binding_desc *desc)
{
	sht_vk_driver *vk_driver = sht_driver_to_vk(driver);

	sht_vk_bss *vk_bss = NULL;
	for (fckc_size_t index = 0; index < vk_driver->storages.bss.count; index++)
	{
		// Find free bss
		if (vk_driver->storages.bss.handles[index].refcount < 0)
		{
			vk_bss = vk_driver->storages.bss.handles + index;
			fck_assert(vk_bss->refcount == -1); // I do not want to overflow :(
		}
	}

	if (vk_bss == NULL)
	{
		vk_bss                        = vk_driver->storages.bss.handles + vk_driver->storages.bss.count;
		vk_driver->storages.bss.count = vk_driver->storages.bss.count + 1;
	}

	vk_bss->refcount = 1;

	sht_vk_assert(vk_driver->storages.bss.count < fck_arraysize(vk_driver->storages.bss.handles));
	sht_invalidate(vk_bss);
	memset(vk_bss, 0, sizeof(*vk_bss));

	sht_vk_assert(fck_arraysize(vk_bss->desc.bindings) >= desc->count);
	memcpy(vk_bss->desc.bindings, desc->bindings, sizeof(*desc->bindings) * desc->count);
	vk_bss->desc.count = desc->count;
	memset(&vk_bss->nodes, 0, sizeof(vk_bss->nodes));
	// vk_bss->node.prev = vk_bss->node.next = NULL;

	vk_bss->pool_storage_key = sht_vk_descriptor_pool_storage_create(vk_driver, &vk_driver->storages.descriptor_pool, desc);
	sht_vk_descriptor_pool_storage_entry *pool_entry = sht_vk_descriptor_pool_storage_key_resolve(vk_bss->pool_storage_key);

	sht_vk_crash(sht_vk_descriptor_set_create(vk_driver, pool_entry->constant_pool, pool_entry->layout, vk_bss->baselines,
	                                          fck_arraysize(vk_bss->baselines)));

	memcpy((void *)vk_bss->latest, (void *)vk_bss->baselines, sizeof(vk_bss->baselines));

	return (sht_bss){.owner = (sht_handle *)vk_driver, .handle = (sht_handle *)vk_bss};
}

static void sht_vk_bss_storage_destroy(sht_vk_bss_storage *storage, sht_vk_driver *driver)
{
	for (fckc_size_t index = 0; index < storage->count; index++)
	{
		sht_vk_bss *bss = storage->handles + index;

		if (sht_vk_descriptor_pool_storage_key_is_ok(bss->pool_storage_key))
		{
			sht_bss temp = (sht_bss){.handle = bss, .owner = driver};
			sht_bss_destroy(&temp);
			os->io->log("WARNING: SHT BSS HAS NOT BEEN DESTROYED");
		}
	}
}

static sht_bool32 sht_vk_graphics_pipeline_storage_remove(sht_vk_graphics_pipeline_storage *storage, sht_vk_driver *driver,
                                                          sht_graphics_pipeline_key handle);

static void sht_vk_graphics_pipeline_storage_destroy(sht_vk_graphics_pipeline_storage *storage, sht_vk_driver *driver)
{
	for (fckc_size_t index = 0; index < fck_arraysize(storage->handles); index++)
	{
		sht_vk_graphics_pipeline       *gp  = storage->handles + index;
		const sht_graphics_pipeline_key key = storage->keys[index];
		if (sht_vk_graphics_pipeline_storage_remove(storage, driver, key))
		{
			os->io->log("WARNING: SHT GRAPHICS PIPLINE HAS NOT BEEN DESTROYED");
		}
	}
}

static void sht_vk_render_pass_storage_destroy(sht_vk_render_pass_storage *storage, sht_vk_driver *driver)
{
	for (fckc_size_t index = 0; index < storage->count; index++)
	{
		sht_vk_render_pass *pass = storage->handles + index;
		driver->DestroyRenderPass(driver->device, pass->handle, default_allocation_callbacks);
		sht_invalidate(pass);
	}
	storage->count = 0;
}

static void sht_vk_framebuffer_storage_destroy(sht_vk_framebuffer_storage *storage, sht_vk_driver *driver)
{
	for (fckc_size_t index = 0; index < storage->count; index++)
	{
		sht_vk_framebuffer *framebuffer = storage->handles + index;
		driver->DestroyFramebuffer(driver->device, framebuffer->handle, default_allocation_callbacks);
		sht_invalidate(framebuffer);
	}
	storage->count = 0;
}

static void sht_vk_resize(sht_vk_swapchain *swapchain)
{
	sht_vk_driver *driver = swapchain->driver;
	VkDevice       device = driver->device;

	sht_vk_crash(driver->DeviceWaitIdle(device));

	VkExtent2D extent;
	sht_vk_crash(sht_vk_surface_size(driver->gpu, swapchain->surface, &extent));

	sht_vk_swapchain_resize(swapchain, driver, extent);

	sht_vk_framebuffer_storage_destroy(&driver->storages.framebuffer, driver);
	sht_vk_render_pass_storage_destroy(&driver->storages.render_pass, driver);
}

static void sht_vk_command_buffer_bss_copy(sht_vk_driver *driver, sht_vk_bss *bss, VkDescriptorSet src, VkDescriptorSet dst)
{
	VkCopyDescriptorSet copies[sht_vk_bss_binding_capacity];
	for (fckc_size_t index = 0; index < bss->desc.count; index++)
	{
		sht_binding        *binding = bss->desc.bindings + index;
		VkCopyDescriptorSet copy    = {0};
		copy.sType                  = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
		copy.descriptorCount        = 1;
		copy.srcSet                 = src;
		copy.dstSet                 = dst;
		copy.srcBinding             = binding->id;
		copy.dstBinding             = binding->id;
		copies[index]               = copy;
	}

	driver->UpdateDescriptorSets(driver->device, 0, NULL, bss->desc.count, copies);
}

static sht_swapchain_state sht_swapchain_wait_and_acquire(sht_swapchain swapchain)
{
	// I think some changes need to be done!
	const fckc_u64 timeout = ~0LLU;

	sht_swapchain_state state = {0};

	sht_vk_swapchain             *sc   = (sht_vk_swapchain *)swapchain.handle;
	sht_vk_common_sync_resources *sync = &sc->sync;

	sht_vk_driver *driver = sc->driver;
	VkDevice       device = driver->device;

	VkFence *wait_fence = &sync->wait_fences[sync->index];
	VkResult result     = driver->WaitForFences(device, 1, wait_fence, VK_TRUE, timeout);
	sht_vk_crash(driver->ResetFences(device, 1, wait_fence));

	VkSemaphore *completed = &sync->presentation_completed[sync->index];
	state.index            = sync->index;

	fckc_u32 image_index;
	result = sc->AcquireNextImageKHR(device, sc->swapchain, timeout, *completed, VK_NULL_HANDLE, &image_index);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		state.resize = 1;
		sht_vk_resize(sc);
		return state;
	}

	if (result != VK_SUCCESS)
	{
		sht_vk_error(result);
		state.issues = 1;
		return state;
	}

	// We map the image index to the frame index. This sync/frame index now owns the swapchain image!!
	sync->frame_index_to_swapchain_image_index[sync->index] = image_index;
	// BANGER!
	// TODO: We also need the IMAGE not just the IMAGE VIEW
	// ALSO: WE need to propagate the image TO the end so we can create a pipeline barrier

	{
		sht_vk_bss_nodes *start = &driver->storages.bss.inflight;
		sht_vk_bss_node  *root  = driver->storages.bss.inflight.values + sync->index;

		sht_vk_bss_nodes *current = root->next;
		while (current != start)
		{
			sht_vk_bss *bss = (sht_vk_bss *)current;

			sht_vk_bss_node  *current_node = current->values + sync->index;
			sht_vk_bss_nodes *next         = current_node->next;

			current = next;

			sht_vk_bss_node *node = bss->nodes.values + sync->index;
			node->next            = NULL;
			node->prev            = NULL;

			const VkDescriptorSet src = bss->latest[sync->index];
			const VkDescriptorSet dst = bss->baselines[sync->index];
			sht_vk_command_buffer_bss_copy(driver, bss, src, dst);
			bss->latest[sync->index] = bss->baselines[sync->index];

			sht_bss_buffer_backends *buffer_backend = bss->buffer_backends + sync->index;
			memset(buffer_backend->offsets, 0, sizeof(buffer_backend->offsets));

			bss->refcount = bss->refcount - 1;
			if (bss->refcount < 0)
			{
				sht_vk_bss_destroy(bss, driver);

				// Out of mind, out of sight!
				os->io->log("I THINK WE CAN FREE!!!");
			}
		}
		root->next = &driver->storages.bss.inflight;
		root->prev = &driver->storages.bss.inflight;
	}

	const fckc_size_t pool_count = driver->storages.descriptor_pool.count;
	for (fckc_size_t index = 0; index < pool_count; index++)
	{
		sht_vk_descriptor_pool_storage_entry *entry = driver->storages.descriptor_pool.entries + index;
		if (entry->layout != VK_NULL_HANDLE)
		{
			driver->ResetDescriptorPool(driver->device, entry->dynamic_pools[sync->index], 0);
		}
	}

	state.view = sht_swapchain_get_view(swapchain, image_index);
	return state;
}

static sht_extent sht_swapchain_display(sht_swapchain swapchain)
{
	sht_vk_swapchain *sc     = (sht_vk_swapchain *)swapchain.handle;
	const VkExtent2D  extent = (VkExtent2D){.width = 0, .height = 0};
	int               w, h;
	sht_vk_assert(os->win->size(sc->driver->window, &w, &h));
	return (sht_extent){.width = (fckc_f32)w, .height = (fckc_f32)h};
}

static float sht_swapchain_scale(sht_swapchain swapchain)
{
	sht_vk_swapchain *sc      = (sht_vk_swapchain *)swapchain.handle;
	const sht_extent  extent  = sht_swapchain_extent(swapchain);
	const sht_extent  display = sht_swapchain_display(swapchain);
	return extent.width / display.width;
}

static sht_bool32 sht_swapchain_is_ready(sht_swapchain swapchain, const sht_swapchain_state *state)
{
	(void)swapchain;
	return state->issues == 0;
}

static sht_command_buffer sht_command_buffer_create(sht_driver driver)
{
	sht_vk_driver  *vk_driver = sht_driver_to_vk(driver);
	sht_vk_command *api       = &vk_driver->command;

	VkCommandBufferAllocateInfo command_buffer_alloc_info = {0};
	command_buffer_alloc_info.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	command_buffer_alloc_info.commandPool                 = api->pool;
	command_buffer_alloc_info.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	command_buffer_alloc_info.commandBufferCount          = 1;

	VkCommandBuffer command_buffer;
	sht_vk_crash(api->AllocateCommandBuffers(vk_driver->device, &command_buffer_alloc_info, &command_buffer));
	// We can make it fail later - If we identified the command buffer is in-flight, is_ok returns NULL command buffer
	VkCommandBufferBeginInfo command_buffer_begin_info = {0};
	command_buffer_begin_info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	command_buffer_begin_info.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	sht_vk_crash(api->BeginCommandBuffer(command_buffer, &command_buffer_begin_info));
	return (sht_command_buffer){.owner = &vk_driver->command, .handle = command_buffer};
}

static void sht_command_buffer_destroy(sht_command_buffer *command)
{
	sht_vk_command *api            = (sht_vk_command *)command->owner;
	VkCommandBuffer command_buffer = (VkCommandBuffer)command->handle;
	api->FreeCommandBuffers(api->driver->device, api->pool, 1, &command_buffer);
	sht_invalidate(command);
}

static sht_command_buffer sht_command_buffer_acquire(sht_driver driver, fckc_u32 index)
{
	// We can make it fail later - If we identified the command buffer is in-flight, is_ok returns NULL command buffer
	sht_vk_driver  *vk_driver      = sht_driver_to_vk(driver);
	sht_vk_command *api            = &vk_driver->command;
	VkCommandBuffer command_buffer = api->buffers[index];

	// TODO: Reinit the bss?! OH FUCK. Maybe do it lazily in bind and/or upload
	api->ResetCommandBuffer(command_buffer, 0);

	VkCommandBufferBeginInfo cmd_buffer_info = {0};
	cmd_buffer_info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	sht_vk_crash(api->BeginCommandBuffer(command_buffer, &cmd_buffer_info));

	const fckc_u32 image_index = vk_driver->swapchain.sync.frame_index_to_swapchain_image_index[index];
	sht_vk_assert(image_index < sht_vk_swapchain_image_capacity);

	VkImageMemoryBarrier imageBarrier;
	imageBarrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageBarrier.pNext                           = NULL;
	imageBarrier.srcAccessMask                   = 0;
	imageBarrier.dstAccessMask                   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	imageBarrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
	imageBarrier.newLayout                       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	imageBarrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
	imageBarrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
	imageBarrier.image                           = vk_driver->swapchain.images[image_index];
	imageBarrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
	imageBarrier.subresourceRange.baseMipLevel   = 0;
	imageBarrier.subresourceRange.levelCount     = 1;
	imageBarrier.subresourceRange.baseArrayLayer = 0;
	imageBarrier.subresourceRange.layerCount     = 1;

	api->CmdPipelineBarrier(                           //
		command_buffer,                                //
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, //
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, //
		0,                                             //
		0,                                             //
		NULL,                                          //
		0,                                             //
		NULL,                                          //
		1,                                             //
		&imageBarrier                                  //
	);

	return (sht_command_buffer){.owner = api, .handle = (sht_handle *)command_buffer};
}

static sht_bool32 sht_command_buffer_is_ok(sht_command_buffer command)
{
	return command.handle != NULL && command.owner != NULL;
}

static void sht_command_buffer_scissor(sht_command_buffer command, sht_scissor *scissor)
{
	sht_vk_command *api            = (sht_vk_command *)command.owner;
	VkCommandBuffer command_buffer = (VkCommandBuffer)command.handle;
	const VkRect2D  rect           = (VkRect2D){
				   .offset.x      = (fckc_u32)scissor->offset.x,
				   .offset.y      = (fckc_u32)scissor->offset.y,
				   .extent.width  = (fckc_u32)scissor->extent.width,
				   .extent.height = (fckc_u32)scissor->extent.height,
    };
	api->CmdSetScissor(command_buffer, 0, 1, &rect);
}

static void sht_command_buffer_viewport(sht_command_buffer command, sht_viewport *viewport)
{
	sht_vk_command  *api            = (sht_vk_command *)command.owner;
	VkCommandBuffer  command_buffer = (VkCommandBuffer)command.handle;
	const VkViewport vp             = (VkViewport){
					.x        = viewport->offset.x,
					.y        = viewport->offset.y,
					.width    = viewport->extent.width,
					.height   = viewport->extent.height,
					.minDepth = viewport->depth.min,
					.maxDepth = viewport->depth.max,
    };
	api->CmdSetViewport(command_buffer, 0, 1, &vp);
}

static void sht_command_buffer_vertex_buffer(sht_command_buffer command, sht_buffer *vertex_buffer, fckc_u64 offset)
{
	sht_vk_command *api            = (sht_vk_command *)command.owner;
	VkCommandBuffer command_buffer = (VkCommandBuffer)command.handle;
	VkBuffer        buffer         = (VkBuffer)vertex_buffer->gpu;
	api->CmdBindVertexBuffers(command_buffer, 0, 1, &buffer, &(VkDeviceSize){offset});
}
static void sht_command_buffer_index_buffer(sht_command_buffer command, sht_buffer *index_buffer, fckc_u64 offset)
{
	sht_vk_command *api            = (sht_vk_command *)command.owner;
	VkCommandBuffer command_buffer = (VkCommandBuffer)command.handle;
	api->CmdBindIndexBuffer(command_buffer, index_buffer->gpu, offset, VK_INDEX_TYPE_UINT32);
}
static void sht_command_buffer_draw_indexed(sht_command_buffer command, const sht_draw_indexed_desc *params)
{
	sht_vk_command *api            = (sht_vk_command *)command.owner;
	VkCommandBuffer command_buffer = (VkCommandBuffer)command.handle;
	const fckc_i32  vertex_offset  = params->vertex_offset;
	const fckc_u32  first_index    = params->first_index;
	const fckc_u32  index_count    = params->index_count;
	const fckc_u32  first_instance = params->first_instance;
	const fckc_u32  instance_count = params->instance_count;

	if (index_count == 0 || instance_count == 0)
	{
		return;
	}

	api->CmdDrawIndexed(command_buffer, index_count, instance_count, first_index, vertex_offset, first_instance);
}

static fckc_size_t sht_vk_command_find(sht_vk_command *command, sht_command_buffer command_buffer)
{
	sht_vk_assert(command == command_buffer.owner);

	for (fckc_size_t index = 0; index < sht_frame_count; index++)
	{
		VkCommandBuffer current = command->buffers[index];
		if (current == command_buffer.handle)
		{
			return index + 1;
		}
	}
	return 0;
}

static void sht_swapchain_present(sht_command_buffer command_buffer, VkSemaphore *graphics_completed, fckc_u32 image_index)
{
	sht_vk_command *api = (sht_vk_command *)command_buffer.owner;

	sht_vk_swapchain             *swapchain = &api->driver->swapchain;
	sht_vk_common_sync_resources *sync      = &swapchain->sync;
	sht_vk_driver                *driver    = api->driver;

	VkPresentInfoKHR present_info   = {0};
	present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores    = graphics_completed;
	present_info.swapchainCount     = 1;
	present_info.pSwapchains        = &swapchain->swapchain;
	present_info.pImageIndices      = &image_index;

	sht_vk_queues *queues        = &driver->gpu->queues;
	VkQueue        present_queue = sht_vk_queues_get_primary(queues, driver->device, sht_queue_present);
	const VkResult result        = queues->QueuePresentKHR(present_queue, &present_info);
	(void)result; // We discard it, since next time we try to get a new swapchain image, we ask for the same shit
	sync->index = (sync->index + 1) % sht_frame_count; // Idk if we should skip counting lol
}

static void sht_command_buffer_submit(sht_command_buffer command, sht_queue_type queue_type)
{
	sht_vk_command *api    = (sht_vk_command *)command.owner;
	sht_vk_driver  *driver = api->driver;

	sht_vk_common_sync_resources *sync           = &driver->swapchain.sync;
	VkCommandBuffer               command_buffer = (VkCommandBuffer)command.handle;

	// Pipeline stage at which the queue submission will wait (via pWaitSemaphores)
	const VkPipelineStageFlags wait_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	// The submit info structure specifies a command buffer queue submission batch
	VkSubmitInfo               submit_info     = {0};
	switch (queue_type)
	{
	case sht_queue_graphic: {
		// Present
		const fckc_u32 image_index   = sync->frame_index_to_swapchain_image_index[sync->index];
		sht_vk_queues *queues        = &driver->gpu->queues;
		VkFence       *wait_fence    = &sync->wait_fences[sync->index];
		VkQueue        graphic_queue = sht_vk_queues_get_primary(queues, driver->device, queue_type);

		{
			sht_vk_swapchain *swapchain = &driver->swapchain;

			const VkPipelineStageFlags source_stages      = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			const VkPipelineStageFlags destination_stages = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			VkImageMemoryBarrier       memory_barrier;
			memset(&memory_barrier, 0, sizeof(memory_barrier));
			memory_barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			memory_barrier.pNext                           = NULL;
			memory_barrier.srcAccessMask                   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			memory_barrier.oldLayout                       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			memory_barrier.dstAccessMask                   = 0;
			memory_barrier.newLayout                       = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			memory_barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
			memory_barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
			memory_barrier.image                           = swapchain->images[image_index];
			memory_barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
			memory_barrier.subresourceRange.baseArrayLayer = 0;
			memory_barrier.subresourceRange.layerCount     = 1;
			memory_barrier.subresourceRange.baseMipLevel   = 0;
			memory_barrier.subresourceRange.levelCount     = 1;

			api->CmdPipelineBarrier(command_buffer, source_stages, destination_stages, 0, 0, NULL, 0, NULL, 1, &memory_barrier);
		}

		submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.pWaitDstStageMask  = &wait_stage_mask; // Pointer to the list of pipeline stages that the semaphore waits will occur at
		submit_info.pCommandBuffers    = &command_buffer;  // Command buffers(s) to execute in this batch (submission)
		submit_info.commandBufferCount = 1;                // We submit a single command buffer

		// fckc_size_t find = sht_vk_command_find(api, command);
		// sht_vk_assert(find);
		// fckc_u32 frame_index = find - 1;
		sync->frame_index_to_swapchain_image_index[sync->index] = image_index | (1 << 31);
		sht_vk_assert(image_index < sht_vk_swapchain_image_capacity);

		api->EndCommandBuffer(command_buffer);

		VkSemaphore *graphic_completed = &sync->graphics_completed[sync->index];

		submit_info.pWaitSemaphores      = &sync->presentation_completed[sync->index];
		submit_info.waitSemaphoreCount   = 1;
		submit_info.pSignalSemaphores    = &sync->graphics_completed[sync->index];
		submit_info.signalSemaphoreCount = 1;
		sht_vk_crash(queues->QueueSubmit(graphic_queue, 1, &submit_info, *wait_fence));
		sht_swapchain_present(command, graphic_completed, image_index);
		break;
	}
	case sht_queue_transfer: {
		api->EndCommandBuffer(command_buffer);

		submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers    = &command_buffer;

		VkFenceCreateInfo fence_create_info = {0};
		fence_create_info.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fence_create_info.flags             = 0;
		VkFence fence;
		sht_vk_crash(driver->CreateFence(driver->device, &fence_create_info, default_allocation_callbacks, &fence));

		sht_vk_queues *queues = &driver->gpu->queues;
		VkQueue        queue  = sht_vk_queues_get_primary(queues, driver->device, queue_type);

		sht_vk_crash(queues->QueueSubmit(queue, 1, &submit_info, fence));
		// TODO: Barriers might be better! Let's see and do all that later!
		sht_vk_crash(driver->WaitForFences(driver->device, 1, &fence, VK_TRUE, ~0llu));
		driver->DestroyFence(driver->device, fence, default_allocation_callbacks);
		break;
	}
	case sht_queue_present: {
		sht_vk_assert(sht_false && "Present not supported for now!");
		return;
	}
	case sht_queue_compute:
		sht_vk_assert(sht_false && "Compute not supported for now!");
		return;
	case sht_queue_count:
		break;
	}
}

static void sht_driver_copy_buffer(sht_driver driver, sht_buffer *dst, sht_buffer *src)
{
	// sht_vk_command *api = (sht_vk_command *)command.owner;
	// VkCommandBuffer command_buffer = (VkCommandBuffer)command.handle;
	sht_vk_driver *vk_driver = sht_driver_to_vk(driver);
	if (src->cpu != NULL && dst->cpu != NULL)
	{
		// Fast path if both buffers use CPU visible memory!
		memcpy(dst->cpu, src->cpu, src->size);
		return;
	}

	sht_command_buffer command        = sht_command_buffer_create(driver);
	VkCommandBuffer    command_buffer = (VkCommandBuffer)command.handle;

	VkBufferCopy copy_region;
	copy_region.srcOffset = 0;
	copy_region.dstOffset = 0;
	copy_region.size      = fck_min(src->size, dst->size);

	vk_driver->command.CmdCopyBuffer(command_buffer, src->gpu, dst->gpu, 1, &copy_region);
	sht_command_buffer_submit(command, sht_queue_transfer);

	sht_command_buffer_destroy(&command);
}

static void sht_driver_upload_image(sht_driver driver, sht_image *dst, const void *src, fckc_size_t size)
{
	sht_vk_driver *vk_driver = sht_driver_to_vk(driver);

	sht_command_buffer command        = sht_command_buffer_create(driver);
	VkCommandBuffer    command_buffer = (VkCommandBuffer)command.handle;

	VkBufferImageCopy copy_region = {.imageExtent = {dst->width, dst->height, 1}, .imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1}};

	sht_buffer staging = vk_driver->memory.malloc(vk_driver->memory.temp, &(sht_buffer_source(0, size)), sht_memory_cpu);
	memcpy(staging.cpu, src, size);

	{
		VkImageMemoryBarrier barrier            = {0};
		barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image                           = dst->gpu;
		barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel   = 0;
		barrier.subresourceRange.levelCount     = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount     = 1;
		barrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.srcAccessMask                   = 0;
		barrier.dstAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
		vk_driver->command.CmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL,
		                                      0, NULL, 1, &barrier);
	}

	vk_driver->command.CmdCopyBufferToImage(command_buffer, staging.gpu, dst->gpu, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

	{
		VkImageMemoryBarrier barrier            = {0};
		barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image                           = dst->gpu;
		barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel   = 0;
		barrier.subresourceRange.levelCount     = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount     = 1;
		barrier.oldLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout                       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask                   = VK_ACCESS_SHADER_READ_BIT;
		barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
		vk_driver->command.CmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0,
		                                      NULL, 0, NULL, 1, &barrier);
	}

	sht_command_buffer_submit(command, sht_queue_transfer);

	vk_driver->memory.free(vk_driver->memory.temp, &staging);
	sht_command_buffer_destroy(&command);
}

static void sht_driver_upload_buffer(sht_driver driver, sht_buffer *dst, const void *src, fckc_size_t size)
{
	sht_vk_driver *vk_driver = sht_driver_to_vk(driver);
	if (dst->cpu != NULL)
	{
		// Fast path if buffer uses CPU visible memory!
		memcpy(dst->cpu, src, size);
		return;
	}

	sht_buffer staging = vk_driver->memory.malloc(vk_driver->memory.temp, &(sht_buffer_source(0, size)), sht_memory_cpu);
	memcpy(staging.cpu, src, size);
	sht_driver_copy_buffer(driver, dst, &staging);
	vk_driver->memory.free(vk_driver->memory.temp, &staging);
}

static sht_bool32 sht_image_view_equals(const sht_image_view *lhs, const sht_image_view *rhs)
{
	return lhs->format == rhs->format && lhs->gpu == rhs->gpu;
}

static sht_bool32 sht_depth_target_desc_equals(const sht_depth_target_desc *lhs, const sht_depth_target_desc *rhs)
{
	if (!sht_image_view_equals(&lhs->view, &rhs->view))
	{
		return sht_false;
	}
	if (lhs->clear_value != rhs->clear_value)
	{
		return sht_false;
	}
	if (lhs->load_op != rhs->load_op)
	{
		return sht_false;
	}
	if (lhs->store_op != rhs->store_op)
	{
		return sht_false;
	}
	return sht_true;
}

static sht_bool32 sht_color_target_desc_equals(const sht_color_target_desc *lhs, const sht_color_target_desc *rhs)
{
	if (!sht_image_view_equals(&lhs->view, &rhs->view))
	{
		return sht_false;
	}
	if (lhs->clear_value[0] != rhs->clear_value[0] || lhs->clear_value[1] != rhs->clear_value[1] ||
	    lhs->clear_value[2] != rhs->clear_value[2] || lhs->clear_value[3] != rhs->clear_value[3])
	{
		return sht_false;
	}
	if (lhs->load_op != rhs->load_op)
	{
		return sht_false;
	}
	if (lhs->store_op != rhs->store_op)
	{
		return sht_false;
	}
	return sht_true;
}

static sht_bool32 sht_vk_depth_target_desc_equals(const sht_vk_depth_target_desc *lhs, const sht_vk_depth_target_desc *rhs)
{
	if (lhs->format != rhs->format)
	{
		return sht_false;
	}
	// if (lhs->clear_value != rhs->clear_value)
	//{
	//	return sht_false;
	// }
	if (lhs->load_op != rhs->load_op)
	{
		return sht_false;
	}
	if (lhs->store_op != rhs->store_op)
	{
		return sht_false;
	}
	return sht_true;
}

static sht_bool32 sht_vk_color_target_desc_equals(const sht_vk_color_target_desc *lhs, const sht_vk_color_target_desc *rhs)
{
	if (lhs->format != rhs->format)
	{
		return sht_false;
	}
	// if (lhs->clear_value[0] != rhs->clear_value[0] || lhs->clear_value[1] != rhs->clear_value[1] ||
	//     lhs->clear_value[2] != rhs->clear_value[2] || lhs->clear_value[3] != rhs->clear_value[3])
	//{
	//	return sht_false;
	// }
	if (lhs->load_op != rhs->load_op)
	{
		return sht_false;
	}
	if (lhs->store_op != rhs->store_op)
	{
		return sht_false;
	}
	return sht_true;
}

static sht_bool32 sht_vk_render_pass_desc_equals(const sht_vk_render_pass_desc *lhs, const sht_vk_render_pass_desc *rhs)
{
	if (!sht_vk_color_target_desc_equals(&lhs->colour, &rhs->colour))
	{
		return sht_false;
	}
	if (!sht_vk_depth_target_desc_equals(&lhs->depth, &rhs->depth))
	{
		return sht_false;
	}
	return sht_true;
}

static sht_bool32 sht_render_desc_equals(const sht_render_desc *lhs, const sht_render_desc *rhs)
{
	if (!sht_color_target_desc_equals(&lhs->colour, &rhs->colour))
	{
		return sht_false;
	}
	if (!sht_depth_target_desc_equals(&lhs->depth, &rhs->depth))
	{
		return sht_false;
	}
	return sht_true;
}

static fckc_size_t sht_vk_render_pass_storage_find(sht_vk_render_pass_storage *storage, const sht_vk_render_pass_desc *desc)
{
	for (fckc_size_t index = 0; index < storage->count; index++)
	{
		const sht_vk_render_pass *render_pass = storage->handles + index;
		if (sht_vk_render_pass_desc_equals(desc, &render_pass->desc))
		{
			return index + 1;
		}
	}
	return 0;
}

static fckc_size_t sht_vk_framebuffers_storage_find(sht_vk_framebuffer_storage *storage, const sht_render_desc *desc)
{
	for (fckc_size_t index = 0; index < storage->count; index++)
	{
		sht_vk_framebuffer *framebuffer = storage->handles + index;
		if (sht_render_desc_equals(desc, &framebuffer->desc))
		{
			return index + 1;
		}
	}
	return 0;
}

static sht_bool32 sht_image_view_is_from_swapchain(sht_vk_swapchain *swapchain, const sht_color_target_desc *color_target)
{
	for (fckc_size_t index = 0; index < swapchain->count; index++)
	{
		VkImageView vk_view = swapchain->views[index];
		if (vk_view == color_target->view.gpu)
		{
			return sht_true;
		}
	}
	return sht_false;
}

static VkResult sht_vk_render_pass_create(sht_vk_driver *driver, const sht_vk_render_pass_desc *desc, VkRenderPass *render_pass)
{
	VkAttachmentDescription attachments[2];
	VkAttachmentReference   references[2];
	VkSubpassDependency     dependencies[2];

	fckc_size_t dependency_count  = 0;
	fckc_size_t attachments_count = 0;
	// fckc_size_t colour_count = 0;

	// Setup a single subpass reference
	VkSubpassDescription subpass_desc    = {0};
	subpass_desc.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass_desc.inputAttachmentCount    = 0;
	subpass_desc.pInputAttachments       = NULL;
	subpass_desc.preserveAttachmentCount = 0;
	subpass_desc.pPreserveAttachments    = NULL;
	subpass_desc.pResolveAttachments     = NULL;

	if (desc->colour.format != sht_format_undefined)
	{
		VkAttachmentReference *reference = &references[attachments_count];
		memset(reference, 0, sizeof(*reference));
		reference->attachment = attachments_count;
		reference->layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription *attachment = &attachments[attachments_count++];
		memset(attachment, 0, sizeof(*attachment));
		attachment->format         = sht_vk_format_from_sht_format(desc->colour.format); // TODO: Make clean translation
		attachment->samples        = VK_SAMPLE_COUNT_1_BIT;                              // TODO: Make clean translation
		attachment->loadOp         = sht_vk_load_op_from_op(desc->colour.load_op);
		attachment->storeOp        = sht_vk_store_op_from_op(desc->colour.store_op);
		attachment->stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		// TODO: we technically have to do this for finalLayout too
		attachment->initialLayout =
			desc->colour.load_op == sht_clear ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachment->finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDependency *dependency = &dependencies[dependency_count++];
		memset(dependency, 0, sizeof(*dependency));
		dependency->srcSubpass      = VK_SUBPASS_EXTERNAL;
		dependency->dstSubpass      = 0;
		dependency->srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency->dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency->srcAccessMask   = 0;
		dependency->dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
		dependency->dependencyFlags = 0;

		subpass_desc.colorAttachmentCount = 1;
		subpass_desc.pColorAttachments    = references;
	}

	if (desc->depth.format != sht_format_undefined)
	{
		VkAttachmentReference *reference = &references[attachments_count];
		memset(reference, 0, sizeof(*reference));
		reference->attachment = attachments_count;
		reference->layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription *attachment = &attachments[attachments_count++];
		memset(attachment, 0, sizeof(*attachment));
		attachment->format         = sht_vk_format_from_sht_format(desc->depth.format); // TODO: Make clean translation
		attachment->samples        = VK_SAMPLE_COUNT_1_BIT;
		attachment->loadOp         = sht_vk_load_op_from_op(desc->depth.load_op);
		attachment->storeOp        = sht_vk_store_op_from_op(desc->depth.store_op);
		attachment->stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment->initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
		attachment->finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDependency *dependency = &dependencies[dependency_count++];
		memset(dependency, 0, sizeof(*dependency));
		dependency->srcSubpass      = VK_SUBPASS_EXTERNAL;
		dependency->dstSubpass      = 0;
		dependency->srcStageMask    = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependency->dstStageMask    = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependency->srcAccessMask   = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependency->dstAccessMask   = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		dependency->dependencyFlags = 0;

		subpass_desc.pDepthStencilAttachment = reference;
	}

	// Create the actual renderpass
	VkRenderPassCreateInfo render_pass_create_info = {0};
	render_pass_create_info.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_create_info.attachmentCount        = attachments_count; // Number of attachments used by this render pass
	render_pass_create_info.pAttachments           = attachments;       // Descriptions of the attachments used by the render pass
	render_pass_create_info.subpassCount           = 1;                 // We only use one subpass in this example
	render_pass_create_info.pSubpasses             = &subpass_desc;     // Description of that subpass
	render_pass_create_info.dependencyCount        = dependency_count;  // Number of subpass dependencies
	render_pass_create_info.pDependencies          = dependencies;      // Subpass dependencies used by the render pass

	return sht_vk_error(driver->CreateRenderPass(driver->device, &render_pass_create_info, default_allocation_callbacks, render_pass));
}

typedef enum sht_standard_stage
{
	SHT_STANDARD_STAGE_VERTEX,
	SHT_STANDARD_STAGE_FRAGMENT,
	SHT_STANDARD_STAGE_COUNT,
} sht_standard_stage;

typedef struct sht_wip_stage
{
	int         type;
	sht_handle *shader;
	sht_buffer  state;
} sht_wip_stage;

typedef struct sht_wip
{
	VkPipeline pipeline;
} sht_wip;

static VkResult sht_vk_shader_module_load(sht_vk_driver *driver, fck_shader_desc desc, const char *path, VkShaderModule *shader)
{
	fck_shader_api     *shader_api = (fck_shader_api *)apis->find(fck_shader_api_name);
	fck_shader_compiler compiler   = shader_api->create();

	const fck_file shader_source = os->fs->open(path, "rb");

	const fckc_size_t size = os->fs->size(shader_source);
	char             *text = (char *)kll_malloc(kll->system, size);
	os->fs->read(shader_source, text, size);

	fck_hlsl_object  hlsl  = compiler.create_hlsl(&compiler, &desc, text);
	fck_spirv_object spirv = compiler.create_spirv(&compiler, &hlsl.generic);

	const VkShaderModuleCreateInfo shader_info = (VkShaderModuleCreateInfo){
		.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = spirv.generic.souce_byte_size,
		.pCode    = (const fckc_u32 *)spirv.generic.source,
	};

	const VkResult result = sht_vk_error(driver->CreateShaderModule(driver->device, &shader_info, default_allocation_callbacks, shader));

	compiler.destroy(&compiler, &hlsl.generic);
	compiler.destroy(&compiler, &spirv.generic);
	compiler.shutdown(&compiler);
	return result;
}

static VkResult sht_vk_shader_module_create(fck_shader_compiler *compiler, sht_vk_driver *driver, fck_shader_generic *generic,
                                            VkShaderModule *shader)
{
	VkResult result;
	if (compiler->language(generic) != fck_shader_spirv)
	{
		fck_spirv_object spirv = compiler->create_spirv(compiler, generic);

		const VkShaderModuleCreateInfo shader_info = (VkShaderModuleCreateInfo){
			.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = compiler->size(&spirv.generic),
			.pCode    = (const fckc_u32 *)compiler->source(&spirv.generic),
		};
		result = sht_vk_error(driver->CreateShaderModule(driver->device, &shader_info, default_allocation_callbacks, shader));
		compiler->destroy(compiler, &spirv.generic);
	}
	else
	{
		const VkShaderModuleCreateInfo shader_info = (VkShaderModuleCreateInfo){
			.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = compiler->size(generic),
			.pCode    = (const fckc_u32 *)compiler->source(generic),
		};
		result = sht_vk_error(driver->CreateShaderModule(driver->device, &shader_info, default_allocation_callbacks, shader));
	}
	return result;
}

static sht_vk_render_pass_desc sht_vk_render_pass_desc_from_render_desc(const sht_render_desc *desc)
{
	sht_vk_render_pass_desc vk = {0};
	vk.colour.format           = desc->colour.view.format;
	vk.colour.load_op          = desc->colour.load_op;
	vk.colour.store_op         = desc->colour.store_op;
	// vk.colour.clear_value[0] = desc->colour.clear_value[0];
	//  vk.colour.clear_value[1] = desc->colour.clear_value[1];
	//  vk.colour.clear_value[2] = desc->colour.clear_value[2];
	//  vk.colour.clear_value[3] = desc->colour.clear_value[3];

	vk.depth.format   = desc->depth.view.format;
	vk.depth.load_op  = desc->depth.load_op;
	vk.depth.store_op = desc->depth.store_op;
	// vk.depth.clear_value = desc->depth.clear_value;
	return vk;
}

static VkPipelineInputAssemblyStateCreateInfo sht_vk_input_assembly_state()
{
	VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info;
	input_assembly_create_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_create_info.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly_create_info.pNext                  = NULL;
	input_assembly_create_info.flags                  = 0;
	input_assembly_create_info.primitiveRestartEnable = VK_FALSE;
	return input_assembly_create_info;
}

static VkPipelineRasterizationStateCreateInfo sht_vk_raster_state()
{
	VkPipelineRasterizationStateCreateInfo rasterisation_state_create_info;
	rasterisation_state_create_info.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterisation_state_create_info.polygonMode             = VK_POLYGON_MODE_FILL;
	rasterisation_state_create_info.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterisation_state_create_info.cullMode                = VK_CULL_MODE_NONE;
	rasterisation_state_create_info.lineWidth               = 1.0f;
	rasterisation_state_create_info.depthBiasEnable         = VK_FALSE;
	rasterisation_state_create_info.depthBiasSlopeFactor    = 0.0f;
	rasterisation_state_create_info.depthBiasConstantFactor = 0.0f;
	rasterisation_state_create_info.depthBiasClamp          = 0.0f;
	rasterisation_state_create_info.pNext                   = NULL;
	rasterisation_state_create_info.flags                   = 0;
	rasterisation_state_create_info.depthClampEnable        = VK_FALSE;
	rasterisation_state_create_info.rasterizerDiscardEnable = VK_FALSE;
	return rasterisation_state_create_info;
}

static VkPipelineColorBlendAttachmentState sht_vk_color_blend_attachment_state()
{
	VkPipelineColorBlendAttachmentState blend_attachment_state;
	const int color_mask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	blend_attachment_state.colorWriteMask      = color_mask;
	blend_attachment_state.blendEnable         = VK_TRUE;
	blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blend_attachment_state.colorBlendOp        = VK_BLEND_OP_ADD;
	blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	blend_attachment_state.alphaBlendOp        = VK_BLEND_OP_ADD;
	return blend_attachment_state;
}

static VkPipelineColorBlendStateCreateInfo sht_vk_color_blend_state(VkPipelineColorBlendAttachmentState *blend_attachment_states,
                                                                    fckc_size_t                          count)
{
	VkPipelineColorBlendStateCreateInfo color_blend_state_create_info;
	color_blend_state_create_info.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blend_state_create_info.pNext           = NULL;
	color_blend_state_create_info.attachmentCount = count;
	color_blend_state_create_info.pAttachments    = blend_attachment_states;
	color_blend_state_create_info.flags           = 0;
	color_blend_state_create_info.logicOpEnable   = VK_FALSE;
	color_blend_state_create_info.logicOp         = VK_LOGIC_OP_CLEAR;
	// Yes, this is ugly lol
	memset(color_blend_state_create_info.blendConstants, 0, sizeof(color_blend_state_create_info.blendConstants));
	return color_blend_state_create_info;
}
static VkPipelineViewportStateCreateInfo sht_vk_viewport_state()
{
	VkPipelineViewportStateCreateInfo viewport_state_create_info;
	viewport_state_create_info.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state_create_info.pNext         = NULL;
	viewport_state_create_info.viewportCount = 1;
	viewport_state_create_info.scissorCount  = 1;
	viewport_state_create_info.flags         = 0;
	viewport_state_create_info.pViewports    = NULL;
	viewport_state_create_info.pScissors     = NULL;
	return viewport_state_create_info;
}

static VkPipelineDynamicStateCreateInfo sht_vk_dynamic_state(VkDynamicState *dynamic_states, fckc_size_t count)
{
	VkPipelineDynamicStateCreateInfo dynamic_state_create_info;
	dynamic_state_create_info.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state_create_info.pNext             = NULL;
	dynamic_state_create_info.flags             = 0;
	dynamic_state_create_info.pDynamicStates    = dynamic_states;
	dynamic_state_create_info.dynamicStateCount = count;
	return dynamic_state_create_info;
}

static VkPipelineDepthStencilStateCreateInfo sht_vk_depth_stencil_state()
{
	VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info;
	depth_stencil_create_info.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil_create_info.pNext                 = NULL;
	depth_stencil_create_info.flags                 = 0;
	depth_stencil_create_info.depthTestEnable       = VK_TRUE;
	depth_stencil_create_info.depthWriteEnable      = VK_TRUE;
	depth_stencil_create_info.depthCompareOp        = VK_COMPARE_OP_GREATER_OR_EQUAL;
	// Depth bounds are off
	depth_stencil_create_info.depthBoundsTestEnable = VK_FALSE;
	depth_stencil_create_info.minDepthBounds        = 0.0f;
	depth_stencil_create_info.maxDepthBounds        = 0.0f;
	// Stencil is off
	depth_stencil_create_info.stencilTestEnable     = VK_FALSE;
	depth_stencil_create_info.back.failOp           = VK_STENCIL_OP_KEEP;
	depth_stencil_create_info.back.passOp           = VK_STENCIL_OP_KEEP;
	depth_stencil_create_info.back.compareOp        = VK_COMPARE_OP_ALWAYS;
	depth_stencil_create_info.back.depthFailOp      = VK_STENCIL_OP_ZERO;
	depth_stencil_create_info.back.compareMask      = 0;
	depth_stencil_create_info.back.writeMask        = 0;
	depth_stencil_create_info.back.reference        = 0;
	depth_stencil_create_info.front                 = depth_stencil_create_info.back;
	return depth_stencil_create_info;
}

static VkPipelineMultisampleStateCreateInfo sht_vk_multisampling_state()
{
	VkPipelineMultisampleStateCreateInfo multisample_state_create_info;
	multisample_state_create_info.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample_state_create_info.pNext                 = NULL;
	multisample_state_create_info.flags                 = 0;
	multisample_state_create_info.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
	multisample_state_create_info.pSampleMask           = NULL;
	multisample_state_create_info.sampleShadingEnable   = VK_FALSE;
	multisample_state_create_info.minSampleShading      = 0.0f;
	multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
	multisample_state_create_info.alphaToOneEnable      = VK_FALSE;
	return multisample_state_create_info;
}

static VkVertexInputBindingDescription sht_vk_vertex_input_binding_desc(fckc_size_t stride)
{
	VkVertexInputBindingDescription vertex_input_binding;
	vertex_input_binding.binding   = 0;
	vertex_input_binding.stride    = stride;
	vertex_input_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return vertex_input_binding;
}

static VkPipelineVertexInputStateCreateInfo sht_vk_vertex_input_state(VkVertexInputBindingDescription   *binding,
                                                                      VkVertexInputAttributeDescription *attributes, fckc_size_t count)
{
	VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info;
	vertex_input_state_create_info.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_state_create_info.pNext                           = NULL;
	vertex_input_state_create_info.flags                           = 0;
	vertex_input_state_create_info.vertexBindingDescriptionCount   = 1;
	vertex_input_state_create_info.pVertexBindingDescriptions      = binding;
	vertex_input_state_create_info.vertexAttributeDescriptionCount = count;
	vertex_input_state_create_info.pVertexAttributeDescriptions    = attributes;
	return vertex_input_state_create_info;
}

static VkResult sht_vk_graphics_pipeline_create(sht_vk_driver *driver, VkRenderPass render_pass, VkDescriptorSetLayout *set_layouts,
                                                VkPipelineLayout pipeline_layout, fckc_size_t count,
                                                VkPipelineShaderStageCreateInfo *shader_create_infos, fckc_size_t shader_count,
                                                sht_vertex_desc *vertex_desc, sht_vk_graphics_pipeline *graphics_pipeline)
{
	// Create the graphics pipeline used in this example
	// Vulkan uses the concept of rendering pipelines to encapsulate fixed states, replacing OpenGL's complex state machine
	// A pipeline is then stored and hashed on the GPU making pipeline changes very fast
	// Note: There are still a few dynamic states that are not directly part of the pipeline (but the info that they are used is)

	// Construct the different states making up the pipeline

	// Input assembly state describes how primitives are assembled
	// This pipeline will assemble vertex data as a triangle lists (though we only use one triangle)
	const VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info = sht_vk_input_assembly_state();

	// Rasterization state
	const VkPipelineRasterizationStateCreateInfo rasterisation_state_create_info = sht_vk_raster_state();

	// Color blend state describes how blend factors are calculated (if used)
	// We need one blend attachment state per color attachment (even if blending is not used)
	VkPipelineColorBlendAttachmentState       blend_attachment_state        = sht_vk_color_blend_attachment_state();
	const VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = sht_vk_color_blend_state(&blend_attachment_state, 1);

	// Viewport state sets the number of viewports and scissor used in this pipeline
	// Note: This is actually overridden by the dynamic states (see below)
	const VkPipelineViewportStateCreateInfo viewport_state_create_info = sht_vk_viewport_state();

	// Enable dynamic states
	// Most states are baked into the pipeline, but there are still a few dynamic states that can be changed within a command buffer
	// To be able to change these we need do specify which dynamic states will be changed using this pipeline. Their actual states are set
	// later on in the command buffer. For this example we will set the viewport and scissor using dynamic states
	VkDynamicState                         dynamic_states[]          = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	const VkPipelineDynamicStateCreateInfo dynamic_state_create_info = sht_vk_dynamic_state(dynamic_states, fck_arraysize(dynamic_states));

	// Depth and stencil state containing depth and stencil compare and test operations
	// We only use depth tests and want depth tests and writes to be enabled and compare with less or equal
	const VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info = sht_vk_depth_stencil_state();

	// Multi sampling state
	// This example does not make use of multi sampling (for anti-aliasing), the state must still be set and passed to the pipeline
	const VkPipelineMultisampleStateCreateInfo multisample_state_create_info = sht_vk_multisampling_state();

	// Vertex input descriptions
	// Specifies the vertex input parameters for a pipeline

	// Vertex input binding
	// This example uses a single vertex input binding at binding point 0 (see vkCmdBindVertexBuffers)
	VkVertexInputBindingDescription binding = sht_vk_vertex_input_binding_desc(vertex_desc->stride);

	// Input attribute bindings describe shader attribute locations and memory layouts
	VkVertexInputAttributeDescription attributes[8];
	sht_vk_assert(fck_arraysize(attributes) >= vertex_desc->count);

	for (fckc_size_t index = 0; index < vertex_desc->count; index++)
	{
		const sht_vertex_binding          *binding   = vertex_desc->bindings + index;
		VkVertexInputAttributeDescription *attribute = attributes + index;
		attribute->binding                           = 0;
		attribute->location                          = binding->location;
		attribute->offset                            = binding->offset;
		attribute->format                            = sht_vk_format_from_sht_format(binding->format);
	}

	// These match the following shader layout (see triangle.vert):
	//	layout (location = 0) in vec3 inPos;
	//	layout (location = 1) in vec3 inColor;
	// Attribute location 0: Position
	// Position attribute is three 32 bit signed (SFLOAT) floats (R32 G32 B32)
	// attributes[0].binding = 0;
	// attributes[0].location = 0;
	// attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	// attributes[0].offset = offsetof(sht_standard_vertex, position);
	//// Attribute location 1: Color
	//// Color attribute is three 32 bit signed (SFLOAT) floats (R32 G32 B32)
	// attributes[1].binding = 0;
	// attributes[1].location = 1;
	// attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	// attributes[1].offset = offsetof(sht_standard_vertex, color);
	// Vertex input state used for pipeline creation
	const VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info =
		sht_vk_vertex_input_state(&binding, attributes, vertex_desc->count);

	VkGraphicsPipelineCreateInfo pipeline_create_info;
	pipeline_create_info.sType              = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_create_info.pNext              = NULL;
	pipeline_create_info.flags              = 0;
	// The layout used for this pipeline (can be shared among multiple pipelines using the same layout)
	pipeline_create_info.layout             = pipeline_layout;
	// Renderpass this pipeline is attached to
	pipeline_create_info.renderPass         = render_pass;
	pipeline_create_info.subpass            = 0;
	pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
	pipeline_create_info.basePipelineIndex  = 0;
	// Shaders
	// Set pipeline shader stage info
	pipeline_create_info.stageCount         = shader_count;
	pipeline_create_info.pStages            = shader_create_infos;

	// Assign the pipeline states to the pipeline creation info structure
	pipeline_create_info.pVertexInputState   = &vertex_input_state_create_info;
	pipeline_create_info.pInputAssemblyState = &input_assembly_create_info;
	pipeline_create_info.pRasterizationState = &rasterisation_state_create_info;
	pipeline_create_info.pColorBlendState    = &color_blend_state_create_info;
	pipeline_create_info.pMultisampleState   = &multisample_state_create_info;
	pipeline_create_info.pViewportState      = &viewport_state_create_info;
	pipeline_create_info.pDepthStencilState  = &depth_stencil_create_info;
	pipeline_create_info.pDynamicState       = &dynamic_state_create_info;

	// Create rendering pipeline using the specified states

	VkPipelineCache pipeline_cache;

	VkPipelineCacheCreateInfo pipeline_cache_create_info;
	pipeline_cache_create_info.sType           = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	pipeline_cache_create_info.pNext           = NULL;
	pipeline_cache_create_info.flags           = 0;
	pipeline_cache_create_info.initialDataSize = 0;
	pipeline_cache_create_info.pInitialData    = NULL;

	sht_vk_crash(driver->CreatePipelineCache(driver->device, &pipeline_cache_create_info, default_allocation_callbacks, &pipeline_cache));

	VkPipeline pipeline;
	sht_vk_crash(
		driver->CreateGraphicsPipelines(driver->device, pipeline_cache, 1, &pipeline_create_info, default_allocation_callbacks, &pipeline));

	graphics_pipeline->cache    = pipeline_cache;
	graphics_pipeline->pipeline = pipeline;
	return VK_SUCCESS;
}

static VkResult sht_vk_framebuffer_create(sht_vk_driver *driver, VkSurfaceKHR surface, const sht_vk_render_pass *render_pass,
                                          const sht_render_desc *desc, sht_vk_framebuffer *framebuffer)
{
	fckc_size_t attachment_count = 0;

	VkImageView attachments[8];
	if (desc->colour.view.format != VK_FORMAT_UNDEFINED)
	{
		attachments[attachment_count++] = (VkImageView)desc->colour.view.gpu;
	}
	if (desc->depth.view.format != VK_FORMAT_UNDEFINED)
	{
		attachments[attachment_count++] = (VkImageView)desc->depth.view.gpu;
	}

	// VkImage swapchain_images[FCK_VK_IMAGE_COUNT];
	//  swapchain_image_view_count = fck_arraysize(swapchain_images);
	//  In any case, we can query them, in other cases we have views.
	VkExtent2D     extent;
	const VkResult result = sht_vk_error(sht_vk_surface_size(driver->gpu, surface, &extent));
	if (result != VK_SUCCESS)
	{
		return result;
	}

	VkFramebufferCreateInfo fb_info = {0};
	fb_info.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fb_info.pNext                   = NULL;
	fb_info.renderPass              = render_pass->handle;
	fb_info.attachmentCount         = attachment_count;
	fb_info.pAttachments            = attachments;
	fb_info.width                   = extent.width;
	fb_info.height                  = extent.height;
	fb_info.layers                  = 1;

	framebuffer->desc   = *desc;
	framebuffer->extent = extent;
	return sht_vk_error(driver->CreateFramebuffer(driver->device, &fb_info, default_allocation_callbacks, &framebuffer->handle));
}

static void sht_vk_render_pass_begin(sht_command_buffer command, sht_vk_driver *driver, VkSurfaceKHR surface,
                                     const sht_vk_render_pass *render_pass, sht_vk_framebuffer *framebuffer, const sht_render_desc *desc)
{
	sht_vk_command *api            = (sht_vk_command *)command.owner;
	VkCommandBuffer command_buffer = (VkCommandBuffer)command.handle;

	fckc_size_t  clear_count = 0;
	VkClearValue clearValues[8];
	if (desc->colour.view.format != VK_FORMAT_UNDEFINED)
	{
		const fckc_f32 *values     = desc->colour.clear_value;
		clearValues[clear_count++] = (VkClearValue){.color = {values[0], values[1], values[2], values[3]}};
	}
	if (desc->depth.view.format != VK_FORMAT_UNDEFINED)
	{
		clearValues[clear_count++] = (VkClearValue){.depthStencil = {.depth = desc->depth.clear_value}};
	}

	VkExtent2D extent;
	sht_vk_crash(sht_vk_surface_size(driver->gpu, surface, &extent));

	VkRenderPassBeginInfo render_pass_begin_info = {0};
	render_pass_begin_info.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_begin_info.pNext                 = NULL;
	render_pass_begin_info.renderPass            = render_pass->handle;
	render_pass_begin_info.renderArea.offset.x   = 0;
	render_pass_begin_info.renderArea.offset.y   = 0;
	render_pass_begin_info.renderArea.extent     = framebuffer->extent;
	render_pass_begin_info.clearValueCount       = clear_count;
	render_pass_begin_info.pClearValues          = clearValues;
	render_pass_begin_info.framebuffer           = framebuffer->handle;

	api->CmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
}

static sht_render_pass sht_render_pass_begin(sht_command_buffer command_buffer, const sht_render_desc *desc)
{
	sht_vk_command *api    = (sht_vk_command *)command_buffer.owner;
	sht_vk_driver  *driver = api->driver;

	sht_vk_render_pass_desc render_pass_desc = sht_vk_render_pass_desc_from_render_desc(desc);
	fckc_size_t             find             = sht_vk_render_pass_storage_find(&driver->storages.render_pass, &render_pass_desc);
	sht_vk_render_pass     *render_pass;
	sht_vk_framebuffer     *framebuffer;
	if (!find)
	{
		// Neither exists - Need to create render pass and framebuffer!
		sht_vk_assert(driver->storages.render_pass.count < sht_vk_render_pass_storage_capacity);
		sht_vk_assert(driver->storages.framebuffer.count < sht_vk_framebuffer_storage_capacity);
		render_pass = driver->storages.render_pass.handles + driver->storages.render_pass.count;
		framebuffer = driver->storages.framebuffer.handles + driver->storages.framebuffer.count;

		VkRenderPass vk_render_pass;
		sht_vk_crash(sht_vk_render_pass_create(driver, &render_pass_desc, &vk_render_pass));
		render_pass->desc   = render_pass_desc;
		render_pass->handle = vk_render_pass;

		sht_vk_crash(sht_vk_framebuffer_create(driver, driver->swapchain.surface, render_pass, desc, framebuffer));
		driver->storages.render_pass.count = driver->storages.render_pass.count + 1;
		driver->storages.framebuffer.count = driver->storages.framebuffer.count + 1;
	}
	else
	{
		// Render pass exists!
		render_pass = driver->storages.render_pass.handles + (find - 1);

		find = sht_vk_framebuffers_storage_find(&driver->storages.framebuffer, desc);
		if (find)
		{
			// Both exist - We can just fetch render pass and framebuffer!
			framebuffer = driver->storages.framebuffer.handles + (find - 1);
		}
		else
		{
			// Render pass exists, framebuffer does not - need to create framebuffer
			sht_vk_assert(driver->storages.framebuffer.count < sht_vk_framebuffer_storage_capacity);
			framebuffer = driver->storages.framebuffer.handles + driver->storages.framebuffer.count;

			sht_vk_crash(sht_vk_framebuffer_create(driver, driver->swapchain.surface, render_pass, desc, framebuffer));
			driver->storages.framebuffer.count = driver->storages.framebuffer.count + 1;
		}
	}

	sht_vk_render_pass_begin(command_buffer, driver, driver->swapchain.surface, render_pass, framebuffer, desc);
	return (sht_render_pass){.handle = render_pass};
}

static VkBool32 sht_render_pass_is_ok(sht_render_pass render_pass)
{
	return render_pass.handle != NULL;
}

static void sht_render_pass_end(sht_command_buffer command)
{
	sht_vk_command *api            = (sht_vk_command *)command.owner;
	VkCommandBuffer command_buffer = (VkCommandBuffer)command.handle;
	api->CmdEndRenderPass(command_buffer);
}

static sht_swapchain sht_driver_get_swapchain(sht_driver driver)
{
	sht_vk_driver *vk_driver = sht_driver_to_vk(driver);
	return (sht_swapchain){.handle = &vk_driver->swapchain, .vt = &sht_swapchain_vt_api};
}

static sht_memory *sht_driver_get_memory(sht_driver driver)
{
	sht_vk_driver *vk_driver = sht_driver_to_vk(driver);
	return &vk_driver->memory;
}

static sht_bool32 sht_driver_is_ok(sht_driver driver)
{
	return driver.handle != NULL && driver.vt != NULL;
}

static int sht_driver_create_null_texture(sht_driver driver, sht_image *image, sht_image_view *view)
{
	sht_memory                   *memory = driver.vt->memory(driver);
	const sht_image_configuration config = {
		.format   = sht_format_r8g8b8a8_unorm,
		.height   = 1,
		.width    = 1,
		.transfer = sht_transfer_target,
		.usage    = sht_image_usage_sampled,
	};

	*image = memory->image->create(memory->bump, &config, sht_memory_gpu);
	if (!memory->image->is_ok(image))
	{
		return 0;
	}

	const fckc_u32 pixel = 0xFFFF00FF;

	const fckc_size_t size = 4;
	driver.vt->upload_image(driver, image, &pixel, size);

	*view = memory->image->view(memory->bump, *image, config.format);

	return 1;
}

static sht_driver sht_driver_create(sht_instance instance, struct fck_window *window)
{
	sht_vk_instance *vk = sht_instance_to_vk(instance);

	sht_vk_crash(sht_vk_gpu_init(vk, &vk->gpu));

	// The platform selects the GPU for now!!-!
	VkSurfaceKHR surface;
	sht_vk_crash(sht_vk_platform_init(vk, &vk->platform, &vk->gpu, *window, &surface));
	sht_vk_crash(sht_vk_queues_init(&vk->gpu.queues, &vk->gpu, surface));

	sht_vk_crash(sht_vk_driver_init(&vk->driver, &vk->gpu.queues));

	// fck_megabytes(128) * 2 configs * 2 arenas * 2 (one is coherent) - Ok, we reached a GB lmao
	sht_vk_crash(sht_vk_memory_init(&vk->driver.memory, &vk->driver, fck_megabytes(128)));
	sht_vk_crash(sht_vk_command_init(&vk->driver.command, &vk->driver, &vk->driver.gpu->queues));

	VkExtent2D extent;
	sht_vk_crash(sht_vk_surface_size(&vk->gpu, surface, &extent));
	sht_vk_crash(sht_vk_swapchain_init(&vk->driver.swapchain, &vk->driver, surface, extent));

	memset(&vk->driver.storages, 0, sizeof(vk->driver.storages));
	for (fckc_size_t index = 0; index < fck_arraysize(vk->driver.storages.bss.inflight.values); index++)
	{
		sht_vk_bss_node *dummy = vk->driver.storages.bss.inflight.values + index;
		dummy->prev            = &vk->driver.storages.bss.inflight;
		dummy->next            = &vk->driver.storages.bss.inflight;
	}
	vk->driver.window = *window;

	const sht_driver driver = {.handle = &vk->driver, .vt = &sht_driver_vt_api};
	const int        result = sht_driver_create_null_texture(driver, &vk->driver.null.image, &vk->driver.null.view);
	sht_vk_assert(result && "Could not create null texture!");

	vk->driver.null.sampler = driver.vt->create_sampler(driver, sht_filter_nearest);

	return driver;
}

static void sht_resource_storages_destroy(sht_resource_storages *storages, sht_vk_driver *driver)
{
	sht_vk_framebuffer_storage_destroy(&storages->framebuffer, driver);
	sht_vk_render_pass_storage_destroy(&storages->render_pass, driver);
	sht_vk_graphics_pipeline_storage_destroy(&storages->graphics_pipeline, driver);
	sht_vk_bss_storage_destroy(&storages->bss, driver);
	sht_invalidate(storages);
}

static void sht_driver_shutdown(sht_driver *driver)
{
	sht_vk_driver *vk_driver = sht_driver_to_vk(*driver);

	// Idle out!
	// Take surface away from swapchain since swapchain data will get invalidated...
	// What is a good spot to place the surface on AHHHHHHHH... Maybe external?
	// ... MAYBE SURFACE COMES FROM OUTSIDE?! YES! ok, We will see
	VkSurfaceKHR surface = vk_driver->swapchain.surface;
	vk_driver->DeviceWaitIdle(vk_driver->device);
	sht_vk_swapchain_destroy(&vk_driver->swapchain);
	sht_resource_storages_destroy(&vk_driver->storages, vk_driver);
	sht_vk_memory_destroy(&vk_driver->memory);
	sht_vk_command_destroy(&vk_driver->command);

	vk_driver->DestroySurfaceKHR(vk_driver->gpu->vk->instance, surface, NULL);
	vk_driver->DestroyDevice(vk_driver->device, default_allocation_callbacks);
	sht_invalidate(vk_driver);
	sht_invalidate(driver);
}

static void sht_driver_idle(sht_driver driver)
{
	sht_vk_driver *vk_driver = sht_driver_to_vk(driver);
	vk_driver->DeviceWaitIdle(vk_driver->device);
}

static fck_hash_int sht_shader_hash(fck_shader_compiler *compiler, fck_shader_generic *shader)
{
	const char *entry_point = compiler->entry_point(shader);
	const char *file        = compiler->file(shader);

	fck_hash_int hash = fck_hash(file, strlen(file));
	hash              = fck_hash_combine(hash, fck_hash(entry_point, strlen(entry_point)));
	return hash;
}

static sht_bool32 sht_shader_equals(fck_shader_compiler *compiler, fck_shader_generic *lhs, fck_shader_generic *rhs)
{
	if (compiler->type(lhs) != compiler->type(rhs))
	{
		return sht_false;
	}
	if (compiler->language(lhs) != compiler->language(rhs))
	{
		return sht_false;
	}

	if (strcmp(compiler->entry_point(lhs), compiler->entry_point(rhs)) != 0)
	{
		return sht_false;
	}

	if (strcmp(compiler->file(lhs), compiler->file(rhs)) != 0)
	{
		return sht_false;
	}
	// We never compare source - fuck that!
	return sht_true;
}

typedef struct sht_pointer_as_string
{
	char str[sizeof(void *)];
} sht_pointer_as_string;

static sht_pointer_as_string sht_pointer_to_string(void *pointer)
{
	sht_pointer_as_string as;
	for (fckc_size_t index = 0; index < fck_arraysize(as.str); index++)
	{
		as.str[index] = ((fckc_size_t)pointer >> index) & 0xFF;
	}
	return as;
}

static fckc_u32 sht_vk_graphics_pipeline_hash(sht_vk_graphics_pipeline const *pipeline)
{
	sht_pointer_as_string pas  = sht_pointer_to_string((void *)pipeline->pipeline);
	// sht_pointer_as_string cas = sht_pointer_to_string((void *)pipeline->cache);
	// sht_pointer_as_string las = sht_pointer_to_string((void *)pipeline->layout);
	const fck_hash_int    hash = fck_hash(pas.str, fck_arraysize(pas.str));
	// hash = fck_hash_combine(hash, fck_hash(cas.str, fck_arraysize(cas.str)));
	// hash = fck_hash_combine(hash, fck_hash(las.str, fck_arraysize(las.str)));
	// For now the pipeline and the other state are 1:1 mappes, so no need to hash more then needed
	return (fckc_u32)((hash ^ (hash >> 32)) * 0x9E3779B9);
}

static fck_hash_int sht_vk_graphics_pipeline_equals(sht_vk_graphics_pipeline const *lhs, sht_vk_graphics_pipeline const *rhs)
{
	return lhs->pipeline == rhs->pipeline && lhs->cache == rhs->cache;
}

static sht_vk_graphics_pipeline *sht_vk_graphics_pipeline_find(sht_vk_graphics_pipeline_storage *storage, sht_graphics_pipeline_key handle)
{
	fck_hash_int at = handle.hash % fck_arraysize(storage->handles);
	for (;;)
	{
		sht_graphics_pipeline_key const *key = storage->keys + at;
		if (key->invalid == 0)
		{
			if (key->hash == 0)
			{
				return NULL;
			}
			if (key->hash == handle.hash)
			{
				if (key->generation != handle.generation)
				{
					// Maybe log error
					return NULL;
				}
				return storage->handles + at;
			}
		}
		at = (at + 1) % fck_arraysize(storage->handles);
	}
}

static sht_graphics_pipeline_key sht_vk_graphics_pipeline_storage_add(sht_vk_graphics_pipeline_storage *storage,
                                                                      sht_vk_graphics_pipeline const   *pipeline)
{
	if (storage->count >= fck_arraysize(storage->handles) / 2)
	{
		sht_vk_assert(0 && "come up with fallback");
		return (sht_graphics_pipeline_key){.invalid = 1};
	}

	const fck_hash_int hash = sht_vk_graphics_pipeline_hash(pipeline);
	if (hash == 0)
	{
		return (sht_graphics_pipeline_key){.invalid = 1};
	}

	fck_hash_int at = hash % fck_arraysize(storage->handles);
	for (;;)
	{
		// We have bytes (u8) and each control element has two bits! So 4 indices map to one control byte
		sht_graphics_pipeline_key *key = storage->keys + at;
		if (key->hash == 0)
		{
			storage->count                   = storage->count + 1;
			sht_vk_graphics_pipeline *result = storage->handles + at;
			*result                          = *pipeline;
			// We drop 1 bit, let's pray!
			key->hash                        = hash;
			key->invalid                     = 0;
			return *key;
		}
		at = (at + 1) % fck_arraysize(storage->handles);
	}
}

sht_bool32 sht_vk_graphics_pipeline_storage_remove(sht_vk_graphics_pipeline_storage *storage, sht_vk_driver *driver,
                                                   sht_graphics_pipeline_key handle)
{
	if (storage->count == 0)
	{
		return sht_false;
	}

	fck_hash_int at = handle.hash % fck_arraysize(storage->handles);
	for (;;)
	{
		sht_graphics_pipeline_key *key = storage->keys + at;
		if (key->invalid == 1)
		{
			continue;
		}
		if (key->hash == 0)
		{
			return sht_false;
		}
		if (key->hash == handle.hash)
		{
			if (key->generation != handle.generation)
			{
				// Maybe log error
				return sht_false;
			}
			key->generation = key->generation + 1;
			key->hash       = 0;
			key->invalid    = 1;

			sht_vk_graphics_pipeline *pipeline = storage->handles + at;
			driver->DestroyPipeline(driver->device, pipeline->pipeline, default_allocation_callbacks);
			driver->DestroyPipelineCache(driver->device, pipeline->cache, default_allocation_callbacks);

			sht_invalidate(pipeline);
			return sht_true;
		}
		at = (at + 1) % fck_arraysize(storage->handles);
	}
}

static sht_graphics_pipeline sht_driver_graphics_pipeline_create(sht_driver driver, sht_bss bss, const sht_graphic_desc *desc)
{
	sht_vk_driver *vk_driver = sht_driver_to_vk(driver);

	fck_shader_api *shader_api = (fck_shader_api *)apis->find(fck_shader_api_name);

	// TODO: Do not use extern declared function - Use API
	fck_shader_compiler             compiler = shader_api->create();
	VkPipelineShaderStageCreateInfo stages[] = {
		[SHT_STANDARD_STAGE_VERTEX] =
			{
				.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.pNext               = NULL,
				.pSpecializationInfo = NULL,
				.stage               = VK_SHADER_STAGE_VERTEX_BIT,
				.flags               = 0,
				.module              = NULL,
				.pName               = compiler.entry_point(desc->vertex),
			},
		[SHT_STANDARD_STAGE_FRAGMENT] =
			{
				.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.pNext               = NULL,
				.pSpecializationInfo = NULL,
				.stage               = VK_SHADER_STAGE_FRAGMENT_BIT,
				.flags               = 0,
				.module              = NULL,
				.pName               = compiler.entry_point(desc->fragment),
			},
	};
	sht_vk_crash(sht_vk_shader_module_create(        //
		&compiler, vk_driver,                        //
		desc->vertex,                                //
		&stages[SHT_STANDARD_STAGE_VERTEX].module)); //
	sht_vk_crash(sht_vk_shader_module_create(        //
		&compiler, vk_driver,                        //
		desc->fragment,                              //
		&stages[SHT_STANDARD_STAGE_FRAGMENT].module));

	compiler.shutdown(&compiler);

	// We have a graphic pipeline
	// The graphic pipeline has x shaders
	// The descriptor set layout (and its bindings) express which variables are around!
	// Let's say each vertex shader has ONE and gets only ever ONE UBO. Nothing else for now!
	sht_vk_render_pass_desc temp_desc;
	temp_desc.depth.format    = desc->raster.depth;
	temp_desc.depth.load_op   = sht_dont_care;
	temp_desc.depth.store_op  = sht_dont_care;
	temp_desc.colour.format   = desc->raster.color;
	temp_desc.colour.load_op  = sht_dont_care;
	temp_desc.colour.store_op = sht_dont_care;

	VkRenderPass temp;
	sht_vk_crash(sht_vk_render_pass_create(vk_driver, &temp_desc, &temp));

	sht_vk_bss *vk_bss = (sht_vk_bss *)bss.handle;

	sht_vk_descriptor_pool_storage_entry *pool_entry = sht_vk_descriptor_pool_storage_key_resolve(vk_bss->pool_storage_key);

	sht_vk_graphics_pipeline graphics_pipeline;
	sht_vk_crash(sht_vk_graphics_pipeline_create(vk_driver, temp, &pool_entry->layout, pool_entry->pipeline_layout, 1, stages,
	                                             fck_arraysize(stages), desc->vertex_desc, &graphics_pipeline));

	sht_graphics_pipeline_key stable = sht_vk_graphics_pipeline_storage_add(&vk_driver->storages.graphics_pipeline, &graphics_pipeline);
	vk_driver->DestroyRenderPass(vk_driver->device, temp, default_allocation_callbacks);

	vk_driver->DestroyShaderModule(vk_driver->device, stages[SHT_STANDARD_STAGE_VERTEX].module, default_allocation_callbacks);
	vk_driver->DestroyShaderModule(vk_driver->device, stages[SHT_STANDARD_STAGE_FRAGMENT].module, default_allocation_callbacks);
	sht_graphics_pipeline result;
	result.owner = (sht_handle *)vk_driver;
	(void)memcpy(&result.handle, &stable, sizeof(stable));
	return result;
}

sht_bool32 sht_driver_graphics_pipeline_is_ok(sht_graphics_pipeline pipeline)
{
	sht_vk_driver *vk_driver = (sht_vk_driver *)pipeline.owner;
	return vk_driver != NULL;
}

static void sht_driver_graphics_pipeline_destroy(sht_graphics_pipeline pipeline)
{
	sht_vk_driver            *vk_driver = (sht_vk_driver *)pipeline.owner;
	sht_graphics_pipeline_key stable;
	(void)memcpy(&stable, &pipeline.handle, sizeof(stable));

	sht_vk_graphics_pipeline_storage_remove(&vk_driver->storages.graphics_pipeline, vk_driver, stable);
}

static void sht_command_buffer_graphics_pipeline(sht_command_buffer command, sht_graphics_pipeline pipeline)
{
	sht_vk_command *api            = (sht_vk_command *)command.owner;
	sht_vk_driver  *vk_driver      = (sht_vk_driver *)pipeline.owner;
	VkCommandBuffer command_buffer = (VkCommandBuffer)command.handle;

	sht_vk_assert(api->driver == vk_driver && "Must be same driver!");
	sht_graphics_pipeline_key stable;
	(void)memcpy(&stable, &pipeline.handle, sizeof(stable));
	sht_vk_graphics_pipeline *vk_pipeline = sht_vk_graphics_pipeline_find(&vk_driver->storages.graphics_pipeline, stable);

	sht_vk_assert(vk_pipeline != NULL);
	api->CmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline->pipeline);
}

static void sht_command_buffer_bss(sht_command_buffer command, sht_bss bss)
{
	sht_vk_bss     *vk_bss         = (sht_vk_bss *)bss.handle;
	VkCommandBuffer command_buffer = (VkCommandBuffer)command.handle;
	sht_vk_command *api            = (sht_vk_command *)command.owner;

	const fckc_u32 index = api->driver->swapchain.sync.index;
	// VkDescriptorSet set = vk_bss->sets[index];

	// Make a copy, we bind the copy - This means we can always send it out
	// sht_vk_descriptor_set_copies *set_copies = vk_bss->copies + index;
	VkDescriptorSet src = vk_bss->latest[index];
	// set_copies->at = (set_copies->at + 1) % sht_vk_bss_binding_capacity;

	sht_vk_descriptor_pool_storage_entry *pool_entry = sht_vk_descriptor_pool_storage_key_resolve(vk_bss->pool_storage_key);
	VkDescriptorSet                       dst;
	sht_vk_crash(sht_vk_descriptor_set_create(api->driver, pool_entry->dynamic_pools[index], pool_entry->layout, &dst, 1));
	sht_vk_command_buffer_bss_copy(api->driver, vk_bss, src, dst);

	sht_vk_descriptor_pool_storage_entry *entry = sht_vk_descriptor_pool_storage_key_resolve(vk_bss->pool_storage_key);
	api->CmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, entry->pipeline_layout, 0, 1, &src, 0, NULL);

	vk_bss->latest[index] = dst;

	sht_vk_bss_node *inflight = api->driver->storages.bss.inflight.values + index;
	sht_vk_bss_node *current  = vk_bss->nodes.values + index;
	if (current->next == NULL)
	{
		vk_bss->refcount = vk_bss->refcount + 1;

		sht_vk_bss_nodes *inflight_nodes = &api->driver->storages.bss.inflight;
		sht_vk_bss_nodes *tail_nodes     = inflight->prev;
		sht_vk_bss_nodes *bss_nodes      = &vk_bss->nodes;

		sht_vk_bss_node *tail_node = tail_nodes->values + index;
		sht_vk_bss_node *bss_node  = bss_nodes->values + index;

		tail_node->next = bss_nodes;
		bss_node->next  = inflight_nodes;
		bss_node->prev  = tail_nodes;
		inflight->prev  = bss_nodes;
	}
}

static fckc_size_t sht_bss_binding_find(sht_vk_bss *bss, fckc_u32 id)
{
	// Might make more sense to count the bindings that are of a certain type up to that point
	// I.e., accum
	// TODO: Do that ^
	for (fckc_size_t index = 0; index < bss->desc.count; index++)
	{
		sht_binding *binding = bss->desc.bindings + index;
		if (binding->id == id)
		{
			return index + 1;
		}
	}
	return 0;
}

static void sht_bss_buffer_resize(sht_memory *mem, sht_buffer_usage_flags usage, sht_buffer *buffer, const void *data, fckc_size_t size,
                                  VkDeviceSize offset)
{
	if (buffer->size < size + offset)
	{
		fckc_size_t total = size + offset;
		if (buffer->cpu != NULL)
		{
			total = total * 2;
		}

		sht_buffer temp = mem->malloc(mem->bump, &sht_buffer_retained(usage, total), sht_memory_cpu);
		if (buffer->cpu != NULL)
		{
			memcpy(temp.cpu, buffer->cpu, buffer->size);
			// We do need to free the buffer
			// Make a GC for this bad boy! :)
			// mem->free(mem->bump, buffer);
		}
		*buffer = temp;
	}

	// TODO: A transfer might make sense... but... what if we just do very smart offset coherent uploads?
	memcpy((fckc_u8 *)buffer->cpu + offset, data, size);
}

static sht_buffer_usage_flags sht_binding_type_to_usage_flags(sht_binding_type type)
{
	switch (type)
	{
	case sht_binding_uniform:
		return sht_buffer_usage_uniform;
	case sht_binding_storage:
		return sht_buffer_usage_storage;
	case sht_binding_none:
	case sht_binding_readonly_image:
	case sht_binding_count:
		break;
	}
	return 0;
}

static sht_bool32 sht_bss_upload_buffer(sht_bss bss, fckc_u32 id, const sht_buffer_upload_desc *desc)
{
	// TODO: Less assert, more return false (TODO: Make error code)

	sht_vk_bss    *vk_bss = (sht_vk_bss *)bss.handle;
	sht_vk_driver *driver = (sht_vk_driver *)bss.owner;

	const fckc_u32 index = driver->swapchain.sync.index;
	sht_vk_assert(index < sht_frame_count);

	VkDescriptorSet *set = &vk_bss->latest[index];

	sht_bss_buffer_backends *buffer_backend = vk_bss->buffer_backends + index;

	const fckc_size_t find = sht_bss_binding_find(vk_bss, id);
	sht_vk_assert(find); // Crash! MWAH
	const fckc_size_t at = find - 1;
	sht_vk_assert(at < sht_vk_bss_binding_capacity);

	sht_binding  *binding = vk_bss->desc.bindings + at;
	sht_buffer   *buffer  = buffer_backend->buffers + at;
	VkDeviceSize *offset  = buffer_backend->offsets + at;

	sht_vk_assert(binding->type == sht_binding_uniform || binding->type == sht_binding_storage);
	// TODO: Each of these buffers should be a bump allocator.
	// This is because the coherent memory update and the queue execution are two separate phases
	// So it should be that each time we bind a bss, we have to bump it up by the size written
	sht_buffer_usage_flags usage = sht_binding_type_to_usage_flags(binding->type);

	VkPhysicalDeviceProperties device_properties;
	driver->gpu->GetPhysicalDeviceProperties(driver->gpu->device, &device_properties);

	VkDeviceSize offset_alignment;
	switch (binding->type)
	{
	case sht_binding_uniform:
		offset_alignment = device_properties.limits.minUniformBufferOffsetAlignment;
		break;
	case sht_binding_storage:
		offset_alignment = device_properties.limits.minStorageBufferOffsetAlignment;
		break;
	default:
		offset_alignment = 0;
		break;
	}

	*offset = fckc_align(*offset, offset_alignment);
	sht_bss_buffer_resize(&driver->memory, usage, buffer, desc->data, desc->size * desc->count, *offset);
	sht_vk_descriptor_set_update_buffer(driver, *set, binding, buffer, *offset);
	*offset = *offset + desc->size * desc->count;
	return sht_true;
}

static sht_bool32 sht_bss_upload_image(sht_bss bss, fckc_u32 id, const sht_image_upload_desc *desc)
{
	// TODO: Less assert, more return false (TODO: Make error code)

	sht_vk_bss    *vk_bss = (sht_vk_bss *)bss.handle;
	sht_vk_driver *driver = (sht_vk_driver *)bss.owner;

	const fckc_u32 index = driver->swapchain.sync.index;
	sht_vk_assert(index < sht_frame_count);

	VkDescriptorSet *set = &vk_bss->latest[index];

	const fckc_size_t find = sht_bss_binding_find(vk_bss, id);
	sht_vk_assert(find); // Crash! MWAH
	const fckc_size_t at = find - 1;
	sht_vk_assert(at < sht_vk_bss_binding_capacity);

	sht_binding *binding = vk_bss->desc.bindings + at;
	sht_vk_assert(binding->type == sht_binding_readonly_image);

	// We do not allow Array of Samplers/Views for now.
	const sht_image_view *view = desc->view;
	if (view == NULL || view->gpu == NULL)
	{
		view = &driver->null.view;
	}

	const sht_sampler *sampler = desc->sampler;
	if (sampler == NULL || sampler->handle == NULL)
	{
		sampler = &driver->null.sampler;
	}

	sht_vk_descriptor_set_update_image(driver, *set, binding, view, sampler, 1);
	return sht_true;
}

static sht_instance sht_vk_load(fckc_u32 version)
{
	sht_vk_instance *vk = kll_malloc(kll->system, sizeof(*vk));
	vk->allocator       = kll->system;
	vk->name            = "sht-vulkan";
	sht_vk_crash(sht_vk_instance_init(vk));

	sht_instance public;
	public.vt     = &sht_instance_vt_api;
	public.handle = vk;
	return public;
}

static void sht_vk_unload(sht_instance *instance)
{
	sht_vk_instance *vk = (sht_vk_instance *)instance->handle;

	instance->vt = NULL;
	kll_free(vk->allocator, vk);
	instance->handle = NULL;
}

static sht_bool32 sht_vk_is_ok(sht_instance instance)
{
	return instance.handle != NULL;
}

FCK_EXPORT_API sht_render_api *fck_render_vk_load(fck_api_registry *registry, sht_render_api_config *config)
{
	apis = registry;
	sht_vk_assert(registry && "Cannot use sht-renderer-vk without API registry available");

	// Maybe here we load the shared object ;)
	registry->add(sht_render_api_name, &sht_loader_api);
	return &sht_loader_api;
}

static sht_bss_vt sht_bss_vt_api = {
	.create        = sht_bss_create,
	.upload_buffer = sht_bss_upload_buffer,
	.upload_image  = sht_bss_upload_image,
	.destroy       = sht_bss_destroy,
};

static sht_command_buffer_vt sht_command_buffer_vt_api = {
	.create            = sht_command_buffer_create,
	.destroy           = sht_command_buffer_destroy,
	.acquire           = sht_command_buffer_acquire,
	.is_ok             = sht_command_buffer_is_ok,
	.scissor           = sht_command_buffer_scissor,
	.viewport          = sht_command_buffer_viewport,
	.vertex_buffer     = sht_command_buffer_vertex_buffer,
	.bss               = sht_command_buffer_bss,
	.index_buffer      = sht_command_buffer_index_buffer,
	.draw_indexed      = sht_command_buffer_draw_indexed,
	.submit            = sht_command_buffer_submit,
	.graphics_pipeline = sht_command_buffer_graphics_pipeline,
	.render_pass       = &sht_render_pass_vt_api,
	// Rest null for now
};

static sht_graphics_pipeline_vt sht_graphics_pipeline_vt_api = {
	.create  = sht_driver_graphics_pipeline_create,
	.is_ok   = sht_driver_graphics_pipeline_is_ok,
	.destroy = sht_driver_graphics_pipeline_destroy,
};

static sht_render_pass_vt sht_render_pass_vt_api = {
	.begin = sht_render_pass_begin,
	.is_ok = sht_render_pass_is_ok,
	.end   = sht_render_pass_end,
};

static sht_swapchain_vt sht_swapchain_vt_api = {
	.wait_and_acquire = sht_swapchain_wait_and_acquire,
	.is_ok            = sht_swapchain_is_ready,
	.extent           = sht_swapchain_extent,
	.scale            = sht_swapchain_scale,
	.display          = sht_swapchain_display,
};

static sht_driver_vt sht_driver_vt_api = {
	.shutdown = sht_driver_shutdown,
	.idle     = sht_driver_idle,

	.swapchain     = sht_driver_get_swapchain,
	.memory        = sht_driver_get_memory,
	.copy_buffers  = sht_driver_copy_buffer,
	.upload_buffer = sht_driver_upload_buffer,

	.create_sampler    = sht_driver_create_sampler,
	.destroy_sampler   = sht_driver_destroy_sampler,
	.upload_image      = sht_driver_upload_image,
	.command_buffer    = &sht_command_buffer_vt_api,
	.graphics_pipeline = &sht_graphics_pipeline_vt_api,
	.bss               = &sht_bss_vt_api,
};

static sht_instance_vt sht_instance_vt_api = {
	.start  = sht_driver_create,
	.is_ok  = sht_driver_is_ok,
	.unload = sht_vk_unload,
};

static sht_render_api sht_loader_api = {
	.load  = sht_vk_load,
	.is_ok = sht_vk_is_ok,
};
