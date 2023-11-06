#include <vulkan/vulkan.h>

#include "render_core.h"
#include "VkBootstrap.h"

void Renderer::init(vkb::Instance vkbInstance, VkSurfaceKHR* surface, uint32_t width, uint32_t height)
{
	instance = vkbInstance.instance;
	this->surface = surface;

	messenger = vkbInstance.debug_messenger;
	
	vkb::PhysicalDeviceSelector selector{ vkbInstance };

	auto devRet = selector.set_surface(*surface)
		.set_minimum_version(1, 1)
		.prefer_gpu_device_type()
		.select();
	vkb::PhysicalDevice vkbPhysicalDevice = devRet.value();

	physicalDevice = vkbPhysicalDevice.physical_device;

	vkb::DeviceBuilder deviceBuilder{ vkbPhysicalDevice };

	vkb::Device vkbDevice = deviceBuilder.build().value();

	device = vkbDevice.device;

	vkb::SwapchainBuilder swapchainBuilder{ vkbPhysicalDevice, vkbDevice, *surface };

	vkb::Swapchain vkbSwapchain = swapchainBuilder
		.use_default_format_selection()
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_extent(width, height)
		.build()
		.value();

	swapchain = vkbSwapchain.swapchain;
	swapchainImages = vkbSwapchain.get_images().value();
	swapchainImageViews = vkbSwapchain.get_image_views().value();

	swapchainImageFormat = vkbSwapchain.image_format;
};

void Renderer::cleanup()
{
	for (VkImageView imageView : swapchainImageViews)
	{
		vkDestroyImageView(device, imageView, nullptr);
	}

	vkDestroySwapchainKHR(device, swapchain, nullptr);

	vkDestroyDevice(device, nullptr);
	vkb::destroy_debug_utils_messenger(instance, messenger);
};