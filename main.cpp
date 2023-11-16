#include "game_main.h"
int main()
{
	SlopeGame game = SlopeGame();
	game.init();
	while (game.tick());
	game.cleanup();
	return 0;
}