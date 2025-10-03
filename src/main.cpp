#include "Engine/Engine.h"
#include "Game/Game.h"
#include "Engine/CommandLineHandler/CommandLineHandler.h"

int main(int argc, char* argv[])
{
    // Parse command line arguments
    GameConfig config = CommandLineHandler::ParseArguments(argc, argv);

    // Show parsed configuration in developer mode
    if (config.developer)
    {
        CommandLineHandler::ShowConfig(config);
    }

    // Create and initialize engine
    Engine engine(config.width, config.height);
    engine.Init();

    // Create and initialize game
    Game game(&engine);
    game.Init();
    game.Run();

    return 0;
}