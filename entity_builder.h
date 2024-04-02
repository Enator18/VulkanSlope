#pragma once
#include <functional>
#include <unordered_map>
#include <string>
#include <memory>

#include "entity.h"

std::unordered_map<std::string, std::function<std::unique_ptr<Entity>()>> entityBuilder =
{
	{"Entity", std::make_unique<Entity>}
};
