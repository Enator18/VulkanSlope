#pragma once
#include <vector>
#include <string>
#include <functional>

#include "mesh.h"
#include "math_utils.h"


class Entity
{
public:
	virtual void begin();
	virtual void tick(float delta);
	std::string name;
	Transform transform;
	MeshInstance mesh;
};


struct EntityBuilder
{
	std::function<Entity()> buildFunction;
};