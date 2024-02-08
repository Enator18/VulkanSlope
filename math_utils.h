#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

struct Transform
{
	glm::vec3 position = glm::vec3(0, 0, 0);
	glm::vec3 rotation;
	glm::vec3 scale = glm::vec3(1, 1, 1);

	glm::mat4 getTransformMatrix();
	glm::mat4 getRotationMatrix();
	glm::vec3 getForwardVector();
};