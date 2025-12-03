// Generic entry point for Map Editor
#include "EditorApplication.h"
#include "main/EntryPoint.h"


// EditorApplication doesn't take argc/argv
int main()
{
    EditorApplication app;
    EngineApplication::Config config;
    config.windowName = "Chained Decos Map Editor";
    EngineApplication engine(config, &app);
    engine.Run();
    return 0;
}