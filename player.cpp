#include "player.h"
#include "game_main.h"
#include "input.h"
void Player::tick(float delta)
{
	InputHandler& inputHandler = game->inputHandler;

	transform.rotation.x -= inputHandler.readMouseY();
	transform.rotation.y -= inputHandler.readMouseX();

	transform.rotation.x = std::clamp(transform.rotation.x, -90.0f, 90.0f);

	if (inputHandler.readBind("forward"))
	{
		transform.position += glm::vec3(glm::vec4(flySpeed * delta, 0.0, 0.0, 1.0) * transform.getRotationMatrix());
	}

	if (inputHandler.readBind("left"))
	{
		transform.position += glm::vec3(glm::vec4(0.0, -flySpeed * delta, 0.0, 1.0) * transform.getRotationMatrix());
	}

	if (inputHandler.readBind("backward"))
	{
		transform.position += glm::vec3(glm::vec4(-flySpeed * delta, 0.0, 0.0, 1.0) * transform.getRotationMatrix());
	}

	if (inputHandler.readBind("right"))
	{
		transform.position += glm::vec3(glm::vec4(0.0, flySpeed * delta, 0.0, 1.0) * transform.getRotationMatrix());
	}

	game->getCurrentScene().cameraTransform = transform;
}