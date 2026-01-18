#include "editor.h"
#include "editor_layer.h"
#include "editor_settings.h"
#include "engine/renderer/asset_manager.h"
#include <raylib.h>

namespace CHEngine
{
Editor::Editor(const Application::Config &config) : Application(config)
{
    CH_CORE_INFO("Editor Started");
    SetWindowIcon(
        LoadImage(AssetManager::ResolvePath(
                      "engine:icons/game-engine-icon-featuring-a-game-controller-with-.png")
                      .string()
                      .c_str()));
    PushLayer(new EditorLayer());
}

} // namespace CHEngine
