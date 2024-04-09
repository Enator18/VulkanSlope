#include "input.h"
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

InputHandler::InputHandler(GLFWwindow* window)
{
	keybinds =
	{
		{"forward", GLFW_KEY_W},
		{"backward", GLFW_KEY_S},
		{"left", GLFW_KEY_A},
		{"right", GLFW_KEY_D}
	};

	this->window = window;
	mouseX1 = 0;
	mouseY1 = 0;
	mouseX2 = 0;
	mouseY2 = 0;
}

bool InputHandler::readBind(std::string bind)
{
	return glfwGetKey(window, keybinds[bind]);
}

void InputHandler::update()
{
	glfwPollEvents();

	mouseX1 = mouseX2;
	mouseY1 = mouseY2;

	glfwGetCursorPos(window, &mouseX2, &mouseY2);
}

double InputHandler::readMouseX()
{
	return (mouseX2 - mouseX1) * mouseSensitivity;
}

double InputHandler::readMouseY()
{
	return (mouseY2 - mouseY1) * mouseSensitivity;
}