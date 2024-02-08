#pragma once
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <vector>
#include <chrono>

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

	std::vector<MeshAsset> assets;
	std::vector<MeshInstance> instances;

	int width = 800;
	int height = 600;

	double mouseX = 0, mouseY = 0;

	std::chrono::steady_clock::time_point currentTime;
	std::chrono::steady_clock::time_point previousTime;
};