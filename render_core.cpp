#include <vulkan/vulkan.h>

#include "render_core.h"

void Renderer::init(VkInstance* instance, VkSurfaceKHR* surface)
{
	this->instance = instance;
	this->surface = surface;
};