#include "sht_vk.internal.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <vulkan/vulkan_win32.h>

#elif defined(__APPLE__)
#include <ApplicationServices/ApplicationServices.h>
#include <objc/message.h>
#include <vulkan/vulkan_metal.h>

#include <dlfcn.h>
#endif

// Inline this...
#if defined(__APPLE__)
VkSurfaceKHR sht_vk_surface_create(sht_vk_instance *vk, sht_vk_platform *platform, const void *handle)
{
	// This shit in between here has to come from OUTSIDE the render api
	// since we have no control over a window! :)
	// Or maybe it doesn not? We know we are on macos...
	VkSurfaceKHR surface;

	VkMetalSurfaceCreateInfoEXT metal_create_info = (VkMetalSurfaceCreateInfoEXT){
		.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT,
		.pLayer = (const CAMetalLayer *)handle, //
		.flags = 0,
		.pNext = NULL,
	};

	PFN_vkCreateMetalSurfaceEXT create = (PFN_vkCreateMetalSurfaceEXT)platform->CreateSurfaceOpaque;
	if (sht_vk_error(create(vk->instance, &metal_create_info, NULL, &surface)))
	{
		return VK_NULL_HANDLE;
	}

	return surface;
}
#endif

// #if defined _WIN32
// #define FCK_WIN32 1
// #else
// #define FCK_WIN32 0
// #endif
//
//// This might not work long term!!
// #if defined __APPLE__
// #define FCK_APPLE 1
// #else
// #define FCK_APPLE 0
// #endif

void sht_vk_platform_adjust_instance(VkInstanceCreateInfo *create_info)
{
#if defined(__APPLE__)
	{
		create_info->flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
	}
#endif
}

void sht_vk_platform_adjust_extensions(const char **instance_extension_names, fckc_size_t *count)
{
#if defined(_WIN32)
	{
		instance_extension_names[*count] = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
		*count = *count + 1;
	}
#elif defined(__APPLE__)
	{
		instance_extension_names[*count] = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
		*count = *count + 1;
		instance_extension_names[*count] = VK_EXT_METAL_SURFACE_EXTENSION_NAME;
		*count = *count + 1;
	}
#endif
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
		if (strcmp(props.deviceName, target) == 0)
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
#if defined(_WIN32)
	{                                                                    uint32_t queueFamilyIndex);
		struct
		{
			sht_vk_declare(CreateWin32SurfaceKHR);
			sht_vk_declare(GetPhysicalDeviceWin32PresentationSupportKHR);
		} platform_data, *pf;
		pf = &platform_data;

		sht_vk_load_function(pf, vk->so, CreateWin32SurfaceKHR);
		sht_vk_load_function(pf, vk->so, GetPhysicalDeviceWin32PresentationSupportKHR);

		VkWin32SurfaceCreateInfoKHR create_info = {0};
		create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		create_info.hwnd = (HWND)os->win->native(window, "win32.window");
		create_info.hinstance = (HINSTANCE)os->win->native(window, "win32.instance");

		VkPhysicalDevice physical_device = NULL;
		{
			VkPhysicalDevice phy_devices[16]; // There is no fucking way...
			fckc_u32 phy_device_count = fck_arraysize(phy_devices);
			if (sht_vk_error(gpu->EnumeratePhysicalDevices(vk->instance, &phy_device_count, phy_devices)))
			{
				// Umm...
			}
			for (fckc_u32 index = 0; index < phy_device_count; index++)
			{
				physical_device = phy_devices[index];
				// QueueFamilyIndex 0. YOLO
				if (pf->GetPhysicalDeviceWin32PresentationSupportKHR(physical_device, 0))
				{
					break;
				}
				/*VkPhysicalDeviceProperties props = { 0 };
				gpu->GetPhysicalDeviceProperties(physical_device, &props);
				if (os->str->unsafe->cmp(props.deviceName, target) == 0)
				{
				    return physical_device;
				}*/
			}
		}
		if (physical_device == NULL)
		{
			return VK_NOT_READY;
		}

		pf->CreateWin32SurfaceKHR(vk->instance, &create_info, NULL, out_surface);
		if (*out_surface == VK_NULL_HANDLE)
		{
			return VK_NOT_READY;
		}
		gpu->device = physical_device;
	}

