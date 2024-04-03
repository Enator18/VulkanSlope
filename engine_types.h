#pragma once
#include <vector>
#include <memory>

#include "math_utils.h"
#include "entity.h"

struct Scene
{
	Transform cameraTransform;
	std::vector<std::unique_ptr<Entity>> entities;
};