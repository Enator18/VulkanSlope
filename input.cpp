#include "input.h"
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

InputHandler::InputHandler(void* window)
{
	keybinds =
	{
		{"forward", GLFW_KEY_W},
		{"backward", GLFW_KEY_S},
		{"left", GLFW_KEY_A},
		{"right", GLFW_KEY_D}
	};

	this->window = window;
}

bool InputHandler::readBind(std::string bind)
{
	return glfwGetKey((GLFWwindow*)window, keybinds[bind]);
}