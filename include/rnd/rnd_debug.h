#ifndef FCK_RND_DEBUG_INCLUDED
#define FCK_RND_DEBUG_INCLUDED

#include <rnd/rnd_renderer.h>

void rnd_setupDebugMessenger(rnd_renderer *renderer);

void rnd_populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);

void rnd_destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                       const VkAllocationCallbacks *pAllocator);

VkResult rnd_createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                          const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger);

static VKAPI_ATTR VkBool32 VKAPI_CALL rnd_debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                        VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData);

#endif // FCK_RND_DEBUG_INCLUDED