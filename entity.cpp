#include "entity.h"

void Entity::begin()
{

}

void Entity::tick(float delta)
{

}

void Entity::setName(std::string name)
{
	this->name = name;
}

std::string Entity::getName()
{
	return name;
}

void Entity::setTransform(Transform transform)
{
	this->transform = transform;
}