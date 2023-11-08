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
	physicalDevice = devRet.value();

	vkb::DeviceBuilder deviceBuilder{ physicalDevice };

	device = deviceBuilder.build().value();

	graphicsQueue = device.get_queue(vkb::QueueType::graphics).value();
	graphicsQueueFamily = device.get_queue_index(vkb::QueueType::graphics).value();

	createSwapchain(width, height);

	initCommands();
};

void Renderer::createSwapchain(uint32_t width, uint32_t height)
{
	vkb::SwapchainBuilder swapchainBuilder{ physicalDevice, device, *surface };

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

void Renderer::cleanupSwapchain()
{
	for (VkImageView imageView : swapchainImageViews)
	{
		vkDestroyImageView(device, imageView, nullptr);
	}

	vkDestroySwapchainKHR(device, swapchain, nullptr);
}

void Renderer::updateSwapchain(uint32_t width, uint32_t height)
{
	cleanupSwapchain();
	createSwapchain(width, height);
}

void Renderer::initCommands()
{
	VkCommandPoolCreateInfo commandPoolInfo = {};
	commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolInfo.pNext = nullptr;
	commandPoolInfo.queueFamilyIndex = graphicsQueueFamily;
	commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if (vkCreateCommandPool(device, &commandPoolInfo, nullptr, &commandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create command pool");
	}

	VkCommandBufferAllocateInfo cmdAllocInfo = {};
	cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdAllocInfo.pNext = nullptr;
	cmdAllocInfo.commandPool = commandPool;
	cmdAllocInfo.commandBufferCount = 1;
	cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	if (vkAllocateCommandBuffers(device, &cmdAllocInfo, nullptr, &commandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create command pool");
	}
}

void Renderer::drawFrame()
{

}

void Renderer::cleanup()
{
	vkDestroyCommandPool(device, commandPool, nullptr);

	cleanupSwapchain();

	vkDestroyDevice(device, nullptr);
	vkb::destroy_debug_utils_messenger(instance, messenger);
};