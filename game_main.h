#pragma once
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <vector>
#include <chrono>
#include <unordered_map>
#include <memory>

#include "render_core.h"
#include "render_types.h"
#include "mesh.h"
#include "file_io.h"

class SlopeGame
{
public:
	void onResize(int width, int height);
	void init();
	bool tick();
	void cleanup();
private:
	const float MOUSE_SENSITIVITY = 0.1;
	const float FLY_SPEED = 2;

	GLFWwindow* window;
	vkb::Instance vkbInstance;
	VkSurfaceKHR surface;

	Mesh triangle;
	Mesh triangle2;

	Transform cameraTransform;

	Renderer renderer;

	std::unordered_map<std::string, MeshAsset> assets;
	std::unordered_map<std::string, TextureImage> textures;
	std::vector<std::unique_ptr<Entity>> mainScene;

	int width = 960;
	int height = 540;


	double mouseX = 0, mouseY = 0;

	std::chrono::steady_clock::time_point currentTime;
	std::chrono::steady_clock::time_point previousTime;
};