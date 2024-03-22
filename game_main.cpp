#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <stdexcept>
#include <iostream>
#include <chrono>
#include <algorithm>

#include "game_main.h"
#include "VkBootstrap.h"
#include "render_core.h"
#include "mesh.h"
#include "file_io.h"
#include "math_utils.h"

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
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

#ifdef NDEBUG
	bool validation = false;
#else
	bool validation = true;
#endif

	vkb::InstanceBuilder builder;

	auto instRet = builder.set_app_name("Slope")
		.request_validation_layers(validation)
		.require_api_version(1, 3, 0)
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
		assets.insert({ "monkeyhead", *asset.get() });
	}

	std::optional<std::vector<std::shared_ptr<MeshAsset>>> models2 = loadModel("models/cube.glb");

	for (std::shared_ptr<MeshAsset> asset : models2.value())
	{
		renderer.uploadMesh(asset.get()->mesh);
		assets.insert({ "cube", *asset.get() });
	}

	TextureAsset stone = loadImage("textures/stonetexture.png", "stone");
	TextureAsset dirt = loadImage("textures/dirt.png", "dirt");

	uint32_t stoneIndex = renderer.uploadTexture(stone.data, stone.width, stone.height);
	uint32_t dirtIndex = renderer.uploadTexture(dirt.data, dirt.width, dirt.height);

	std::vector<std::unique_ptr<Entity>>mainScene = loadScene("scenes/testmap.json", assets);

	MeshInstance instance = { &assets["monkeyhead"].mesh, {glm::vec3(0.0, 2.0, 0.0), glm::vec3(-90.0f, -90.0f, 0.0f), glm::vec3(1, 1, 1)}, stoneIndex};
	MeshInstance instance2 = { &assets["monkeyhead"].mesh, {glm::vec3(0.0, -2.0, 0.0), glm::vec3(-90.0f, -90.0f, 0.0f), glm::vec3(1, 1, 1)}, dirtIndex};

	MeshInstance instance3 = { &assets["cube"].mesh, {glm::vec3(3.0, 0.0, 0.0), glm::vec3(0.0f, 45.0f, 0.0f)}, dirtIndex};

	instances.push_back(instance);
	instances.push_back(instance2);
	instances.push_back(instance3);

	cameraTransform.position = glm::vec3(-8.0, 0.0, 0.0);

	previousTime = std::chrono::high_resolution_clock::now();
}

bool SlopeGame::tick()
{
	glfwPollEvents();

	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}

	currentTime = std::chrono::high_resolution_clock::now();

	float delta = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - previousTime).count();

	previousTime = currentTime;

	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	cameraTransform.rotation.x += (mouseY - ypos) * MOUSE_SENSITIVITY;
	cameraTransform.rotation.y += (mouseX - xpos) * MOUSE_SENSITIVITY;

	cameraTransform.rotation.x = std::clamp(cameraTransform.rotation.x, -90.0f, 90.0f);

	mouseX = xpos;
	mouseY = ypos;

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
	{
		cameraTransform.position += glm::vec3(glm::vec4(FLY_SPEED * delta, 0.0, 0.0, 1.0) * cameraTransform.getRotationMatrix());
	}

	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
	{
		cameraTransform.position += glm::vec3(glm::vec4(0.0, -FLY_SPEED * delta, 0.0, 1.0) * cameraTransform.getRotationMatrix());
	}

	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
	{
		cameraTransform.position += glm::vec3(glm::vec4(-FLY_SPEED * delta, 0.0, 0.0, 1.0) * cameraTransform.getRotationMatrix());
	}

	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
	{
		cameraTransform.position += glm::vec3(glm::vec4(0.0, FLY_SPEED * delta, 0.0, 1.0) * cameraTransform.getRotationMatrix());
	}

	instances[2].transform.rotation.x += 25 * delta;

	glm::mat4 view = glm::lookAt(cameraTransform.position, cameraTransform.position + glm::vec3(glm::vec4(1, 0, 0, 1) * cameraTransform.getRotationMatrix()), glm::vec3(glm::vec4(0, 0, 1, 1) * cameraTransform.getRotationMatrix()));
	glm::mat4 projection = glm::rotate(glm::perspective(glm::radians(45.0f), width / (float)height, 0.1f, 40.0f), glm::radians(180.0f), glm::vec3(0.0, 0.0, 1.0));

	Camera camera = { view, projection };

	renderer.drawFrame(mainScene, camera);

	return !glfwWindowShouldClose(window);
}

void SlopeGame::cleanup()
{
	for (auto& element : assets)
	{
		renderer.deleteMesh(element.second.mesh);
	}

	renderer.cleanup();

	vkDestroySurfaceKHR(vkbInstance.instance, surface, nullptr);
	vkDestroyInstance(vkbInstance.instance, nullptr);

	glfwDestroyWindow(window);
	glfwTerminate();
}