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
#include "file_io.h"

static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	auto app = reinterpret_cast<SlopeGame*>(glfwGetWindowUserPointer(window));
	app->onResize(width, height);
}

void SlopeGame::onResize(int width, int height)
{
	this->width = width;
	this->height = height;
	renderer.onResized(width, height);
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

	std::optional<std::vector<std::shared_ptr<MeshAsset>>> models = loadModel("models/monkeyhead.glb");

	for (std::shared_ptr<MeshAsset> asset : models.value())
	{
		renderer.uploadMesh(asset.get()->mesh);
		assets.push_back(*asset.get());
	}

	MeshInstance instance = { &assets[0].mesh, {glm::vec3(2.0, 0, 0.0), glm::vec3(glm::radians(90.0f), glm::radians(0.0f), glm::radians(-90.0f)), glm::vec3(1, 1, 1)}};
	MeshInstance instance2 = { &assets[0].mesh, {glm::vec3(-2.0, 0, 0.0), glm::vec3(glm::radians(90.0f), glm::radians(0.0f), glm::radians(-90.0f)), glm::vec3(1, 1, 1)} };

	instances.push_back(instance);
	instances.push_back(instance2);

	cameraTransform.position = glm::vec3(-8.0, 0.0, 0.0);
}

bool SlopeGame::tick()
{
	glfwPollEvents();

	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}

	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();

	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	//instances[0].transform.rotation = glm::vec3(glm::radians(90.0f), 0.0, 0.8 * time);

	glm::mat4 view = glm::lookAt(cameraTransform.position, cameraTransform.position + glm::vec3(glm::vec4(1, 0, 0, 1) * cameraTransform.getTransformMatrix()), glm::vec3(glm::vec4(0, 0, 1, 1) * cameraTransform.getTransformMatrix()));
	glm::mat4 projection = glm::rotate(glm::perspective(glm::radians(45.0f), width / (float)height, 0.1f, 10.0f), glm::radians(180.0f), glm::vec3(0.0, 0.0, 1.0));

	Camera camera = { view, projection };

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