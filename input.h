#pragma once
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <unordered_map>
#include <string>

class InputHandler
{
public:
	bool readBind(std::string bind);
private:
	GLFWwindow* window;
	std::unordered_map<std::string, int> keybinds =
	{
		{"forward", GLFW_KEY_W},
		{"backward", GLFW_KEY_S},
		{"left", GLFW_KEY_A},
		{"right", GLFW_KEY_D}
	};
};
