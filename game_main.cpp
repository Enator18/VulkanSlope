#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <stdexcept>
#include <iostream>
#include <chrono>
#include <algorithm>
#include <filesystem>

#include "game_main.h"
#include "VkBootstrap.h"
#include "render_core.h"
#include "mesh.h"
#include "file_io.h"
#include "math_utils.h"
#include "entity.h"

namespace fs = std::filesystem;

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

	loadAssets();

	mainScene = loadScene("scenes/testmap.json", assets, textures);

	for (auto& entity : mainScene)
	{
		entity->begin();
	}

	cameraTransform.position = glm::vec3(-8.0, 0.0, 0.0);

	previousTime = std::chrono::high_resolution_clock::now();

	
}

void SlopeGame::loadAssets()
{
	//Load Models
	for (const auto& entry : fs::directory_iterator("models"))
	{
		const auto path = entry.path();
		if (entry.is_regular_file() && path.extension().string().compare("glb"))
		{
			std::optional<std::vector<std::shared_ptr<MeshAsset>>> model = loadModel(path);
			for (std::shared_ptr<MeshAsset> asset : model.value())
			{
				renderer.uploadMesh(asset.get()->mesh);
				assets.insert({ path.filename().stem().string(), *asset.get()});
			}
		}
	}

	//Load Textures
	for (const auto& entry : fs::directory_iterator("textures"))
	{
		const auto path = entry.path();
		if (entry.is_regular_file() && path.extension().string().compare("png"))
		{
			const std::string name = path.filename().stem().string();
			TextureAsset texture = loadImage(path, name);
			textures.insert({ name, renderer.uploadTexture(texture.data, texture.width, texture.height) });
		}
	}
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

	for (auto& entity : mainScene)
	{
		entity->tick(delta);
	}

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

	for (auto& element : textures)
	{
		renderer.deleteTexture(element.second);
	}

	renderer.cleanup();

	vkDestroySurfaceKHR(vkbInstance.instance, surface, nullptr);
	vkDestroyInstance(vkbInstance.instance, nullptr);

	glfwDestroyWindow(window);
	glfwTerminate();
}