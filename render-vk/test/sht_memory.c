

#include "sht_vk.internal.h"

// Stolen from good ol VK, maybe it is better... probs
// Find a memory in `memoryTypeBitsRequirement` that includes all of `requiredProperties`
VkBool32 sht_vk_query_memory_type_index(const VkPhysicalDeviceMemoryProperties *properties, uint32_t mem_type_bits,
                                        VkMemoryPropertyFlags flags, uint32_t *type_index)
{
	const uint32_t count = properties->memoryTypeCount;
	for (uint32_t index = 0; index < count; ++index)
	{
		const uint32_t memory_type_bits = (1 << index);
		const int is_required = mem_type_bits & memory_type_bits;

		const VkMemoryPropertyFlags props = properties->memoryTypes[index].propertyFlags;
		const int has_requirements = (props & flags) == flags;

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
	if (sht_test(config->transfer, SHT_TRANSFER_SOURCE))
	{
		flags = flags | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	}
	if (sht_test(config->transfer, SHT_TRANSFER_TARGET))
	{
		flags = flags | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	}

	if (sht_test(config->usage, SHT_BUFFER_USAGE_UNIFORM))
	{
		flags = flags | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	}
	if (sht_test(config->usage, SHT_BUFFER_USAGE_STORAGE))
	{
		flags = flags | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	}
	if (sht_test(config->usage, SHT_BUFFER_USAGE_INDEX))
	{
		flags = flags | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	}
	if (sht_test(config->usage, SHT_BUFFER_USAGE_VERTEX))
	{
		flags = flags | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	}
	if (sht_test(config->usage, SHT_BUFFER_USAGE_INDIRECT))
	{
		flags = flags | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
	}
	return flags;
}

VkFormat sht_vk_format_from_sht_format(sht_format format)
{
	switch (format)
	{
	case SHT_FORMAT_UNDEFINED:
		return VK_FORMAT_UNDEFINED;
	case SHT_FORMAT_R8G8B8A8_UNORM:
		return VK_FORMAT_R8G8B8A8_UNORM;
	case SHT_FORMAT_R8G8B8A8_SRGB:
		return VK_FORMAT_R8G8B8A8_SRGB;
	case SHT_FORMAT_B8G8R8A8_UNORM:
		return VK_FORMAT_B8G8R8A8_UNORM;
	case SHT_FORMAT_B8G8R8A8_SRGB:
		return VK_FORMAT_B8G8R8A8_SRGB;
	case SHT_FORMAT_D16_UNORM:
		return VK_FORMAT_D16_UNORM;
	case SHT_FORMAT_R32G32B32_SFLOAT:
		return VK_FORMAT_R32G32B32_SFLOAT;
	case SHT_FORMAT_R32_SFLOAT:
		return VK_FORMAT_R32_SFLOAT;
	case SHT_FORMAT_R32G32_SFLOAT:
		return VK_FORMAT_R32G32_SFLOAT;
	case SHT_FORMAT_R32G32B32A32_SFLOAT:
		return VK_FORMAT_R32G32B32A32_SFLOAT;
	}
	return VK_FORMAT_UNDEFINED;
}

sht_format sht_vk_format_to_sht_format(VkFormat format)
{
	switch (format)
	{
	case VK_FORMAT_UNDEFINED:
		return SHT_FORMAT_UNDEFINED;
	case VK_FORMAT_R8G8B8A8_UNORM:
		return SHT_FORMAT_R8G8B8A8_UNORM;
	case VK_FORMAT_R8G8B8A8_SRGB:
		return SHT_FORMAT_R8G8B8A8_SRGB;
	case VK_FORMAT_B8G8R8A8_UNORM:
		return SHT_FORMAT_B8G8R8A8_UNORM;
	case VK_FORMAT_B8G8R8A8_SRGB:
		return SHT_FORMAT_B8G8R8A8_SRGB;
	case VK_FORMAT_D16_UNORM:
		return SHT_FORMAT_D16_UNORM;
	case VK_FORMAT_R32G32B32_SFLOAT:
		return SHT_FORMAT_R32G32B32_SFLOAT;
	case VK_FORMAT_R32_SFLOAT:
		return SHT_FORMAT_R32_SFLOAT;
	case VK_FORMAT_R32G32_SFLOAT:
		return SHT_FORMAT_R32G32_SFLOAT;
	case VK_FORMAT_R32G32B32A32_SFLOAT:
		return SHT_FORMAT_R32G32B32A32_SFLOAT;
	default:
		return SHT_FORMAT_UNDEFINED;
	}
}

static VkImageUsageFlags sht_vk_usage_flags_from_image_config(sht_image_configuration *config)
{
	VkBufferUsageFlags flags = 0;
	if (sht_test(config->transfer, SHT_TRANSFER_SOURCE))
	{
		flags = flags | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	}
	if (sht_test(config->transfer, SHT_TRANSFER_TARGET))
	{
		flags = flags | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	}

	if (sht_test(config->usage, SHT_IMAGE_USAGE_SAMPLED))
	{
		flags = flags | VK_IMAGE_USAGE_SAMPLED_BIT;
	}
	if (sht_test(config->usage, SHT_IMAGE_USAGE_STORAGE))
	{
		flags = flags | VK_IMAGE_USAGE_STORAGE_BIT;
	}
	if (sht_test(config->usage, SHT_IMAGE_USAGE_COLOR_ATTACHMENT))
	{
		flags = flags | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	}
	if (sht_test(config->usage, SHT_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT))
	{
		flags = flags | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	}
	if (sht_test(config->usage, SHT_IMAGE_USAGE_INPUT_ATTACHMENT))
	{
		flags = flags | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
	}
	// if (sht_test(config->usage, SHT_IMAGE_USAGE_TRANSIENT_ATTACHMENT))
	//{
	//	flags = flags | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
	// }
	return flags;
}

int sht_vk_image_has_color_aspect(sht_image_usage_flags usage)
{
	int result = sht_test(usage, SHT_IMAGE_USAGE_SAMPLED) ||          //
	             sht_test(usage, SHT_IMAGE_USAGE_STORAGE) ||          //
	             sht_test(usage, SHT_IMAGE_USAGE_COLOR_ATTACHMENT) || //
	             sht_test(usage, SHT_IMAGE_USAGE_INPUT_ATTACHMENT);   //
	return result;
}

static VkImageAspectFlags sht_vk_image_aspect_from_config(sht_image_usage_flags usage)
{
	// TODO: SUPPORT FOR STENCIL!! We could see if the format is two-dimensional? i do not know :/ Need to read specs
	VkImageAspectFlagBits flags = VK_IMAGE_ASPECT_NONE;
	if (sht_test(usage, SHT_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT))
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
	if (sht_test(config->usage, SHT_IMAGE_USAGE_SAMPLED))
	{
		return SHT_FORMAT_B8G8R8A8_UNORM;
	}
	if (sht_test(config->usage, SHT_IMAGE_USAGE_STORAGE))
	{
		return SHT_FORMAT_B8G8R8A8_UNORM;
	}
	if (sht_test(config->usage, SHT_IMAGE_USAGE_COLOR_ATTACHMENT))
	{
		return SHT_FORMAT_B8G8R8A8_UNORM;
	}
	if (sht_test(config->usage, SHT_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT))
	{
		return SHT_FORMAT_D16_UNORM;
	}
	// if (sht_test(config->usage, SHT_IMAGE_USAGE_TRANSIENT_ATTACHMENT))
	//{
	//	return SHT_FORMAT_B8G8R8A8_UNORM;
	// }
	if (sht_test(config->usage, SHT_IMAGE_USAGE_INPUT_ATTACHMENT))
	{
		return SHT_FORMAT_UNDEFINED;
	}

	return SHT_FORMAT_UNDEFINED;
}

sht_image sht_memory_arena_image_create(sht_memory_arena *mem, sht_image_configuration *config, sht_memory_type memory_type)
{
	sht_vk_driver *driver = (sht_vk_driver *)mem->opaque;

	if (config->format == SHT_FORMAT_UNDEFINED)
	{
		config->format = sht_vk_image_resolve_format_from_config(config);
		// If format is still undefined, let's pray the render backend wiill help us out!
	}

	VkImageUsageFlags usage = sht_vk_usage_flags_from_image_config(config);
	VkFormat format = sht_vk_format_from_sht_format(config->format);

	VkFormatProperties props;
	driver->gpu->GetPhysicalDeviceFormatProperties(driver->gpu->device, format, &props);

	VkImageCreateInfo image_info = {0};
	if (sht_test(config->usage, SHT_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT))
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
	else if (sht_test(config->usage, SHT_IMAGE_USAGE_COLOR_ATTACHMENT))
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
	else if (sht_test(config->usage, SHT_IMAGE_USAGE_SAMPLED))
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

	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.pNext = NULL;
	image_info.flags = 0;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.format = format;
	image_info.extent.width = config->width;
	image_info.extent.height = config->height;
	image_info.extent.depth = 1;
	image_info.mipLevels = 1;
	image_info.arrayLayers = 1;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_info.usage = usage;
	image_info.queueFamilyIndexCount = 0;
	image_info.pQueueFamilyIndices = NULL;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	fckc_size_t *offset = &mem->offset[memory_type];

	VkImage image;
	sht_vk_crash(driver->CreateImage(driver->device, &image_info, NULL, &image));

	VkMemoryRequirements memory_requirements;
	driver->GetImageMemoryRequirements(driver->device, image, &memory_requirements);

	fckc_size_t capacity = mem->capacity[memory_type];
	if (*offset + memory_requirements.size > capacity)
	{
		*offset = 0;
	}
	*offset = fckc_align(*offset, memory_requirements.alignment);

	VkDeviceMemory memory = (VkDeviceMemory)mem->heaps[memory_type];
	sht_vk_crash(driver->BindImageMemory(driver->device, image, memory, *offset));

	sht_image out;
	out.cpu = NULL;
	out.heap = (sht_heap *)memory;
	out.gpu = image;
	out.width = config->width;
	out.height = config->height;
	out.format = config->format;
	out.usage = config->usage;
	if (memory_type == SHT_MEMORY_CPU)
	{
		void *cpu;
		out.cpu = (void *)((fckc_u8 *)mem->cpu[SHT_MEMORY_CPU] + *offset);
	}

	*offset = *offset + memory_requirements.size;
	return out;
}

sht_image_view sht_memory_arena_image_view(sht_memory_arena *mem, sht_image image, fck_alias(sht_format, fckc_u32) incoming_format)
{
	if (incoming_format == SHT_FORMAT_UNDEFINED)
	{
		incoming_format = image.format;
	}

	sht_vk_driver *driver = (sht_vk_driver *)mem->opaque;

	VkFormat format = sht_vk_format_from_sht_format(incoming_format);
	VkImageAspectFlags aspect_mask = sht_vk_image_aspect_from_config(image.usage);

	VkImageViewCreateInfo view_info = {0};
	view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_info.pNext = NULL;
	view_info.format = format;
	view_info.components.r = VK_COMPONENT_SWIZZLE_R;
	view_info.components.g = VK_COMPONENT_SWIZZLE_G;
	view_info.components.b = VK_COMPONENT_SWIZZLE_B;
	view_info.components.a = VK_COMPONENT_SWIZZLE_A;
	view_info.subresourceRange.aspectMask = aspect_mask;
	view_info.subresourceRange.baseMipLevel = 0;
	view_info.subresourceRange.levelCount = 1;
	view_info.subresourceRange.baseArrayLayer = 0;
	view_info.subresourceRange.layerCount = 1;
	view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view_info.flags = 0;
	view_info.image = (VkImage)image.gpu;

	VkImageView view;
	sht_vk_crash(driver->CreateImageView(driver->device, &view_info, NULL, &view));

	sht_image_view out;
	out.format = incoming_format;
	out.gpu = (void *)view;
	return out;
}

void sht_memory_arena_image_discard(sht_memory_arena *mem, sht_image_view *view)
{
	sht_vk_driver *driver = (sht_vk_driver *)mem->opaque;
	driver->DestroyImageView(driver->device, (VkImageView)view->gpu, NULL);
	view->gpu = VK_NULL_HANDLE;
	view->format = VK_FORMAT_UNDEFINED;
}

void sht_memory_arena_image_destroy(sht_memory_arena *mem, sht_image *image)
{
	sht_vk_driver *driver = (sht_vk_driver *)mem->opaque;

	driver->DestroyImage(driver->device, (VkImage)image->gpu, NULL);
	image->gpu = VK_NULL_HANDLE;
	image->cpu = NULL;
	image->format = SHT_FORMAT_UNDEFINED;
	image->width = 0;
	image->height = 0;
	image->heap = (sht_heap *)VK_NULL_HANDLE;
}

sht_buffer sht_memory_arena_malloc(sht_memory_arena *mem, sht_buffer_configuration *config, sht_memory_type memory_type)
{
	sht_vk_driver *driver = (sht_vk_driver *)mem->opaque;
	VkDevice device = driver->device;

	VkBufferCreateInfo create_info = {0};
	// We are lazy with this one just because
	create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	create_info.pNext = NULL;
	create_info.size = config->size;
	create_info.flags = (VkBufferCreateFlags)0;

	// OUT OF SCOPE
	create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	create_info.queueFamilyIndexCount = 0;
	create_info.pQueueFamilyIndices = NULL;
	// !OUT OF SCOPE

	create_info.usage = sht_vk_usage_flags_from_config(config);

	fckc_size_t *offset = &mem->offset[memory_type];

	VkBuffer buffer;
	sht_vk_crash(driver->CreateBuffer(device, &create_info, NULL, &buffer));

	VkMemoryRequirements memory_requirements;
	driver->GetBufferMemoryRequirements(device, buffer, &memory_requirements);

	fckc_size_t capacity = mem->capacity[memory_type];
	if (*offset + config->size > capacity)
	{
		*offset = 0;
	}

	*offset = fckc_align(*offset, memory_requirements.alignment);

	VkDeviceMemory memory = (VkDeviceMemory)mem->heaps[memory_type];
	sht_vk_crash(driver->BindBufferMemory(device, buffer, memory, *offset));

	sht_buffer out;
	out.size = memory_requirements.size;
	out.cpu = NULL;
	out.heap = (sht_heap *)memory;
	out.gpu = buffer;

	if (memory_type == SHT_MEMORY_CPU)
	{
		void *cpu;
		out.cpu = (void *)((fckc_u8 *)mem->cpu[SHT_MEMORY_CPU] + *offset);
	}

	*offset = *offset + config->size;

	return out;
}

void sht_memory_arena_free(sht_memory_arena *mem, sht_buffer *buffer)
{
	sht_vk_driver *driver = (sht_vk_driver *)mem->opaque;

	driver->DestroyBuffer(driver->device, (VkBuffer)buffer->gpu, NULL);
	buffer->gpu = VK_NULL_HANDLE;
	buffer->cpu = NULL;
	buffer->size = 0;
	buffer->heap = (sht_heap *)VK_NULL_HANDLE;
}

void sht_memory_arena_reset(sht_memory_arena *mem)
{
	sht_vk_driver *driver = (sht_vk_driver *)mem->opaque;

	for (fckc_size_t index = 0; index < SHT_MEMORY_COUNT; index++)
	{
		mem->offset[index] = 0;
	}
}

VkDeviceMemory sht_memory_arena_of(sht_memory_arena *mem, sht_heap *heap)
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

VkResult sht_memory_arena_init(sht_memory_arena *mem, sht_vk_driver *driver, fckc_size_t size)
{
	mem->opaque = (void *)driver;
	VkPhysicalDeviceMemoryProperties properties;
	driver->gpu->GetPhysicalDeviceMemoryProperties(driver->gpu->device, &properties);

	VkMemoryPropertyFlags configs[SHT_MEMORY_COUNT] = {
		[SHT_MEMORY_GPU] = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		[SHT_MEMORY_CPU] = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	};

	VkMemoryAllocateInfo info;
	info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	info.pNext = NULL;
	info.allocationSize = size;

	for (fckc_size_t index = 0; index < fck_arraysize(configs); index++)
	{
		if (!sht_vk_query_memory_type_index(&properties, ~0, configs[index], &info.memoryTypeIndex))
		{
			// Idk...
			sht_vk_crash(VK_ERROR_UNKNOWN);
		}
		VkDeviceMemory device_memory = mem->heaps[index];
		sht_vk_crash(driver->AllocateMemory(driver->device, &info, NULL, &device_memory));
		mem->heaps[index] = (sht_heap *)device_memory;

		mem->capacity[index] = size;
		mem->offset[index] = 0;
		mem->cpu[index] = NULL;
	}

	VkDeviceMemory device_memory = mem->heaps[SHT_MEMORY_CPU];
	sht_vk_crash(driver->MapMemory(driver->device, device_memory, 0, VK_WHOLE_SIZE, 0, &mem->cpu[SHT_MEMORY_CPU]));
	return VK_SUCCESS;
}

sht_memory_arena *sht_vk_memory_of(sht_memory *mem, sht_heap *heap)
{
	for (fckc_size_t index = 0; index < fck_arraysize(mem->objects); index++)
	{
		sht_memory_arena *object = mem->objects + index;
		VkDeviceMemory memory = sht_memory_arena_of(object, heap);
		if (memory != VK_NULL_HANDLE)
		{
			return object;
		}
	}
	return NULL;
}

sht_bool32 sht_image_is_ok(sht_image *image)
{
	return image->gpu != NULL;
}

sht_bool32 sht_buffer_is_ok(sht_buffer buffer)
{
	return buffer.gpu != NULL;
}

sht_bool32 sht_vk_image_recreate(sht_memory_arena *mem, sht_image *image, sht_extent extent, sht_image_view *views, fckc_size_t view_count)
{
	// Validate heap, but whatever!

	sht_image_configuration config = (sht_image_configuration){
		.format = image->format,
		.height = extent.height,
		.width = extent.width,
		.usage = image->usage,
		.transfer = SHT_TRANSFER_RETAINED,
	};

	sht_memory_type memory_type = SHT_MEMORY_GPU;
	if (image->cpu != NULL)
	{
		memory_type = SHT_MEMORY_CPU;
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
		sht_image_view *view = views + index;
		sht_format format = view->format;
		sht_memory_arena_image_discard(mem, view);
		*view = sht_memory_arena_image_view(mem, next, format);
	}
	return sht_true;
}

VkResult sht_vk_memory_init(sht_memory *mem, sht_vk_driver *driver, fckc_size_t size)
{
	static sht_memory_image image_api = (sht_memory_image){
		.create = sht_memory_arena_image_create,
		.destroy = sht_memory_arena_image_destroy,
		.view = sht_memory_arena_image_view,
		.discard = sht_memory_arena_image_discard,
		.is_ok = sht_image_is_ok,
		.recreate = sht_vk_image_recreate,
	};

	VkResult result;
	fck_assert(fck_arraysize(mem->objects) == 2); // NOLINT
	result = sht_vk_error(sht_memory_arena_init(&mem->objects[0], driver, size));
	sht_vk_propagate_on_error(result);
	result = sht_vk_error(sht_memory_arena_init(&mem->objects[1], driver, size));
	sht_vk_propagate_on_error(result);

	mem->bump = &mem->objects[0];
	mem->temp = &mem->objects[0];

	mem->of = sht_vk_memory_of;
	mem->malloc = sht_memory_arena_malloc;
	mem->free = sht_memory_arena_free;
	mem->reset = sht_memory_arena_reset;
	mem->is_ok = sht_buffer_is_ok;
	mem->image = &image_api;
	return result;
}