#pragma once
#include "entity.h"

class Player : public Entity
{
public:
	void tick(float delta);
private:
	float flySpeed = 2;
};