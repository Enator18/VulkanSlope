#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <stdexcept>

#include "slope_game.h"
#include "VkBootstrap.h"
#include "render_core.h"

static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	auto app = reinterpret_cast<SlopeGame*>(glfwGetWindowUserPointer(window));
	app->onResize(width, height);
}

void SlopeGame::onResize(int width, int height)
{
	this->width = width;
	this->height = height;
}

void SlopeGame::init()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	window = glfwCreateWindow(800, 600, "Slope", nullptr, nullptr);
	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

	initRenderer();
}

void SlopeGame::initRenderer()
{
	vkb::InstanceBuilder builder;

	auto instRet = builder.set_app_name("Slope")
		.request_validation_layers(true)
		.require_api_version(1, 1, 0)
		.use_default_debug_messenger()
		.build();

	if (!instRet)
	{
		throw std::runtime_error("Failed to create instance");
	}

	vkb::Instance vkbInstance = instRet.value();
	instance = vkbInstance.instance;
	messenger = vkbInstance.debug_messenger;

	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create window surface!");
	}

	vkb::PhysicalDeviceSelector selector{ vkbInstance };

	auto devRet = selector.set_surface(surface)
		.set_minimum_version(1, 1)
		.prefer_gpu_device_type()
		.select();
	vkb::PhysicalDevice vkbPhysicalDevice = devRet.value();

	VkPhysicalDevice physicalDevice = vkbPhysicalDevice.physical_device;

	vkb::DeviceBuilder deviceBuilder{ vkbPhysicalDevice };

	vkb::Device vkbDevice = deviceBuilder.build().value();

	VkDevice device = vkbDevice.device;

	renderer.init(&instance, &surface, physicalDevice, device);
}

bool SlopeGame::tick()
{
	glfwPollEvents();
	return !glfwWindowShouldClose(window);
}

void SlopeGame::cleanup()
{
	renderer.cleanup();

	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkb::destroy_debug_utils_messenger(instance, messenger);
	vkDestroyInstance(instance, nullptr);

	glfwDestroyWindow(window);
	glfwTerminate();
}