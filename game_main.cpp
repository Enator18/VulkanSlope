#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <stdexcept>
#include <iostream>
#include <chrono>

#include "game_main.h"
#include "VkBootstrap.h"
#include "render_core.h"
#include "mesh.h"

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
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
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

	std::vector<Vertex> vertices = {
		{glm::vec3(-0.5, -0.5, 0), glm::vec3(1.0, 0.0, 0.0), glm::vec2(-0.5, -0.5)},
		{glm::vec3(0.5, -0.5, 0), glm::vec3(1.0, 0.0, 0.0), glm::vec2(0.5, -0.5)},
		{glm::vec3(0.5, 0.5, 0), glm::vec3(1.0, 0.0, 0.0), glm::vec2(0.5, 0.5)}
	};

	std::vector<Vertex> vertices2 = {
		{glm::vec3(-0.5, -0.5, 0), glm::vec3(1.0, 0.0, 0.0), glm::vec2(-0.5, -0.5)},
		{glm::vec3(0.5, -0.5, 0), glm::vec3(1.0, 0.0, 0.0), glm::vec2(0.5, -0.5)},
		{glm::vec3(0.5, 0.5, 0), glm::vec3(1.0, 0.0, 0.0), glm::vec2(0.5, 0.5)}
	};

	std::vector<uint32_t> indices = { 0, 1, 2 };
	triangle = { vertices, indices };
	triangle2 = { vertices2, indices };
	renderer.uploadMesh(triangle);
	renderer.uploadMesh(triangle2);

	MeshInstance instance = { &triangle, glm::vec3(0, 0, 0), glm::quat(), glm::vec3(1, 1, 1) };
	MeshInstance instance2 = { &triangle2, glm::vec3(0, 0, -0.5), glm::quat(), glm::vec3(1, 1, 1) };
	instance.transform = instance.getTransformMatrix();
	instance2.transform = instance2.getTransformMatrix();

	instances.push_back(instance);
	instances.push_back(instance2);
}

bool SlopeGame::tick()
{
	glfwPollEvents();

	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();

	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	glm::mat4 cameraTransform = glm::lookAt(glm::vec3(4.0f * cos(time), 4.0f * sin(time), 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), width / (float)height, 0.1f, 10.0f);

	Camera camera = { cameraTransform, projection };

	renderer.drawFrame(instances, camera);

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