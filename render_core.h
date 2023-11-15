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
	uint32_t width;
	uint32_t height;
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
	VkRenderPass renderPass;
	std::vector<VkFramebuffer> framebuffers;

	VkSemaphore presentSemaphore, renderSemaphore;
	VkFence renderFence;

	VkPipeline renderPipeline;

	void createSwapchain(uint32_t width, uint32_t height);
	void cleanupSwapchain();
	void initCommands();
	void initRenderpass();
	void initSyncStructures();
};
