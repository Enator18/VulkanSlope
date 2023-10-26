#pragma once
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
class SlopeGame
{
public:
	void onResize(int width, int height);
	void init();
	bool tick();
	void cleanup();
private:
	GLFWwindow* window;
	VkInstance instance;

	int width = 800;
	int height = 600;
};