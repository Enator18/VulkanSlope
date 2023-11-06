#pragma once
#include <vulkan/vulkan.h>

#include "VkBootstrap.h"

class Renderer
{
public:
	void init(vkb::Instance vkbInstance, VkSurfaceKHR* surface, uint32_t width, uint32_t height);
	void cleanup();
private:
	VkInstance instance;
	VkSurfaceKHR* surface;
	VkDebugUtilsMessengerEXT messenger;
	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkSwapchainKHR swapchain;
	VkFormat swapchainImageFormat;
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;
};
