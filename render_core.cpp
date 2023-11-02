#include <vulkan/vulkan.h>

#include "render_core.h"
#include "VkBootstrap.h"

void Renderer::init(VkInstance* instance, VkSurfaceKHR* surface, VkPhysicalDevice physicalDevice, VkDevice device)
{
	this->instance = instance;
	this->surface = surface;
	this->physicalDevice = physicalDevice;
	this->device = device;
};

void Renderer::cleanup()
{
	vkDestroyDevice(device, nullptr);
};