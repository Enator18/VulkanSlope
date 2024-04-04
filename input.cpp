#include "input.h"

bool InputHandler::readBind(std::string bind)
{
	return glfwGetKey(window, keybinds[bind]);
}