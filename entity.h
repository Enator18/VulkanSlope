#pragma once
#include <vector>

#include "mesh.h"
#include "math_utils.h"


class Entity
{
public:
	void begin();
	void tick();
private:
	Transform transform;
	MeshInstance mesh;
	std::vector<Entity> children;
	Entity* parent;
};