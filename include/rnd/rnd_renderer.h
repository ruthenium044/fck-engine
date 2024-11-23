#ifndef FCK_RND_RENDERER_INCLUDED
#define FCK_RND_RENDERER_INCLUDED

#include "SDL3/SDL_log.h"

#include <vector>
#include <vulkan/vulkan.h>

#define VK_CHECK(x, msg)                                                                                                                   \
	do                                                                                                                                     \
	{                                                                                                                                      \
		VkResult err = x;                                                                                                                  \
		if (err)                                                                                                                           \
		{                                                                                                                                  \
			SDL_Log("Detected Vulkan error: {}", msg);                                                                                     \
			abort();                                                                                                                       \
		}                                                                                                                                  \
	} while (0)

struct rnd_renderer
{
	int frameNumber{0};
	bool stopRendering{false};
	static constexpr VkExtent2D windowExtent{666, 666};

	struct SDL_Window *window{nullptr};

	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;

	const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif
};

void rnd_init(rnd_renderer *renderer);
void rnd_render(rnd_renderer *renderer);
void rnd_cleanup(rnd_renderer *renderer);

#endif // FCK_RND_RENDERER_INCLUDED