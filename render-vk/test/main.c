// #include "SDL3/SDL_vulkan.h"

#include <ApplicationServices/ApplicationServices.h>
#include <CoreVideo/CVDisplayLink.h>

#include <dlfcn.h>

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_metal.h>

#include <vulkan/utility/vk_format_utils.h>
#include <vulkan/vk_enum_string_helper.h>

#include <fck_os.h>

// #include <Metal/MTLDevice.h>
// #include <MetalKit/MTKView.h>

#include <objc/message.h>
#include <objc/runtime.h>

#include <fck_events.h>
#include <kll.h>
#include <kll_heap.h>
#include <kll_malloc.h>

#include <dlfcn.h>

#define fck_vk_sucess(vk_result) ((vk_result) == VK_SUCCESS)
#define fck_vk_failure(vk_result) (!(fck_vk_sucess(vk_result)))
#define fck_vk_failure(vk_result) (!(fck_vk_sucess(vk_result)))

typedef struct fck_vulkan
{
	VkInstance instance;

	PFN_vkEnumeratePhysicalDevices EnumeratePhysicalDevices;
	PFN_vkGetPhysicalDeviceProperties GetPhysicalDeviceProperties;
	PFN_vkGetPhysicalDeviceFeatures GetPhysicalDeviceFeatures;
	PFN_vkGetPhysicalDeviceMemoryProperties GetPhysicalDeviceMemoryProperties;
	PFN_vkCreateDevice CreateDevice;
	PFN_vkCreateMetalSurfaceEXT CreateMetalSurfaceEXT;

	// fck_vk_declare(func) macro that fill out PFN_
	PFN_vkGetPhysicalDeviceQueueFamilyProperties GetPhysicalDeviceQueueFamilyProperties;
	PFN_vkEnumerateDeviceExtensionProperties EnumerateDeviceExtensionProperties;

	PFN_vkCreateShaderModule CreateShaderModule;
} fck_vulkan;

VkResult fck_vk_report(VkResult result, const char *msg)
{
	if (fck_vk_sucess(result))
	{
		return result;
	}

	os->io->log("Error: %s - ", string_VkResult(result), msg);
	return result;
}

