#include <vulkan/vulkan.h>

#include "render_core.h"
#include "render_device.h"

void Renderer::init(VkInstance* instance, VkSurfaceKHR* surface)
{
	this->instance = instance;
	this->surface = surface;

	device.createDevice(*instance, *surface, &memProperties);
};

void Renderer::cleanup()
{
	device.destroyDevice();
};