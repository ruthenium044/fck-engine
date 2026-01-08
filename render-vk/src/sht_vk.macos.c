#include "sht_vk.internal.h"
#include <vulkan/vulkan_metal.h>

#include <ApplicationServices/ApplicationServices.h>
#include <objc/message.h>

#include <dlfcn.h>

VkSurfaceKHR sht_vk_surface_create(sht_vk_instance *vk, sht_vk_platform *platform, const void *handle)
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
	PFN_vkCreateMetalSurfaceEXT create = (PFN_vkCreateMetalSurfaceEXT)platform->CreateSurfaceOpaque;
	if (sht_vk_error(create(vk->instance, &metal_create_info, NULL, &surface)))
	{
		return VK_NULL_HANDLE;
	}
	return surface;
}

VkPhysicalDevice sht_vk_physical_device_by_name(sht_vk_instance *vk, sht_vk_gpu *gpu, const char *target)
{
	VkPhysicalDevice phy_devices[16]; // There is no fucking way...
	fckc_u32 phy_device_count = fck_arraysize(phy_devices);
	if (sht_vk_error(gpu->EnumeratePhysicalDevices(vk->instance, &phy_device_count, phy_devices)))
	{
		// Umm...
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

VkBool32 sht_vk_gpu_select(sht_vk_instance *vk, sht_vk_gpu *gpu, const char *name, VkPhysicalDevice *device)
{
	*device = sht_vk_physical_device_by_name(vk, gpu, name);
	if (device == VK_NULL_HANDLE)
	{
		return VK_FALSE;
	}
	return VK_TRUE;
}

VkResult sht_vk_platform_init(sht_vk_instance *vk, sht_vk_platform *platform, sht_vk_gpu *gpu, fck_window window, VkSurfaceKHR *out_surface)
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

	platform->CreateSurfaceOpaque = (PFN_vkVoidFunction)dlsym(RTLD_DEFAULT, "vkCreateMetalSurfaceEXT");

	int width;
	int height;
	os->win->size(window, &width, &height);
	// VkExtent2D extent = (VkExtent2D){.width = width, .height = height};

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

	if (!sht_vk_gpu_select(vk, gpu, mtl_device_cstring, &gpu->device))
	{
		os->io->log("Could not find MTLDevice: %s", mtl_device_cstring);
		return VK_NOT_READY;
	}
	os->io->log("Found shared MTL-VK Device: %s", mtl_device_cstring);
	VkSurfaceKHR surface = sht_vk_surface_create(vk, platform, (const void *)mtl.layer);
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

	// os->win->resize(window, rect.size.width, rect.size.height);

	return VK_SUCCESS;
}