#pragma once
#include <vector>
#include <string>

#include "mesh.h"
#include "math_utils.h"


class Entity
{
public:
	virtual void begin();
	virtual void tick(float delta);
	void setName(std::string name);
	void setTransform(Transform transform);
	std::string getName();

private:
	std::string name;
	Transform transform;
	MeshInstance mesh;
};