#include "Engine/Engine.h"
#include "Game/Game.h"
#include "Engine/CommandLineHandler/CommandLineHandler.h"

// # --------
// # int main - init game
// # --------
int main(int argc, char* argv[])
{
    // Parse command line arguments
    GameConfig config = CommandLineHandler::ParseArguments(argc, argv);

    // Show parsed configuration in developer mode
    if (config.developer)
    {
        CommandLineHandler::ShowConfig(config);
    }

    // Initialize engine with parsed configuration
    Engine engine(config.width, config.height);

    // Apply configuration to engine
    CommandLineHandler::ApplyConfigToEngine(config);

    Game game(engine);

    game.Init();
    game.Run();

    return 0;
}