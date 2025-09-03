#include "Engine/Engine.h"
#include "Game/Game.h"

// # --------
// # int main - init game
// # --------
int main()
{
    Engine engine(1280, 720);
    Game game(engine);

    game.Init();
    game.Run();

    return 0;
}