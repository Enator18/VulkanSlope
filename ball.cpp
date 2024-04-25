#include <glm/common.hpp>

#include "ball.h"
#include "math_utils.h"

void Ball::tick(float delta)
{
	transform.rotation.x += 180 * delta;

	transform.position.x += cos(22.5 * 3.14159 / 180) * rollSpeed * delta;
	transform.position.z -= sin(22.5 * 3.14159 / 180) * rollSpeed * delta;

	Transform cameraTransform;
	cameraTransform.position = transform.position;
	cameraTransform.position += glm::vec3(-8.0, 0.0, 8.0);
	cameraTransform.rotation.x -= 45.0;

	setCameraTransform(cameraTransform);
}