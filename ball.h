#pragma once
#include "entity.h"

class Ball : public Entity
{
	void tick(float delta);

	float rollSpeed = 8.0;
};