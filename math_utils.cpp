#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "math_utils.h"

glm::mat4 Transform::getTransformMatrix()
{
	return glm::scale(glm::translate(glm::mat4(1.0f), position) * getRotationMatrix(), scale);
}

glm::mat4 Transform::getRotationMatrix()
{
	return glm::mat4_cast(quatFromEulerAngles(rotation));
}

glm::vec3 Transform::getForwardVector()
{
	return glm::vec4(1.0, 0.0, 0.0, 1.0) * getRotationMatrix();
}

glm::quat quatFromEulerAngles(glm::vec3 angles)
{
	glm::quat aroundX = glm::angleAxis(glm::radians(angles.z), glm::vec3(1.0, 0.0, 0.0));
	glm::quat aroundY = glm::angleAxis(glm::radians(angles.x), glm::vec3(0.0, 1.0, 0.0));
	glm::quat aroundZ = glm::angleAxis(glm::radians(angles.y), glm::vec3(0.0, 0.0, 1.0));

	return aroundY * aroundZ * aroundX;
}