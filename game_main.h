#pragma once
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <vector>

#include "render_core.h"
#include "mesh.h"
class SlopeGame
{
public:
	void onResize(int width, int height);
	void init();
	bool tick();
	void cleanup();
private:
	GLFWwindow* window;
	vkb::Instance vkbInstance;
	VkSurfaceKHR surface;

	Renderer renderer;

	std::vector<Entity> entities;

	int width = 800;
	int height = 600;
};