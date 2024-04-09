#pragma once
#include <vector>
#include <string>

#include "mesh.h"
#include "math_utils.h"


class SlopeGame;
class Entity
{
public:
	virtual void begin();
	virtual void tick(float delta);
	std::string name;
	Transform transform;
	MeshInstance mesh;
	SlopeGame* game;
private:
	void setCameraTransform(Transform transform);
};
