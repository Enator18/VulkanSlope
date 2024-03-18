#pragma once
#include <vector>

#include "mesh.h"
#include "math_utils.h"


class Entity
{
public:
	virtual void begin();
	virtual void tick(float delta);
private:
	Transform transform;
	MeshInstance mesh;
};