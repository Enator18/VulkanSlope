#pragma once
#include <vulkan/vulkan.h>

class Renderer
{
public:
	void init(VkInstance* instance, VkSurfaceKHR* surface);
private:
	VkInstance* instance;
	VkSurfaceKHR* surface;
};
