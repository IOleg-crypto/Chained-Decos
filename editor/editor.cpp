#include "editor.h"
#include "editor_layer.h"
#include <raylib.h>

namespace CH
{
Editor::Editor(const Application::Config &config) : Application(config)
{
    CH_CORE_INFO("Editor Started");
    SetWindowIcon(LoadImage(PROJECT_ROOT_DIR "/resources/icons/ChainedDecosMapEditor.png"));
    PushLayer(new EditorLayer());
}

Application *CreateApplication()
{
    Application::Config config;
    config.Title = "Chained Editor";
    config.Width = 1600;
    config.Height = 900;

    return new Editor(config);
}
} // namespace CH
