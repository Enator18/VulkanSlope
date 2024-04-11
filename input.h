#pragma once
#include <unordered_map>
#include <string>

struct GLFWwindow;
class InputHandler
{
public:
	bool readBind(std::string bind);
	void update();
	double readMouseX();
	double readMouseY();
	InputHandler(GLFWwindow* window);
private:
	GLFWwindow* window;
	std::unordered_map<std::string, int> keybinds;
	float mouseSensitivity = 0.1f;

	double mouseX1, mouseY1, mouseX2, mouseY2;
};
