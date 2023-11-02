#pragma once
#include <vulkan/vulkan.h>

class Renderer
{
public:
	void init(VkInstance* instance, VkSurfaceKHR* surface, VkPhysicalDevice physicalDevice, VkDevice device);
	void cleanup();
private:
	VkInstance* instance;
	VkSurfaceKHR* surface;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkSwapchainKHR swapchain;
};
