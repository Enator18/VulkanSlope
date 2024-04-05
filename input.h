#pragma once
#include <unordered_map>
#include <string> 

class InputHandler
{
public:
	bool readBind(std::string bind);
	InputHandler(void* window);
private:
	void* window;
	std::unordered_map<std::string, int> keybinds;
};
