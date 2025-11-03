#include "GameApplication.h"

int main(int argc, char* argv[])
{
    GameApplication app(argc, argv);
    app.Run();  // Весь життєвий цикл в базовому класі EngineApplication!
    return 0;
}