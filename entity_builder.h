#pragma once
#include <functional>
#include <unordered_map>
#include <string>
#include <memory>

#include "entity.h"
#include "spinner.h"

std::unordered_map<std::string, std::function<std::unique_ptr<Entity>()>> entityBuilder =
{
	{"Entity", [=]() {return std::make_unique<Entity>(); }},
	{"Spinner", [=]() {return std::make_unique<Spinner>(); }}
};
