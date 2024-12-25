#include "rnd/rnd_renderer.h"
#include "rnd_debug.h"

#include <vulkan/vulkan.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <chrono>
#include <thread>
#include <vector>

constexpr bool bUseValidationLayers = false;

static bool rnd_checkValidationLayerSupport(const rnd_renderer *renderer)
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char *layerName : renderer->validationLayers)
	{
		bool layerFound = false;

		for (const auto &layerProperties : availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound)
		{
			return false;
		}
	}

	return true;
}

std::vector<const char *> rnd_getRequiredExtensions(const rnd_renderer *renderer)
{
	std::vector<const char *> target_extensions;

	if (renderer->enableValidationLayers)
	{
		const char *debug_extension = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
		target_extensions.push_back(debug_extension);
	}

	uint32_t sdlExtensionCount;
	char const *const *sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);
	for (uint32_t index = 0; index < sdlExtensionCount; index++)
	{
		target_extensions.push_back(sdlExtensions[index]);
	}

	return target_extensions;
}

static void rnd_createInstance(rnd_renderer *renderer)
{
	if (renderer->enableValidationLayers && !rnd_checkValidationLayerSupport(renderer))
	{
		SDL_Log("validation layers requested, but not available!");
		SDL_assert(true);
	}

	// Some optional info about application
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "More like Vulcan't";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "FcK";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	// Tells the Vulkan driver which global extensions and validation layers we want to use
	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.flags = 0;

	auto extensions = rnd_getRequiredExtensions(renderer);
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if (renderer->enableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(renderer->validationLayers.size());
		createInfo.ppEnabledLayerNames = renderer->validationLayers.data();

		rnd_populateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	VK_CHECK(vkCreateInstance(&createInfo, nullptr, &renderer->instance), "failed to create instance");
}

void rnd_init(rnd_renderer *renderer)
{
	// todo this cant be a thing right
	SDL_zerop(renderer);

	// todo, move this out
	SDL_Init(SDL_INIT_VIDEO);
	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);
	renderer->window = SDL_CreateWindow("More like Vulcan't 🤣", renderer->windowExtent.width, renderer->windowExtent.height, window_flags);

	rnd_createInstance(renderer);
	rnd_setupDebugMessenger(renderer);
}

void rnd_render(rnd_renderer *renderer)
{
	SDL_assert(renderer != nullptr);
	if (!renderer)
	{
		return;
	}

	// minimized, set stop rendering
	// restored, unset stop rendering
	// rename it to pause rendering wtf

	// do not draw if we are minimized
	if (renderer->stopRendering)
	{
		// throttle the speed to avoid the endless spinning
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		return;
	}

	//// Ask the swapchain for the index of the swapchain image we can render onto
	// int image_index = request_image(mySwapchain);

	//// Create a new command buffer
	// VkCommandBuffer cmd = allocate_command_buffer();

	//// Initialize the command buffer
	// vkBeginCommandBuffer(cmd, ...);

	//// Start a new renderpass with the image index from swapchain as target to render onto
	//// Each framebuffer refers to a image in the swapchain
	// vkCmdBeginRenderPass(cmd, main_render_pass, framebuffers[image_index]);

	//// Rendering all objects
	// for (object in PassObjects)
	//{

	//	// Bind the shaders and configuration used to render the object
	//	vkCmdBindPipeline(cmd, object.pipeline);

	//	// Bind the vertex and index buffers for rendering the object
	//	vkCmdBindVertexBuffers(cmd, object.VertexBuffer, ...);
	//	vkCmdBindIndexBuffer(cmd, object.IndexBuffer, ...);

	//	// Bind the descriptor sets for the object (shader inputs)
	//	vkCmdBindDescriptorSets(cmd, object.textureDescriptorSet);
	//	vkCmdBindDescriptorSets(cmd, object.parametersDescriptorSet);

	//	// Execute drawing
	//	vkCmdDraw(cmd, ...);
	//}

	//// Finalize the render pass and command buffer
	// vkCmdEndRenderPass(cmd);
	// vkEndCommandBuffer(cmd);

	//// Submit the command buffer to begin execution on GPU
	// vkQueueSubmit(graphicsQueue, cmd, ...);

	//// Display the image we just rendered on the screen
	//// renderSemaphore makes sure the image isn't presented until `cmd` is finished executing
	// vkQueuePresent(graphicsQueue, renderSemaphore);
}
void rnd_cleanup(rnd_renderer *renderer)
{
	if (renderer->enableValidationLayers)
	{
		rnd_destroyDebugUtilsMessengerEXT(renderer->instance, renderer->debugMessenger, nullptr);
	}
	vkDestroyInstance(renderer->instance, nullptr);

	// cleaning up window yay nay?
	SDL_assert(renderer != nullptr);
	if (!renderer)
	{
		return;
	}

	renderer = nullptr;
}
