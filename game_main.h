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
#include "engine_types.h"
#include "input.h"

class SlopeGame
{
public:
	void onResize(int width, int height);
	void init();
	bool tick();
	void cleanup();
	Scene getCurrentScene();
private:
	const float MOUSE_SENSITIVITY = 0.1;
	const float FLY_SPEED = 2;

	GLFWwindow* window;
	vkb::Instance vkbInstance;
	VkSurfaceKHR surface;

	Mesh triangle;
	Mesh triangle2;

	Renderer renderer;

	std::unordered_map<std::string, MeshAsset> assets;
	std::unordered_map<std::string, TextureImage> textures;
	Scene mainScene;

	int width = 960;
	int height = 540;

	InputHandler inputHandler = {window};

	double mouseX = 0, mouseY = 0;

	std::chrono::steady_clock::time_point currentTime;
	std::chrono::steady_clock::time_point previousTime;

	void loadAssets();
};