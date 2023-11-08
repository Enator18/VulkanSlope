#pragma once
#include <vulkan/vulkan.h>

#include "VkBootstrap.h"

class Renderer
{
public:
	void init(vkb::Instance vkbInstance, VkSurfaceKHR* surface, uint32_t width, uint32_t height);
	void updateSwapchain(uint32_t width, uint32_t height);
	void drawFrame();
	void cleanup();
private:
	VkInstance instance;
	VkSurfaceKHR* surface;
	VkDebugUtilsMessengerEXT messenger;
	vkb::PhysicalDevice physicalDevice;
	vkb::Device device;
	VkQueue graphicsQueue;
	uint32_t graphicsQueueFamily;
	VkSwapchainKHR swapchain;
	VkFormat swapchainImageFormat;
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;
	VkCommandPool commandPool;
	VkCommandBuffer commandBuffer;

	void createSwapchain(uint32_t width, uint32_t height);
	void cleanupSwapchain();
	void initCommands();
};
