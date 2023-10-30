#pragma once
#include <vulkan/vulkan.h>

#include "render_types.h"

class RenderDevice
{
private:
	VkPhysicalDevice physicalDevice;
	VkDevice device;

	void pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface, VkPhysicalDeviceMemoryProperties* memProperties);
	bool deviceSupportsExtensions();
	uint32_t deviceSuitability(VkSurfaceKHR surface);
	QueueFamilyIndices findQueueFamilies(VkSurfaceKHR surface);
};