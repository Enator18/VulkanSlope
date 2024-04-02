#include "spinner.h"

void Spinner::tick(float delta)
{
	transform.rotation.y += delta * 90;
}