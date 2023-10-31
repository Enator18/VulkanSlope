#pragma once
#include <vulkan/vulkan.h>

#include "render_device.h"

class Renderer
{
public:
	void init(VkInstance* instance, VkSurfaceKHR* surface);
	void cleanup();
private:
	VkInstance* instance;
	VkSurfaceKHR* surface;

	VkPhysicalDeviceMemoryProperties memProperties;
	RenderDevice device;
};
