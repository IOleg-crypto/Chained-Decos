#include "GameApplication.h"
#include "core/engine/EngineApplication.h"

int main(int argc, char *argv[])
{
    GameApplication gameApp(argc, argv);
    EngineApplication::Config engineConfig;
    engineConfig.windowName = "Chained Decos";
    EngineApplication engine(engineConfig, &gameApp);
    engine.Run();
    return 0;
}