VkResult fck_vk_log_VkPhysicalDevice(fck_vulkan *vk, VkPhysicalDevice physicalDevice)
{
	if (!physicalDevice)
	{
		os->io->log("VkPhysicalDevice: NULL\n");
		return VK_SUCCESS;
	}

	VkPhysicalDeviceProperties props = {0};
	vk->GetPhysicalDeviceProperties(physicalDevice, &props);

	VkPhysicalDeviceFeatures feats = {0};
	vk->GetPhysicalDeviceFeatures(physicalDevice, &feats);

	VkPhysicalDeviceMemoryProperties memProps = {0};
	vk->GetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);

	uint32_t queueFamilyCount = 0;
	vk->GetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, NULL);
	VkQueueFamilyProperties *queueFamilies = NULL;
	if (queueFamilyCount > 0)
	{
		queueFamilies = (VkQueueFamilyProperties *)malloc(queueFamilyCount * sizeof(VkQueueFamilyProperties));
		if (!queueFamilies)
		{
			os->io->log("VkPhysicalDevice: FAILED TO ALLOCATE QUEUE FAMILY BUFFER\n");
			return VK_ERROR_OUT_OF_HOST_MEMORY;
		}
		vk->GetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies);
	}

	uint32_t extCount = 0;
	vk->EnumerateDeviceExtensionProperties(physicalDevice, NULL, &extCount, NULL);
	VkExtensionProperties *extensions = NULL;
	if (extCount > 0)
	{
		extensions = (VkExtensionProperties *)malloc(extCount * sizeof(VkExtensionProperties));
		if (!extensions)
		{
			os->io->log("VkPhysicalDevice: FAILED TO ALLOCATE EXTENSION BUFFER\n");
			free(queueFamilies);
			return VK_ERROR_OUT_OF_HOST_MEMORY;
		}
		vk->EnumerateDeviceExtensionProperties(physicalDevice, NULL, &extCount, extensions);
	}

	// Begin logging
	os->io->log("VkPhysicalDevice: %p\n", (void *)physicalDevice);
	os->io->log("  API Version: %u.%u.%u\n", VK_VERSION_MAJOR(props.apiVersion), VK_VERSION_MINOR(props.apiVersion),
	            VK_VERSION_PATCH(props.apiVersion));
	os->io->log("  Driver Version: %u\n", props.driverVersion);
	os->io->log("  Vendor ID: 0x%04X\n", props.vendorID);
	os->io->log("  Device ID: 0x%04X\n", props.deviceID);
	os->io->log("  Device Type: ");

	switch (props.deviceType)
	{
	case VK_PHYSICAL_DEVICE_TYPE_OTHER:
		os->io->log("OTHER");
		break;
	case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
		os->io->log("INTEGRATED_GPU");
		break;
	case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
		os->io->log("DISCRETE_GPU");
		break;
	case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
		os->io->log("VIRTUAL_GPU");
		break;
	case VK_PHYSICAL_DEVICE_TYPE_CPU:
		os->io->log("CPU");
		break;
	default:
		os->io->log("UNKNOWN (%d)", props.deviceType);
		break;
	}
	os->io->log("\n");

	os->io->log("  Device Name: %s\n", props.deviceName);
	os->io->log("  Pipeline Cache UUID: ");
	for (int i = 0; i < VK_UUID_SIZE; ++i)
		os->io->log("%02X%s", props.pipelineCacheUUID[i], i < VK_UUID_SIZE - 1 ? "-" : "\n");

	// Queue Families
	os->io->log("  Queue Families: %u\n", queueFamilyCount);
	for (uint32_t i = 0; i < queueFamilyCount; ++i)
	{
		const VkQueueFamilyProperties *q = &queueFamilies[i];
		os->io->log("    [%u] queueCount: %u, flags: 0x%X (", i, q->queueCount, q->queueFlags);
		if (q->queueFlags & VK_QUEUE_GRAPHICS_BIT)
			os->io->log("GRAPHICS ");
		if (q->queueFlags & VK_QUEUE_COMPUTE_BIT)
			os->io->log("COMPUTE ");
		if (q->queueFlags & VK_QUEUE_TRANSFER_BIT)
			os->io->log("TRANSFER ");
		if (q->queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
			os->io->log("SPARSE ");
		if (q->queueFlags & VK_QUEUE_PROTECTED_BIT)
			os->io->log("PROTECTED ");
		if (q->queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR)
			os->io->log("VIDEO_DECODE ");
		if (q->queueFlags & VK_QUEUE_VIDEO_ENCODE_BIT_KHR)
			os->io->log("VIDEO_ENCODE ");
		os->io->log("), timestampValidBits: %u\n", q->timestampValidBits);
		os->io->log("        minImageTransferGranularity: (%u, %u, %u)\n", q->minImageTransferGranularity.width,
		            q->minImageTransferGranularity.height, q->minImageTransferGranularity.depth);
	}

	// Memory Heaps
	os->io->log("  Memory Heaps: %u\n", memProps.memoryHeapCount);
	for (uint32_t i = 0; i < memProps.memoryHeapCount; ++i)
	{
		const VkMemoryHeap *heap = &memProps.memoryHeaps[i];
		os->io->log("    [%u] size: %llu bytes, flags: 0x%X (", i, (unsigned long long)heap->size, heap->flags);
		if (heap->flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
			os->io->log("DEVICE_LOCAL ");
		if (heap->flags & VK_MEMORY_HEAP_MULTI_INSTANCE_BIT)
			os->io->log("MULTI_INSTANCE ");
		os->io->log(")\n");
	}

	// Memory Types
	os->io->log("  Memory Types: %u\n", memProps.memoryTypeCount);
	for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i)
	{
		const VkMemoryType *type = &memProps.memoryTypes[i];
		os->io->log("    [%u] heapIndex: %u, propertyFlags: 0x%X (", i, type->heapIndex, type->propertyFlags);
		if (type->propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
			os->io->log("DEVICE_LOCAL ");
		if (type->propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
			os->io->log("HOST_VISIBLE ");
		if (type->propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
			os->io->log("HOST_COHERENT ");
		if (type->propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
			os->io->log("HOST_CACHED ");
		if (type->propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
			os->io->log("LAZILY_ALLOCATED ");
		if (type->propertyFlags & VK_MEMORY_PROPERTY_PROTECTED_BIT)
			os->io->log("PROTECTED ");
		if (type->propertyFlags & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD)
			os->io->log("DEVICE_COHERENT_AMD ");
		if (type->propertyFlags & VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD)
			os->io->log("DEVICE_UNCACHED_AMD ");
		if (type->propertyFlags & VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV)
			os->io->log("RDMA_CAPABLE_NV ");
		os->io->log(")\n");
	}

	// Limits (sample of key ones)
	os->io->log("  Limits:\n");
	os->io->log("    maxImageDimension1D: %u\n", props.limits.maxImageDimension1D);
	os->io->log("    maxImageDimension2D: %u\n", props.limits.maxImageDimension2D);
	os->io->log("    maxImageDimension3D: %u\n", props.limits.maxImageDimension3D);
	os->io->log("    maxImageDimensionCube: %u\n", props.limits.maxImageDimensionCube);
	os->io->log("    maxImageArrayLayers: %u\n", props.limits.maxImageArrayLayers);
	os->io->log("    maxTexelBufferElements: %u\n", props.limits.maxTexelBufferElements);
	os->io->log("    maxUniformBufferRange: %u\n", props.limits.maxUniformBufferRange);
	os->io->log("    maxStorageBufferRange: %u\n", props.limits.maxStorageBufferRange);
	os->io->log("    maxPushConstantsSize: %u\n", props.limits.maxPushConstantsSize);
	os->io->log("    maxMemoryAllocationCount: %u\n", props.limits.maxMemoryAllocationCount);
	os->io->log("    maxSamplerAllocationCount: %u\n", props.limits.maxSamplerAllocationCount);
	os->io->log("    bufferImageGranularity: %llu\n", (unsigned long long)props.limits.bufferImageGranularity);
	os->io->log("    maxPerStageDescriptorSamplers: %u\n", props.limits.maxPerStageDescriptorSamplers);
	os->io->log("    maxPerStageDescriptorUniformBuffers: %u\n", props.limits.maxPerStageDescriptorUniformBuffers);
	os->io->log("    maxPerStageDescriptorStorageBuffers: %u\n", props.limits.maxPerStageDescriptorStorageBuffers);
	os->io->log("    maxPerStageDescriptorSampledImages: %u\n", props.limits.maxPerStageDescriptorSampledImages);
	os->io->log("    maxPerStageDescriptorStorageImages: %u\n", props.limits.maxPerStageDescriptorStorageImages);
	os->io->log("    maxPerStageDescriptorInputAttachments: %u\n", props.limits.maxPerStageDescriptorInputAttachments);
	os->io->log("    maxPerStageResources: %u\n", props.limits.maxPerStageResources);
	os->io->log("    maxDescriptorSetSamplers: %u\n", props.limits.maxDescriptorSetSamplers);
	os->io->log("    maxDescriptorSetUniformBuffers: %u\n", props.limits.maxDescriptorSetUniformBuffers);
	os->io->log("    maxDescriptorSetUniformBuffersDynamic: %u\n", props.limits.maxDescriptorSetUniformBuffersDynamic);
	os->io->log("    maxDescriptorSetStorageBuffers: %u\n", props.limits.maxDescriptorSetStorageBuffers);
	os->io->log("    maxDescriptorSetStorageBuffersDynamic: %u\n", props.limits.maxDescriptorSetStorageBuffersDynamic);
	os->io->log("    maxDescriptorSetSampledImages: %u\n", props.limits.maxDescriptorSetSampledImages);
	os->io->log("    maxDescriptorSetStorageImages: %u\n", props.limits.maxDescriptorSetStorageImages);
	os->io->log("    maxDescriptorSetInputAttachments: %u\n", props.limits.maxDescriptorSetInputAttachments);
	os->io->log("    maxVertexInputAttributes: %u\n", props.limits.maxVertexInputAttributes);
	os->io->log("    maxVertexInputBindings: %u\n", props.limits.maxVertexInputBindings);
	os->io->log("    maxVertexInputAttributeOffset: %u\n", props.limits.maxVertexInputAttributeOffset);
	os->io->log("    maxVertexInputBindingStride: %u\n", props.limits.maxVertexInputBindingStride);
	os->io->log("    maxVertexOutputComponents: %u\n", props.limits.maxVertexOutputComponents);
	os->io->log("    maxTessellationGenerationLevel: %u\n", props.limits.maxTessellationGenerationLevel);
	os->io->log("    maxTessellationPatchSize: %u\n", props.limits.maxTessellationPatchSize);
	os->io->log("    maxTessellationControlPerVertexInputComponents: %u\n", props.limits.maxTessellationControlPerVertexInputComponents);
	os->io->log("    maxTessellationControlPerVertexOutputComponents: %u\n", props.limits.maxTessellationControlPerVertexOutputComponents);
	os->io->log("    maxTessellationControlPerPatchOutputComponents: %u\n", props.limits.maxTessellationControlPerPatchOutputComponents);
	os->io->log("    maxTessellationControlTotalOutputComponents: %u\n", props.limits.maxTessellationControlTotalOutputComponents);
	os->io->log("    maxTessellationEvaluationInputComponents: %u\n", props.limits.maxTessellationEvaluationInputComponents);
	os->io->log("    maxTessellationEvaluationOutputComponents: %u\n", props.limits.maxTessellationEvaluationOutputComponents);
	os->io->log("    maxGeometryShaderInvocations: %u\n", props.limits.maxGeometryShaderInvocations);
	os->io->log("    maxGeometryInputComponents: %u\n", props.limits.maxGeometryInputComponents);
	os->io->log("    maxGeometryOutputComponents: %u\n", props.limits.maxGeometryOutputComponents);
	os->io->log("    maxGeometryOutputVertices: %u\n", props.limits.maxGeometryOutputVertices);
	os->io->log("    maxGeometryTotalOutputComponents: %u\n", props.limits.maxGeometryTotalOutputComponents);
	os->io->log("    maxFragmentInputComponents: %u\n", props.limits.maxFragmentInputComponents);
	os->io->log("    maxFragmentOutputAttachments: %u\n", props.limits.maxFragmentOutputAttachments);
	os->io->log("    maxFragmentDualSrcAttachments: %u\n", props.limits.maxFragmentDualSrcAttachments);
	os->io->log("    maxFragmentCombinedOutputResources: %u\n", props.limits.maxFragmentCombinedOutputResources);
	os->io->log("    maxComputeSharedMemorySize: %u\n", props.limits.maxComputeSharedMemorySize);
	os->io->log("    maxComputeWorkGroupCount: [%u, %u, %u]\n", props.limits.maxComputeWorkGroupCount[0],
	            props.limits.maxComputeWorkGroupCount[1], props.limits.maxComputeWorkGroupCount[2]);
	os->io->log("    maxComputeWorkGroupInvocations: %u\n", props.limits.maxComputeWorkGroupInvocations);
	os->io->log("    maxComputeWorkGroupSize: [%u, %u, %u]\n", props.limits.maxComputeWorkGroupSize[0],
	            props.limits.maxComputeWorkGroupSize[1], props.limits.maxComputeWorkGroupSize[2]);
	os->io->log("    subPixelPrecisionBits: %u\n", props.limits.subPixelPrecisionBits);
	os->io->log("    subTexelPrecisionBits: %u\n", props.limits.subTexelPrecisionBits);
	os->io->log("    mipmapPrecisionBits: %u\n", props.limits.mipmapPrecisionBits);
	os->io->log("    maxDrawIndexedIndexValue: %u\n", props.limits.maxDrawIndexedIndexValue);
	os->io->log("    maxDrawIndirectCount: %u\n", props.limits.maxDrawIndirectCount);
	os->io->log("    maxSamplerLodBias: %.2f\n", props.limits.maxSamplerLodBias);
	os->io->log("    maxSamplerAnisotropy: %.2f\n", props.limits.maxSamplerAnisotropy);
	os->io->log("    maxViewports: %u\n", props.limits.maxViewports);
	os->io->log("    maxViewportDimensions: [%u, %u]\n", props.limits.maxViewportDimensions[0], props.limits.maxViewportDimensions[1]);
	os->io->log("    viewportBoundsRange: [%.2f, %.2f]\n", props.limits.viewportBoundsRange[0], props.limits.viewportBoundsRange[1]);
	os->io->log("    viewportSubPixelBits: %u\n", props.limits.viewportSubPixelBits);
	os->io->log("    minMemoryMapAlignment: %zu\n", props.limits.minMemoryMapAlignment);
	os->io->log("    minTexelBufferOffsetAlignment: %llu\n", (unsigned long long)props.limits.minTexelBufferOffsetAlignment);
	os->io->log("    minUniformBufferOffsetAlignment: %llu\n", (unsigned long long)props.limits.minUniformBufferOffsetAlignment);
	os->io->log("    minStorageBufferOffsetAlignment: %llu\n", (unsigned long long)props.limits.minStorageBufferOffsetAlignment);
	os->io->log("    minTexelOffset: %d\n", props.limits.minTexelOffset);
	os->io->log("    maxTexelOffset: %u\n", props.limits.maxTexelOffset);
	os->io->log("    minTexelGatherOffset: %d\n", props.limits.minTexelGatherOffset);
	os->io->log("    maxTexelGatherOffset: %u\n", props.limits.maxTexelGatherOffset);
	os->io->log("    minInterpolationOffset: %.6f\n", props.limits.minInterpolationOffset);
	os->io->log("    maxInterpolationOffset: %.6f\n", props.limits.maxInterpolationOffset);
	os->io->log("    subPixelInterpolationOffsetBits: %u\n", props.limits.subPixelInterpolationOffsetBits);
	os->io->log("    maxFramebufferWidth: %u\n", props.limits.maxFramebufferWidth);
	os->io->log("    maxFramebufferHeight: %u\n", props.limits.maxFramebufferHeight);
	os->io->log("    maxFramebufferLayers: %u\n", props.limits.maxFramebufferLayers);
	os->io->log("    framebufferColorSampleCounts: 0x%X\n", props.limits.framebufferColorSampleCounts);
	os->io->log("    framebufferDepthSampleCounts: 0x%X\n", props.limits.framebufferDepthSampleCounts);
	os->io->log("    framebufferStencilSampleCounts: 0x%X\n", props.limits.framebufferStencilSampleCounts);
	os->io->log("    framebufferNoAttachmentsSampleCounts: 0x%X\n", props.limits.framebufferNoAttachmentsSampleCounts);
	os->io->log("    maxColorAttachments: %u\n", props.limits.maxColorAttachments);
	os->io->log("    sampledImageColorSampleCounts: 0x%X\n", props.limits.sampledImageColorSampleCounts);
	os->io->log("    sampledImageIntegerSampleCounts: 0x%X\n", props.limits.sampledImageIntegerSampleCounts);
	os->io->log("    sampledImageDepthSampleCounts: 0x%X\n", props.limits.sampledImageDepthSampleCounts);
	os->io->log("    sampledImageStencilSampleCounts: 0x%X\n", props.limits.sampledImageStencilSampleCounts);
	os->io->log("    storageImageSampleCounts: 0x%X\n", props.limits.storageImageSampleCounts);
	os->io->log("    maxSampleMaskWords: %u\n", props.limits.maxSampleMaskWords);
	os->io->log("    timestampComputeAndGraphics: %u\n", props.limits.timestampComputeAndGraphics);
	os->io->log("    timestampPeriod: %.2f ns\n", props.limits.timestampPeriod);
	os->io->log("    maxClipDistances: %u\n", props.limits.maxClipDistances);
	os->io->log("    maxCullDistances: %u\n", props.limits.maxCullDistances);
	os->io->log("    maxCombinedClipAndCullDistances: %u\n", props.limits.maxCombinedClipAndCullDistances);
	os->io->log("    discreteQueuePriorities: %u\n", props.limits.discreteQueuePriorities);
	os->io->log("    pointSizeRange: [%.2f, %.2f]\n", props.limits.pointSizeRange[0], props.limits.pointSizeRange[1]);
	os->io->log("    lineWidthRange: [%.2f, %.2f]\n", props.limits.lineWidthRange[0], props.limits.lineWidthRange[1]);
	os->io->log("    pointSizeGranularity: %.2f\n", props.limits.pointSizeGranularity);
	os->io->log("    lineWidthGranularity: %.2f\n", props.limits.lineWidthGranularity);
	os->io->log("    strictLines: %u\n", props.limits.strictLines);
	os->io->log("    standardSampleLocations: %u\n", props.limits.standardSampleLocations);
	os->io->log("    optimalBufferCopyOffsetAlignment: %llu\n", (unsigned long long)props.limits.optimalBufferCopyOffsetAlignment);
	os->io->log("    optimalBufferCopyRowPitchAlignment: %llu\n", (unsigned long long)props.limits.optimalBufferCopyRowPitchAlignment);
	os->io->log("    nonCoherentAtomSize: %llu\n", (unsigned long long)props.limits.nonCoherentAtomSize);

	// Sparse Properties
	os->io->log("  Sparse Properties:\n");
	os->io->log("    residencyStandard2DBlockShape: %u\n", props.sparseProperties.residencyStandard2DBlockShape);
	os->io->log("    residencyStandard2DMultisampleBlockShape: %u\n", props.sparseProperties.residencyStandard2DMultisampleBlockShape);
	os->io->log("    residencyStandard3DBlockShape: %u\n", props.sparseProperties.residencyStandard3DBlockShape);
	os->io->log("    residencyAlignedMipSize: %u\n", props.sparseProperties.residencyAlignedMipSize);
	os->io->log("    residencyNonResidentStrict: %u\n", props.sparseProperties.residencyNonResidentStrict);

	// Features
	os->io->log("  Features:\n");
#define LOG_FEATURE(feat) os->io->log("    " #feat ": %u\n", feats.feat)
	LOG_FEATURE(robustBufferAccess);
	LOG_FEATURE(fullDrawIndexUint32);
	LOG_FEATURE(imageCubeArray);
	LOG_FEATURE(independentBlend);
	LOG_FEATURE(geometryShader);
	LOG_FEATURE(tessellationShader);
	LOG_FEATURE(sampleRateShading);
	LOG_FEATURE(dualSrcBlend);
	LOG_FEATURE(logicOp);
	LOG_FEATURE(multiDrawIndirect);
	LOG_FEATURE(drawIndirectFirstInstance);
	LOG_FEATURE(depthClamp);
	LOG_FEATURE(depthBiasClamp);
	LOG_FEATURE(fillModeNonSolid);
	LOG_FEATURE(depthBounds);
	LOG_FEATURE(wideLines);
	LOG_FEATURE(largePoints);
	LOG_FEATURE(alphaToOne);
	LOG_FEATURE(multiViewport);
	LOG_FEATURE(samplerAnisotropy);
	LOG_FEATURE(textureCompressionETC2);
	LOG_FEATURE(textureCompressionASTC_LDR);
	LOG_FEATURE(textureCompressionBC);
	LOG_FEATURE(occlusionQueryPrecise);
	LOG_FEATURE(pipelineStatisticsQuery);
	LOG_FEATURE(vertexPipelineStoresAndAtomics);
	LOG_FEATURE(fragmentStoresAndAtomics);
	LOG_FEATURE(shaderTessellationAndGeometryPointSize);
	LOG_FEATURE(shaderImageGatherExtended);
	LOG_FEATURE(shaderStorageImageExtendedFormats);
	LOG_FEATURE(shaderStorageImageMultisample);
	LOG_FEATURE(shaderStorageImageReadWithoutFormat);
	LOG_FEATURE(shaderStorageImageWriteWithoutFormat);
	LOG_FEATURE(shaderUniformBufferArrayDynamicIndexing);
	LOG_FEATURE(shaderSampledImageArrayDynamicIndexing);
	LOG_FEATURE(shaderStorageBufferArrayDynamicIndexing);
	LOG_FEATURE(shaderStorageImageArrayDynamicIndexing);
	LOG_FEATURE(shaderClipDistance);
	LOG_FEATURE(shaderCullDistance);
	LOG_FEATURE(shaderFloat64);
	LOG_FEATURE(shaderInt64);
	LOG_FEATURE(shaderInt16);
	LOG_FEATURE(shaderResourceResidency);
	LOG_FEATURE(shaderResourceMinLod);
	LOG_FEATURE(sparseBinding);
	LOG_FEATURE(sparseResidencyBuffer);
	LOG_FEATURE(sparseResidencyImage2D);
	LOG_FEATURE(sparseResidencyImage3D);
	LOG_FEATURE(sparseResidency2Samples);
	LOG_FEATURE(sparseResidency4Samples);
	LOG_FEATURE(sparseResidency8Samples);
	LOG_FEATURE(sparseResidency16Samples);
	LOG_FEATURE(sparseResidencyAliased);
	LOG_FEATURE(variableMultisampleRate);
	LOG_FEATURE(inheritedQueries);
#undef LOG_FEATURE

	// Extensions
	os->io->log("  Extensions: %u\n", extCount);
	for (uint32_t i = 0; i < extCount; ++i)
	{
		os->io->log("    [%u] %s (specVersion: %u)\n", i, extensions[i].extensionName, extensions[i].specVersion);
	}

	// Cleanup
	free(queueFamilies);
	free(extensions);

	return VK_SUCCESS;
}

#define fck_vk_error(vk_result) fck_vk_failure(fck_vk_report((vk_result), (__func__)))

#ifdef __arm64__
/* ARM just uses objc_msgSend */
#define abi_objc_msgSend_stret objc_msgSend
#define abi_objc_msgSend_fpret objc_msgSend
#else /* __i386__ */
/* x86 just uses abi_objc_msgSend_fpret and (NSColor *)objc_msgSend_id respectively */
#define abi_objc_msgSend_stret objc_msgSend_stret
#define abi_objc_msgSend_fpret objc_msgSend_fpret
#endif

typedef CGRect NSRect;
typedef CGPoint NSPoint;
typedef CGSize NSSize;

typedef void NSApplication;

typedef void NSObject;
typedef void NSEvent;
typedef void NSString;
typedef void NSWindow;
typedef void NSView;
typedef void MTKView;
typedef void MTLDevice;

extern MTLDevice *MTLCreateSystemDefaultDevice();

typedef unsigned long NSUInteger;
typedef long NSInteger;
typedef unsigned short NSUShort;

#define NS_ENUM(type, name)                                                                                                                \
	type name;                                                                                                                             \
	enum

typedef NS_ENUM(NSUInteger, NSWindowStyleMask) {
	NSWindowStyleMaskBorderless = 0,
	NSWindowStyleMaskTitled = 1 << 0,
	NSWindowStyleMaskClosable = 1 << 1,
	NSWindowStyleMaskMiniaturizable = 1 << 2,
	NSWindowStyleMaskResizable = 1 << 3,
	NSWindowStyleMaskTexturedBackground = 1 << 8, /* deprecated */
	NSWindowStyleMaskUnifiedTitleAndToolbar = 1 << 12,
	NSWindowStyleMaskFullScreen = 1 << 14,
	NSWindowStyleMaskFullSizeContentView = 1 << 15,
	NSWindowStyleMaskUtilityWindow = 1 << 4,
	NSWindowStyleMaskDocModalWindow = 1 << 6,
	NSWindowStyleMaskNonactivatingPanel = 1 << 7,
	NSWindowStyleMaskHUDWindow = 1 << 13
};

typedef NS_ENUM(NSUInteger, NSBackingStoreType) {
	NSBackingStoreRetained = 0,
	NSBackingStoreNonretained = 1,
	NSBackingStoreBuffered = 2
};

typedef NS_ENUM(NSUInteger, NSEventType) { /* various types of events */
	NSEventTypeLeftMouseDown = 1,
	NSEventTypeLeftMouseUp = 2,
	NSEventTypeRightMouseDown = 3,
	NSEventTypeRightMouseUp = 4,
	NSEventTypeMouseMoved = 5,
	NSEventTypeLeftMouseDragged = 6,
	NSEventTypeRightMouseDragged = 7,
	NSEventTypeMouseEntered = 8,
	NSEventTypeMouseExited = 9,
	NSEventTypeKeyDown = 10,
	NSEventTypeKeyUp = 11,
	NSEventTypeFlagsChanged = 12,
	NSEventTypeAppKitDefined = 13,
	NSEventTypeSystemDefined = 14,
	NSEventTypeApplicationDefined = 15,
	NSEventTypePeriodic = 16,
	NSEventTypeCursorUpdate = 17,
	NSEventTypeScrollWheel = 22,
	NSEventTypeTabletPoint = 23,
	NSEventTypeTabletProximity = 24,
	NSEventTypeOtherMouseDown = 25,
	NSEventTypeOtherMouseUp = 26,
	NSEventTypeOtherMouseDragged = 27,
	/* The following event types are available on some hardware on 10.5.2 and later */
	NSEventTypeGesture API_AVAILABLE(macos(10.5)) = 29,
	NSEventTypeMagnify API_AVAILABLE(macos(10.5)) = 30,
	NSEventTypeSwipe API_AVAILABLE(macos(10.5)) = 31,
	NSEventTypeRotate API_AVAILABLE(macos(10.5)) = 18,
	NSEventTypeBeginGesture API_AVAILABLE(macos(10.5)) = 19,
	NSEventTypeEndGesture API_AVAILABLE(macos(10.5)) = 20,

	NSEventTypeSmartMagnify API_AVAILABLE(macos(10.8)) = 32,
	NSEventTypeQuickLook API_AVAILABLE(macos(10.8)) = 33,

	NSEventTypePressure API_AVAILABLE(macos(10.10.3)) = 34,
	NSEventTypeDirectTouch API_AVAILABLE(macos(10.10)) = 37,

	NSEventTypeChangeMode API_AVAILABLE(macos(10.15)) = 38,
};

typedef NS_ENUM(unsigned long long, NSEventMask) { /* masks for the types of events */
	NSEventMaskLeftMouseDown = 1ULL << NSEventTypeLeftMouseDown,
	NSEventMaskLeftMouseUp = 1ULL << NSEventTypeLeftMouseUp,
	NSEventMaskRightMouseDown = 1ULL << NSEventTypeRightMouseDown,
	NSEventMaskRightMouseUp = 1ULL << NSEventTypeRightMouseUp,
	NSEventMaskMouseMoved = 1ULL << NSEventTypeMouseMoved,
	NSEventMaskLeftMouseDragged = 1ULL << NSEventTypeLeftMouseDragged,
	NSEventMaskRightMouseDragged = 1ULL << NSEventTypeRightMouseDragged,
	NSEventMaskMouseEntered = 1ULL << NSEventTypeMouseEntered,
	NSEventMaskMouseExited = 1ULL << NSEventTypeMouseExited,
	NSEventMaskKeyDown = 1ULL << NSEventTypeKeyDown,
	NSEventMaskKeyUp = 1ULL << NSEventTypeKeyUp,
	NSEventMaskFlagsChanged = 1ULL << NSEventTypeFlagsChanged,
	NSEventMaskAppKitDefined = 1ULL << NSEventTypeAppKitDefined,
	NSEventMaskSystemDefined = 1ULL << NSEventTypeSystemDefined,
	NSEventMaskApplicationDefined = 1ULL << NSEventTypeApplicationDefined,
	NSEventMaskPeriodic = 1ULL << NSEventTypePeriodic,
	NSEventMaskCursorUpdate = 1ULL << NSEventTypeCursorUpdate,
	NSEventMaskScrollWheel = 1ULL << NSEventTypeScrollWheel,
	NSEventMaskTabletPoint = 1ULL << NSEventTypeTabletPoint,
	NSEventMaskTabletProximity = 1ULL << NSEventTypeTabletProximity,
	NSEventMaskOtherMouseDown = 1ULL << NSEventTypeOtherMouseDown,
	NSEventMaskOtherMouseUp = 1ULL << NSEventTypeOtherMouseUp,
	NSEventMaskOtherMouseDragged = 1ULL << NSEventTypeOtherMouseDragged,
};
/* The following event masks are available on some hardware on 10.5.2 and later */
#define NSEventMaskGesture API_AVAILABLE(macos(10.5))(1ULL << NSEventTypeGesture)
#define NSEventMaskMagnify API_AVAILABLE(macos(10.5))(1ULL << NSEventTypeMagnify)
#define NSEventMaskSwipe API_AVAILABLE(macos(10.5))(1ULL << NSEventTypeSwipe)
#define NSEventMaskRotate API_AVAILABLE(macos(10.5))(1ULL << NSEventTypeRotate)
#define NSEventMaskBeginGesture API_AVAILABLE(macos(10.5))(1ULL << NSEventTypeBeginGesture)
#define NSEventMaskEndGesture API_AVAILABLE(macos(10.5))(1ULL << NSEventTypeEndGesture)

/* Note: You can only use these event masks on 64 bit. In other words, you cannot setup a local, nor global, event monitor for these event
 * types on 32 bit. Also, you cannot search the event queue for them (nextEventMatchingMask:...) on 32 bit. */
#define NSEventMaskSmartMagnify API_AVAILABLE(macos(10.8))(1ULL << NSEventTypeSmartMagnify)
#define NSEventMaskPressure API_AVAILABLE(macos(10.10.3))(1ULL << NSEventTypePressure)
#define NSEventMaskDirectTouch API_AVAILABLE(macos(10.12.2))(1ULL << NSEventTypeDirectTouch)
#define NSEventMaskChangeMode API_AVAILABLE(macos(10.15))(1ULL << NSEventTypeChangeMode)
#define NSEventMaskAny NSUIntegerMax

typedef NS_ENUM(NSUInteger, NSEventModifierFlags) {
	NSEventModifierFlagCapsLock = 1 << 16,   // Set if Caps Lock key is pressed.
	NSEventModifierFlagShift = 1 << 17,      // Set if Shift key is pressed.
	NSEventModifierFlagControl = 1 << 18,    // Set if Control key is pressed.
	NSEventModifierFlagOption = 1 << 19,     // Set if Option or Alternate key is pressed.
	NSEventModifierFlagCommand = 1 << 20,    // Set if Command key is pressed.
	NSEventModifierFlagNumericPad = 1 << 21, // Set if any key in the numeric keypad is pressed.
	NSEventModifierFlagHelp = 1 << 22,       // Set if the Help key is pressed.
	NSEventModifierFlagFunction = 1 << 23,   // Set if any function key is pressed.
};

#define objc_msgSend_address ((void *(*)(id, SEL))objc_msgSend)
#define objc_msgSend_address_string ((void *(*)(id, SEL, NSString *))objc_msgSend)
#define objc_msgSend_string ((NSString * (*)(id, SEL)) objc_msgSend)
#define objc_msgSend_u32 ((fckc_u32(*)(id, SEL))objc_msgSend)

#define objc_msgSend_id ((id(*)(id, SEL))objc_msgSend)
#define objc_msgSend_id_id ((id(*)(id, SEL, id))objc_msgSend)
#define objc_msgSend_id_rect ((id(*)(id, SEL, NSRect))objc_msgSend)
#define objc_msgSend_uint ((NSUInteger(*)(id, SEL))objc_msgSend)
#define objc_msgSend_int ((NSInteger(*)(id, SEL))objc_msgSend)
#define objc_msgSend_SEL ((SEL(*)(id, SEL))objc_msgSend)
#define objc_msgSend_float ((CGFloat(*)(id, SEL))abi_objc_msgSend_fpret)
#define objc_msgSend_bool ((BOOL(*)(id, SEL))objc_msgSend)
#define objc_msgSend_void ((void (*)(id, SEL))objc_msgSend)
#define objc_msgSend_double ((double (*)(id, SEL))objc_msgSend)
#define objc_msgSend_void_id ((void (*)(id, SEL, id))objc_msgSend)
#define objc_msgSend_void_uint ((void (*)(id, SEL, NSUInteger))objc_msgSend)
#define objc_msgSend_void_int ((void (*)(id, SEL, NSInteger))objc_msgSend)
#define objc_msgSend_void_bool ((void (*)(id, SEL, BOOL))objc_msgSend)
#define objc_msgSend_void_float ((void (*)(id, SEL, CGFloat))objc_msgSend)
#define objc_msgSend_void_double ((void (*)(id, SEL, double))objc_msgSend)
#define objc_msgSend_void_SEL ((void (*)(id, SEL, SEL))objc_msgSend)
#define objc_msgSend_id_char_const ((id(*)(id, SEL, char const *))objc_msgSend)

#define objc_msgSend_ushort ((NSUShort(*)(id, SEL))objc_msgSend)

typedef enum NSApplicationActivationPolicy
{
	NSApplicationActivationPolicyRegular,
	NSApplicationActivationPolicyAccessory,
	NSApplicationActivationPolicyProhibited
} NSApplicationActivationPolicy;

#define NSAlloc(nsclass) objc_msgSend_id((id)(nsclass), sel_registerName("alloc"))
#define NSRelease(nsclass) objc_msgSend_id((id)(nsclass), sel_registerName("release"))

#define name(target_type) sizeof(target_type) ? #target_type : NULL

bool running = true;

unsigned int windowShouldClose(void *self)
{
	NSWindow *win = NULL;
	object_getInstanceVariable(self, name(NSWindow), (void *)&win);
	if (win == NULL)
		return true;

	running = false;

	return true;
}

NSSize windowResize(void *self, SEL sel, NSSize frameSize)
{
	NSWindow *win = NULL;
	object_getInstanceVariable(self, name(NSWindow), (void **)&win);
	if (win == NULL)
		return frameSize;

	printf("window resized to %f %f\n", frameSize.width, frameSize.height);
	return frameSize;
}

// This is optional, but we’ll need it so the view accepts first responder
BOOL myAcceptsFirstResponder(void *self, SEL _cmd)
{
	return YES;
}

#include <string.h>

const char *NSEventTypeToChar(NSEventType eventType);
const char *NSEventModifierFlagsToChar(NSEventModifierFlags modifierFlags);

// typedef struct AppContext
//{
//	NSApplication *NSApp;
// } AppContext;

typedef enum fck_macos_result
{
	FCK_MACOS_RESULT_CONTINUE,
	FCK_MACOS_RESULT_DONE,
} fck_macos_result;

typedef struct fck_macos_application
{
	NSApplication *ns_app;
	fck_pkey pkeys[255];
} fck_macos_application;

#include <dlfcn.h>
#include <fck_os.h>

void fck_macos_create_pkey_mapping(fckc_size_t size, fck_pkey keys[size])
{
	// Shamelessly stolen from SDL... Fuck that shit, seriously
	// Give me some fucking constants
	keys[0] = FCK_PKEY_A;
	keys[1] = FCK_PKEY_S;
	keys[2] = FCK_PKEY_D;
	keys[3] = FCK_PKEY_F;
	keys[4] = FCK_PKEY_H;
	keys[5] = FCK_PKEY_G;
	keys[6] = FCK_PKEY_Z;
	keys[7] = FCK_PKEY_X;
	keys[8] = FCK_PKEY_C;
	keys[9] = FCK_PKEY_V;
	keys[10] = FCK_PKEY_NONUSBACKSLASH;
	keys[11] = FCK_PKEY_B;
	keys[12] = FCK_PKEY_Q;
	keys[13] = FCK_PKEY_W;
	keys[14] = FCK_PKEY_E;
	keys[15] = FCK_PKEY_R;
	keys[16] = FCK_PKEY_Y;
	keys[17] = FCK_PKEY_T;
	keys[18] = FCK_PKEY_1;
	keys[19] = FCK_PKEY_2;
	keys[20] = FCK_PKEY_3;
	keys[21] = FCK_PKEY_4;
	keys[22] = FCK_PKEY_6;
	keys[23] = FCK_PKEY_5;
	keys[24] = FCK_PKEY_EQUALS;
	keys[25] = FCK_PKEY_9;
	keys[26] = FCK_PKEY_7;
	keys[27] = FCK_PKEY_MINUS;
	keys[28] = FCK_PKEY_8;
	keys[29] = FCK_PKEY_0;
	keys[30] = FCK_PKEY_RIGHTBRACKET;
	keys[31] = FCK_PKEY_O;
	keys[32] = FCK_PKEY_U;
	keys[33] = FCK_PKEY_LEFTBRACKET;
	keys[34] = FCK_PKEY_I;
	keys[35] = FCK_PKEY_P;
	keys[36] = FCK_PKEY_RETURN;
	keys[37] = FCK_PKEY_L;
	keys[38] = FCK_PKEY_J;
	keys[39] = FCK_PKEY_APOSTROPHE;
	keys[40] = FCK_PKEY_K;
	keys[41] = FCK_PKEY_SEMICOLON;
	keys[42] = FCK_PKEY_BACKSLASH;
	keys[43] = FCK_PKEY_COMMA;
	keys[44] = FCK_PKEY_SLASH;
	keys[45] = FCK_PKEY_N;
	keys[46] = FCK_PKEY_M;
	keys[47] = FCK_PKEY_PERIOD;
	keys[48] = FCK_PKEY_TAB;
	keys[49] = FCK_PKEY_SPACE;
	keys[50] = FCK_PKEY_GRAVE;
	keys[51] = FCK_PKEY_BACKSPACE;
	keys[52] = FCK_PKEY_KP_ENTER; // keyboard enter on portables
	keys[53] = FCK_PKEY_ESCAPE;
	keys[54] = FCK_PKEY_RGUI;
	keys[55] = FCK_PKEY_LGUI;
	keys[56] = FCK_PKEY_LSHIFT;
	keys[57] = FCK_PKEY_CAPSLOCK;
	keys[58] = FCK_PKEY_LALT;
	keys[59] = FCK_PKEY_LCTRL;
	keys[60] = FCK_PKEY_RSHIFT;
	keys[61] = FCK_PKEY_RALT;
	keys[62] = FCK_PKEY_RCTRL;
	keys[63] = FCK_PKEY_RGUI;
	keys[64] = FCK_PKEY_F17;
	keys[65] = FCK_PKEY_KP_PERIOD;
	keys[66] = FCK_PKEY_UNKNOWN; // unknown (unused?)
	keys[67] = FCK_PKEY_KP_MULTIPLY;
	keys[68] = FCK_PKEY_UNKNOWN; // unknown (unused?)
	keys[69] = FCK_PKEY_KP_PLUS;
	keys[70] = FCK_PKEY_UNKNOWN; // unknown (unused?)
	keys[71] = FCK_PKEY_NUMLOCKCLEAR;
	keys[72] = FCK_PKEY_VOLUMEUP;
	keys[73] = FCK_PKEY_VOLUMEDOWN;
	keys[74] = FCK_PKEY_MUTE;
	keys[75] = FCK_PKEY_KP_DIVIDE;
	keys[76] = FCK_PKEY_KP_ENTER; // keypad enter on external keyboards, fn-return on portables
	keys[77] = FCK_PKEY_UNKNOWN;  // unknown (unused?)
	keys[78] = FCK_PKEY_KP_MINUS;
	keys[79] = FCK_PKEY_F18;
	keys[80] = FCK_PKEY_F19;
	keys[81] = FCK_PKEY_KP_EQUALS;
	keys[82] = FCK_PKEY_KP_0;
	keys[83] = FCK_PKEY_KP_1;
	keys[84] = FCK_PKEY_KP_2;
	keys[85] = FCK_PKEY_KP_3;
	keys[86] = FCK_PKEY_KP_4;
	keys[87] = FCK_PKEY_KP_5;
	keys[88] = FCK_PKEY_KP_6;
	keys[89] = FCK_PKEY_KP_7;
	keys[90] = FCK_PKEY_UNKNOWN; // unknown (unused?)
	keys[91] = FCK_PKEY_KP_8;
	keys[92] = FCK_PKEY_KP_9;
	keys[93] = FCK_PKEY_INTERNATIONAL3; // Cosmo_USB2ADB.c says "Yen (JIS)"
	keys[94] = FCK_PKEY_INTERNATIONAL1; // Cosmo_USB2ADB.c says "Ro (JIS)"
	keys[95] = FCK_PKEY_KP_COMMA;       // Cosmo_USB2ADB.c says ", JIS only"
	keys[96] = FCK_PKEY_F5;
	keys[97] = FCK_PKEY_F6;
	keys[98] = FCK_PKEY_F7;
	keys[99] = FCK_PKEY_F3;
	keys[100] = FCK_PKEY_F8;
	keys[101] = FCK_PKEY_F9;
	keys[102] = FCK_PKEY_LANG2; // Cosmo_USB2ADB.c says "Eisu"
	keys[103] = FCK_PKEY_F11;
	keys[104] = FCK_PKEY_LANG1; // Cosmo_USB2ADB.c says "Kana"
	keys[105] = FCK_PKEY_PRINTSCREEN;
	keys[106] = FCK_PKEY_F16;
	keys[107] = FCK_PKEY_SCROLLLOCK; // F14/scroll lock, see comment about F13/print screen above
	keys[108] = FCK_PKEY_UNKNOWN;    // unknown (unused?)
	keys[109] = FCK_PKEY_F10;
	keys[110] = FCK_PKEY_APPLICATION; // windows contextual menu key, fn-enter on portables
	keys[111] = FCK_PKEY_F12;
	keys[112] = FCK_PKEY_UNKNOWN; // unknown (unused?)
	keys[113] = FCK_PKEY_PAUSE;   // F15/pause, see comment about F13/print screen above
	keys[114] = FCK_PKEY_INSERT;
	keys[115] = FCK_PKEY_HOME;
	keys[116] = FCK_PKEY_PAGEUP;
	keys[117] = FCK_PKEY_DELETE;
	keys[118] = FCK_PKEY_F4;
	keys[119] = FCK_PKEY_END;
	keys[120] = FCK_PKEY_F2;
	keys[121] = FCK_PKEY_PAGEDOWN;
	keys[122] = FCK_PKEY_F1;
	keys[123] = FCK_PKEY_LEFT;
	keys[124] = FCK_PKEY_RIGHT;
	keys[125] = FCK_PKEY_DOWN;
	keys[126] = FCK_PKEY_UP;
	keys[127] = FCK_PKEY_POWER;
}

VkPhysicalDevice fck_vk_physical_device_by_name(fck_vulkan *vk, const char *target)
{
	VkPhysicalDevice phy_devices[16]; // There is no fucking way...
	fckc_u32 phy_device_count = fck_arraysize(phy_devices);
	if (fck_vk_error(vk->EnumeratePhysicalDevices(vk->instance, &phy_device_count, phy_devices)))
	{
	}
	// FIND BY NAME:
	for (fckc_u32 index = 0; index < phy_device_count; index++)
	{
		VkPhysicalDevice physical_device = phy_devices[index];
		VkPhysicalDeviceProperties props = {0};
		vk->GetPhysicalDeviceProperties(physical_device, &props);
		if (os->str->unsafe->cmp(props.deviceName, target) == 0)
		{
			return physical_device;
		}
	}
	return VK_NULL_HANDLE;
}

fck_macos_result fck_macos_app_init(void **appState, int argc, char **argv)
{
	fck_macos_application *macos_app = (fck_macos_application *)kll_malloc(kll_heap, sizeof(*macos_app));
	fck_macos_create_pkey_mapping(fck_arraysize(macos_app->pkeys), macos_app->pkeys);

	class_addMethod(objc_getClass(name(NSObject)), sel_registerName("windowShouldClose:"), (IMP)windowShouldClose, 0);
	macos_app->ns_app = objc_msgSend_id((id)objc_getClass("NSApplication"), sel_registerName("sharedApplication"));
	objc_msgSend_void_int(macos_app->ns_app, sel_registerName("setActivationPolicy:"), NSApplicationActivationPolicyRegular);

	NSBackingStoreType macArgs = NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSBackingStoreBuffered |
	                             NSWindowStyleMaskTitled | NSWindowStyleMaskResizable;

	SEL initWithContentRect = sel_registerName("initWithContentRect:styleMask:backing:defer:");
	Class windowClass = objc_getClass(name(NSWindow));
	NSWindow *window = NSAlloc(windowClass);
	NSRect viewRect = (NSRect){{200, 200}, {200, 200}};
	typedef id (*window_init_with_content_rect)(id, SEL, NSRect, NSWindowStyleMask, NSBackingStoreType, bool);
	window = ((window_init_with_content_rect)objc_msgSend)(window, initWithContentRect, viewRect, macArgs, macArgs, false);

	SEL initWithFrame = sel_registerName("initWithFrame:device:");
	MTKView *view = NSAlloc(objc_getClass(name(MTKView)));
	MTLDevice *mtl_device = MTLCreateSystemDefaultDevice();
	typedef MTKView *(*mtk_view_init_with_frame)(MTKView *, SEL, NSRect, MTLDevice *);
	view = ((mtk_view_init_with_frame)objc_msgSend)(view, initWithFrame, viewRect, mtl_device);

	// cStringUsingEncoding:
	// name
	Class delegateClass = objc_allocateClassPair(objc_getClass(name(NSObject)), "WindowDelegate", 0);
	class_addIvar(delegateClass, name(NSWindow), sizeof(NSWindow *), rint(log2(sizeof(NSWindow *))), "L");

	SEL windowWillResize = sel_registerName("windowWillResize:toSize:");
	BOOL addResize = class_addMethod(delegateClass, windowWillResize, (IMP)windowResize, "{NSSize=ff}@:{NSSize=ff}");
	BOOL addFirstResponder = class_addMethod(delegateClass, sel_registerName("acceptsFirstResponder"), (IMP)myAcceptsFirstResponder, "v@:");

	id delegate = objc_msgSend_id(NSAlloc(delegateClass), sel_registerName("init"));

	object_setInstanceVariable(delegate, name(NSWindow), window);

	objc_msgSend_void_id(window, sel_registerName("setDelegate:"), delegate);

	objc_msgSend_void_bool(macos_app->ns_app, sel_registerName("activateIgnoringOtherApps:"), true);
	((id(*)(id, SEL, SEL))objc_msgSend)(window, sel_registerName("makeKeyAndOrderFront:"), NULL);
	objc_msgSend_void_bool(window, sel_registerName("setIsVisible:"), true);

	objc_msgSend_void_id(window, sel_registerName("setContentView:"), view);

	CAMetalLayer *layer = (CAMetalLayer *)objc_msgSend_id(view, sel_registerName("layer"));

	*appState = macos_app;

	VkInstanceCreateInfo instance_create_info;
	const char *instance_extension_names[16];
	fckc_size_t instance_extension_count = 0;
	static const char *layer_names[] = {"VK_LAYER_KHRONOS_validation"};
	VkApplicationInfo appInfo;

	instance_create_info.flags = 0;

	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pNext = NULL;
	appInfo.pApplicationName = NULL;
	appInfo.applicationVersion = 0;
	appInfo.pEngineName = "fck-vk";
	appInfo.engineVersion = 0;
	appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);

	instance_extension_names[instance_extension_count++] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
	instance_create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

	instance_extension_names[instance_extension_count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

	instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_create_info.pNext = NULL;
	instance_create_info.pApplicationInfo = &appInfo;
	instance_create_info.ppEnabledLayerNames = layer_names;
	instance_create_info.enabledExtensionCount = instance_extension_count;
	instance_create_info.ppEnabledExtensionNames = instance_extension_names;
	instance_create_info.enabledLayerCount = 0;

	VkMetalSurfaceCreateInfoEXT metal_create_info = (VkMetalSurfaceCreateInfoEXT){
		.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT,
		.pLayer = layer, //
		.flags = 0,
		.pNext = NULL,
	};

	// PFN_vkGetInstanceProcAddr instance_get_proc_addr = (PFN_vkGetInstanceProcAddr)dlsym(RTLD_DEFAULT, "vkGetInstanceProcAddr");

	VkInstance instance;
	PFN_vkCreateInstance create_instance = (PFN_vkCreateInstance)dlsym(RTLD_DEFAULT, "vkCreateInstance");
	if (fck_vk_error(create_instance(&instance_create_info, NULL, &instance)))
	{
		return FCK_MACOS_RESULT_DONE;
	}

	fck_vulkan vk;
	// Vulkan API is now ready to get setup!
#define fck_vk_load_function(api_namespace, api_member)                                                                                    \
	(api_namespace).api_member = (PFN_vk##api_member)dlsym(RTLD_DEFAULT, "vk" #api_member)

	vk.instance = instance;
	fck_vk_load_function(vk, EnumeratePhysicalDevices);
	fck_vk_load_function(vk, GetPhysicalDeviceProperties);
	fck_vk_load_function(vk, GetPhysicalDeviceFeatures);
	fck_vk_load_function(vk, GetPhysicalDeviceMemoryProperties);
	fck_vk_load_function(vk, CreateDevice);
	fck_vk_load_function(vk, CreateMetalSurfaceEXT);
	fck_vk_load_function(vk, GetPhysicalDeviceQueueFamilyProperties);
	fck_vk_load_function(vk, EnumerateDeviceExtensionProperties);
	fck_vk_load_function(vk, CreateShaderModule);
	// This shit in between here has to come from OUTSIDE the render api
	// since we have no control over a window! :)
	// Or maybe it doesn ot? We know we are on macos...
	VkSurfaceKHR surface;
	if (fck_vk_error(vk.CreateMetalSurfaceEXT(instance, &metal_create_info, NULL, &surface)))
	{
		return FCK_MACOS_RESULT_DONE;
	}
	// Vulkan API is now ready and setup!

	// VkDevice
	os->io->log("Surface creation: %s", "SUCCESS");

	NSString *mtl_device_name = objc_msgSend_string(mtl_device, sel_registerName("name"));
	const char *mtl_device_cstring = (const char *)objc_msgSend_address(mtl_device_name, sel_registerName("UTF8String"));
	os->io->log("MTLDevice Name: %s", mtl_device_cstring);

	VkPhysicalDevice physical_device = fck_vk_physical_device_by_name(&vk, mtl_device_cstring);
	if (physical_device == NULL)
	{
		os->io->log("Could not find MTLDevice: %s", mtl_device_cstring);
		return FCK_MACOS_RESULT_DONE;
	}
	os->io->log("Found shared MTL-VK Device: %s", mtl_device_cstring);

	//
	VkPhysicalDeviceProperties properties;
	VkPhysicalDeviceFeatures features;
	VkPhysicalDeviceMemoryProperties memory_properties;
	vk.GetPhysicalDeviceProperties(physical_device, &properties);
	vk.GetPhysicalDeviceFeatures(physical_device, &features);
	vk.GetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

	VkDeviceCreateInfo device_create_info = {0};
	VkDevice device;
	fck_vk_error(vk.CreateDevice(physical_device, &device_create_info, NULL, &device));

	// vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator,
	//                VkDevice *pDevice);

	VkRenderPassCreateInfo render_pass_create_info = (VkRenderPassCreateInfo){
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
	};

	typedef enum fck_standard_stage
	{
		FCK_STANDARD_STAGE_VERTEX,
		FCK_STANDARD_STAGE_FRAGMENT,
		FCK_STANDARD_STAGE_COUNT,
	} fck_standard_stage;

	VkShaderModuleCreateInfo shader_info[FCK_STANDARD_STAGE_COUNT] = {
		[FCK_STANDARD_STAGE_VERTEX] =
			{
				.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
				.codeSize = 0,
				.pCode = NULL,
			},
		[FCK_STANDARD_STAGE_FRAGMENT] =
			{
				.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
				.codeSize = 0,
				.pCode = NULL,
			},
	};

	VkShaderModule shaders[FCK_STANDARD_STAGE_COUNT];
	fck_vk_error(vk.CreateShaderModule(device, &shader_info[FCK_STANDARD_STAGE_VERTEX], NULL, &shaders[FCK_STANDARD_STAGE_VERTEX]));
	fck_vk_error(vk.CreateShaderModule(device, &shader_info[FCK_STANDARD_STAGE_FRAGMENT], NULL, &shaders[FCK_STANDARD_STAGE_FRAGMENT]));

	// vkCreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator,
	// VkShaderModule *pShaderModule)
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

	VkPipelineVertexInputStateCreateInfo vertexInputState;
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState;
	VkPipelineTessellationStateCreateInfo tessellationState;
	VkPipelineViewportStateCreateInfo viewportState;
	VkPipelineRasterizationStateCreateInfo rasterizationState;
	VkPipelineMultisampleStateCreateInfo multisampleState;
	VkPipelineDepthStencilStateCreateInfo depthStencilState;
	VkPipelineColorBlendStateCreateInfo colorBlendState;
	VkPipelineDynamicStateCreateInfo dynamicState;

	// VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = (VkGraphicsPipelineCreateInfo){
	//	.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
	//	.pNext = NULL,
	//	.flags = {},
	//	.stageCount = {},
	//	.pStages = {},
	//	.pVertexInputState = {},
	//	.pInputAssemblyState = {},
	//	.pTessellationState = {},
	//	.pViewportState = {},
	//	.pRasterizationState = {},
	//	.pMultisampleState = {},
	//	.pDepthStencilState = {},
	//	.pColorBlendState = {},
	//	.pDynamicState = {},
	//	.layout = {},
	//	.renderPass = {},
	//	.subpass = {},
	//	.basePipelineHandle = {},
	//	.basePipelineIndex = {},
	// };

	objc_msgSend_void(macos_app->ns_app, sel_registerName("finishLaunching"));

	return FCK_MACOS_RESULT_CONTINUE;
}

fck_macos_result fck_macos_app_tick(void *appState)
{
	fck_macos_application *macos_app = (fck_macos_application *)appState;
	id pool = objc_msgSend_id(NSAlloc(objc_getClass("NSAutoreleasePool")), sel_registerName("init"));

	NSEvent *e = (NSEvent *)((id(*)(id, SEL, NSEventMask, void *, NSString *, bool))objc_msgSend)( //
		macos_app->ns_app,                                                                         //
		sel_registerName("nextEventMatchingMask:untilDate:inMode:dequeue:"),                       //
		ULONG_MAX,                                                                                 //
		NULL,                                                                                      //
		((id(*)(id, SEL, const char *))objc_msgSend)((id)objc_getClass("NSString"), sel_registerName("stringWithUTF8String:"),
	                                                 "kCFRunLoopDefaultMode"),
		true);

	NSEventType type = (NSEventType)objc_msgSend_uint(e, sel_registerName("type"));

	NSPoint p = ((NSPoint(*)(id, SEL))objc_msgSend)(e, sel_registerName("locationInWindow"));

	if (type != 0)
	{
		unsigned int modifierFlags = objc_msgSend_uint(e, sel_registerName("modifierFlags"));
		printf("Event [type=%s location={%f, %f} modifierFlags={%s}]\n", NSEventTypeToChar(type), p.x, p.y,
		       NSEventModifierFlagsToChar(modifierFlags));

		fck_event_input_device_keyboard keyboard;
		unsigned int keycode = UINT32_MAX;

		int isAnyKeyEvent = type == NSEventTypeKeyDown;
		switch (type)
		{
		case NSEventTypeKeyDown:
			keyboard.type = FCK_KEYBOARD_EVENT_TYPE_DOWN;
			// Can make it a function instead of goto.
			goto keyboard_event_setup;
		case NSEventTypeKeyUp: {
			keyboard.type = FCK_KEYBOARD_EVENT_TYPE_UP;
		keyboard_event_setup:
			keycode = objc_msgSend_ushort(e, sel_registerName("keyCode"));
			// NSEventModifierFlags em = objc_msgSend_uint(e, sel_registerName("modifierFlags"));
			keyboard.common.size = sizeof(keyboard);
			keyboard.common.timestamp = os->chrono->ms(); // TODO
			keyboard.common.type = FCK_EVENT_INPUT_TYPE_DEVICE;

			if (keycode < fck_arraysize(macos_app->pkeys))
			{
				keyboard.pkey = (fck_pkey)keycode;
			}
			else
			{
				keyboard.pkey = FCK_PKEY_UNKNOWN;
			}

			keyboard.device_type = FCK_INPUT_DEVICE_TYPE_KEYBOARD;
			keyboard.vkey = FCK_VKEY_UNKNOWN; // Maybe remove this for now??
			keyboard.mod = 0;
			break;
		}
		default:
			break;
		}

		if (type == NSEventTypeKeyDown)
		{
			keycode = objc_msgSend_ushort(e, sel_registerName("keyCode"));
			NSEventModifierFlags em = objc_msgSend_uint(e, sel_registerName("modifierFlags"));
			printf("Down: %llu\n", (unsigned long long)em);
		}
		if (type == NSEventTypeKeyUp)
		{
			keycode = objc_msgSend_ushort(e, sel_registerName("keyCode"));
			printf("Up: %d\n", (int)keycode);
		}
	}

	objc_msgSend_void_id(macos_app->ns_app, sel_registerName("sendEvent:"), e);
	((void (*)(id, SEL))objc_msgSend)(macos_app->ns_app, sel_registerName("updateWindows"));

	NSRelease(pool);
	// That's it for this frame.
	return FCK_MACOS_RESULT_CONTINUE;
}

void fck_macos_app_quit(void *appState, fck_macos_result result)
{
}

const char *NSEventTypeToChar(NSEventType eventType)
{
	switch (eventType)
	{
	case NSEventTypeLeftMouseDown:
		return "LeftMouseDown";
	case NSEventTypeLeftMouseUp:
		return "LeftMouseUp";
	case NSEventTypeRightMouseDown:
		return "RightMouseDown";
	case NSEventTypeRightMouseUp:
		return "RightMouseUp";
	case NSEventTypeMouseMoved:
		return "MouseMoved";
	case NSEventTypeLeftMouseDragged:
		return "LeftMouseDragged";
	case NSEventTypeRightMouseDragged:
		return "RightMouseDragged";
	case NSEventTypeMouseEntered:
		return "MouseEntered";
	case NSEventTypeMouseExited:
		return "MouseExited";
	case NSEventTypeKeyDown:
		return "KeyDown";
	case NSEventTypeKeyUp:
		return "KeyUp";
	case NSEventTypeFlagsChanged:
		return "FlagsChanged";
	case NSEventTypeAppKitDefined:
		return "AppKitDefined";
	case NSEventTypeSystemDefined:
		return "SystemDefined";
	case NSEventTypeApplicationDefined:
		return "ApplicationDefined";
	case NSEventTypePeriodic:
		return "Periodic";
	case NSEventTypeCursorUpdate:
		return "CursorUpdate";
	case NSEventTypeScrollWheel:
		return "ScrollWheel";
	case NSEventTypeTabletPoint:
		return "TabletPoint";
	case NSEventTypeTabletProximity:
		return "TabletProximity";
	case NSEventTypeOtherMouseDown:
		return "OtherMouseDown";
	case NSEventTypeOtherMouseUp:
		return "OtherMouseUp";
	case NSEventTypeOtherMouseDragged:
		return "OtherMouseDragged";
	default:
		return "N/A";
	}
}

char *ns_strcat(register char *s, register const char *append)
{
	char *save = s;

	for (; *s; ++s)
		;
	while ((*s++ = *append++))
		;
	return save;
}

const char *NSEventModifierFlagsToChar(NSEventModifierFlags modifierFlags)
{
	static char result[100];
	result[0] = '\0';

	if ((modifierFlags & NSEventModifierFlagCapsLock) == NSEventModifierFlagCapsLock)
		ns_strcat(result, "CapsLock, ");
	if ((modifierFlags & NSEventModifierFlagShift) == NSEventModifierFlagShift)
		ns_strcat(result, "NShift, ");
	if ((modifierFlags & NSEventModifierFlagControl) == NSEventModifierFlagControl)
		ns_strcat(result, "Control, ");
	if ((modifierFlags & NSEventModifierFlagOption) == NSEventModifierFlagOption)
		ns_strcat(result, "Option, ");
	if ((modifierFlags & NSEventModifierFlagCommand) == NSEventModifierFlagCommand)
		ns_strcat(result, "Command, ");
	if ((modifierFlags & NSEventModifierFlagNumericPad) == NSEventModifierFlagNumericPad)
		ns_strcat(result, "NumericPad, ");
	if ((modifierFlags & NSEventModifierFlagHelp) == NSEventModifierFlagHelp)
		ns_strcat(result, "Help, ");
	if ((modifierFlags & NSEventModifierFlagFunction) == NSEventModifierFlagFunction)
		ns_strcat(result, "Function, ");

	return result;
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
	}

	fck_macos_app_quit(app, result);
	return 0;
}
