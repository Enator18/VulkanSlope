#include <iostream>

#include "entity.h"
#include "game_main.h"

void Entity::begin()
{
	
}

void Entity::tick(float delta)
{
	
}

void Entity::setCameraTransform(Transform transform)
{
	game->getCurrentScene().cameraTransform = transform;
}