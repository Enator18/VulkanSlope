#pragma once
#include <vulkan/vulkan.h>

#include "render_types.h"

class RenderDevice
{
public:
	void createDevice(VkInstance instance, VkSurfaceKHR surface, VkPhysicalDeviceMemoryProperties* memProperties);
	void destroyDevice();

private:
	const std::vector<const char*> deviceExtensions =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	VkPhysicalDevice physicalDevice;
	VkDevice device;
	VkQueue graphicsQueue;
	VkQueue presentQueue;

	void pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface, VkPhysicalDeviceMemoryProperties* memProperties);
	bool deviceSupportsExtensions(VkPhysicalDevice device);
	uint32_t deviceSuitability(VkPhysicalDevice device, VkSurfaceKHR surface);
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
	void createLogicalDevice(VkSurfaceKHR surface);
};