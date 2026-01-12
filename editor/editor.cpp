#include "editor.h"
#include "editor_layer.h"
#include <raylib.h>

namespace CHEngine
{
Editor::Editor(const Application::Config &config) : Application(config)
{
    CH_CORE_INFO("Editor Started");
    SetWindowIcon(
        LoadImage(PROJECT_ROOT_DIR
                  "/resources/icons/game-engine-icon-featuring-a-game-controller-with-.png"));
    PushLayer(new EditorLayer());
}

} // namespace CHEngine
