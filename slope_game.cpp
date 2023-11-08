#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <stdexcept>
#include <iostream>

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
	std::cout << "Width: " << width << ", " << "Height: " << height << "\n";
	renderer.updateSwapchain(width, height);
}

void SlopeGame::init()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	window = glfwCreateWindow(width, height, "Slope", nullptr, nullptr);
	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

#ifdef NDEBUG
	bool validation = false;
#else
	bool validation = true;
#endif

	vkb::InstanceBuilder builder;

	auto instRet = builder.set_app_name("Slope")
		.request_validation_layers(validation)
		.require_api_version(1, 1, 0)
		.use_default_debug_messenger()
		.build();

	if (!instRet)
	{
		throw std::runtime_error("Failed to create instance");
	}

	vkbInstance = instRet.value();

	if (glfwCreateWindowSurface(vkbInstance, window, nullptr, &surface) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create window surface!");
	}

	renderer.init(vkbInstance, &surface, width, height);
}

bool SlopeGame::tick()
{
	glfwPollEvents();

	renderer.drawFrame();

	return !glfwWindowShouldClose(window);
}

void SlopeGame::cleanup()
{
	renderer.cleanup();

	vkDestroySurfaceKHR(vkbInstance.instance, surface, nullptr);
	vkDestroyInstance(vkbInstance.instance, nullptr);

	glfwDestroyWindow(window);
	glfwTerminate();
}