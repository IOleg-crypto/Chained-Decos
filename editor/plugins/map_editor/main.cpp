#include "EditorApplication.h"
#include "platform/windows/Core/EngineApplication.h"

// # --------
// # int main - init map editor
// # --------
int main(int argc, char *argv[])
{
    EditorApplication editorApp;
    EngineApplication::Config engineConfig;
    engineConfig.windowName = "Chained Decos Map Editor";
    EngineApplication engine(engineConfig, &editorApp);
    engine.Run();
    return 0;
}