#include "slope_game.h"
int main()
{
	SlopeGame game = SlopeGame();
	game.init();
	while (game.tick());
	game.cleanup();
	return 0;
}