#elif defined(__APPLE__)
	{
		// 1. Declare and load the Metal surface function dynamically
		struct
		{
			sht_vk_declare(CreateMetalSurfaceEXT);
		} platform_data, *pf;
		pf = &platform_data;

		sht_vk_load_function(pf, vk->so, CreateMetalSurfaceEXT);

		if (!pf->CreateMetalSurfaceEXT)
		{
			os->io->log("Failed to load vkCreateMetalSurfaceEXT");
			return VK_NOT_READY;
		}

		// 2. Fetch the native NSWindow pointer
		void *nswindow = os->win->native(window, "macos.window");
		if (!nswindow)
		{
			os->io->log("Failed to retrieve native macOS window");
			return VK_NOT_READY;
		}

		// 3. Pure C invocation to extract the view
		void *view_selector = sel_registerName("contentView");
		void *view = ((void *(*)(void *, void *))objc_msgSend)(nswindow, view_selector);

		if (!view)
		{
			os->io->log("Failed to get contentView from NSWindow");
			return VK_NOT_READY;
		}

		// 4. Force the view to host a CAMetalLayer
		void *metal_layer_class = objc_getClass("CAMetalLayer");
		if (metal_layer_class)
		{
			void *layer_alloc_sel = sel_registerName("layer");
			void *new_metal_layer = ((void *(*)(void *, void *))objc_msgSend)(metal_layer_class, layer_alloc_sel);

			void *set_layer_sel = sel_registerName("setLayer:");
			((void (*)(void *, void *, void *))objc_msgSend)(view, set_layer_sel, new_metal_layer);

			void *set_wants_layer_sel = sel_registerName("setWantsLayer:");
			((void (*)(void *, void *, int))objc_msgSend)(view, set_wants_layer_sel, 1);
		}

		// 5. Verify and retrieve the layer handle
		void *layer_selector = sel_registerName("layer");
		void *metal_layer = ((void *(*)(void *, void *))objc_msgSend)(view, layer_selector);

		if (!metal_layer)
		{
			os->io->log("Failed to get CAMetalLayer backing from SDL3View");
			return VK_NOT_READY;
		}

		// 6. Populate the Metal Surface Create Info
		VkMetalSurfaceCreateInfoEXT create_info = {0};
		create_info.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
		create_info.pNext = NULL;
		create_info.flags = 0;
		create_info.pLayer = metal_layer;

		// 7. Enumerate Physical Devices
		VkPhysicalDevice physical_device = NULL;
		{
			VkPhysicalDevice phy_devices[16];
			fckc_u32 phy_device_count = fck_arraysize(phy_devices);
			if (sht_vk_error(gpu->EnumeratePhysicalDevices(vk->instance, &phy_device_count, phy_devices)))
			{
				os->io->log("Failed to enumerate physical devices");
				return VK_NOT_READY;
			}
			if (phy_device_count > 0)
			{
				physical_device = phy_devices[0];
			}
		}

		if (physical_device == NULL)
		{
			os->io->log("No physical devices found");
			return VK_NOT_READY;
		}

		// 8. Create the surface
		if (sht_vk_error(pf->CreateMetalSurfaceEXT(vk->instance, &create_info, NULL, out_surface)))
		{
			os->io->log("Failed to create Metal surface");
			return VK_NOT_READY;
		}

		gpu->device = physical_device;
	}
#else
	os->io->log("Platform not implemented");
	return VK_NOT_READY;
#endif
	os->io->log("Successfully created surface!");

	// os->win->resize(window, rect.size.width, rect.size.height);

	return VK_SUCCESS;
}