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
	return glm::mat4_cast(glm::quat(rotation));
}

glm::vec3 Transform::getForwardVector()
{
	return glm::vec4(1.0, 0.0, 0.0, 1.0) * getRotationMatrix();
}