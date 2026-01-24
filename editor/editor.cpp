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
}

void Editor::PostInitialize()
{
    CH_CORE_INFO("Editor PostInitialize - Setting window icon");

    // Set window icon (safe to do after window is created)
    Image icon =
        LoadImage(AssetManager::ResolvePath(
                      "engine:icons/game-engine-icon-featuring-a-game-controller-with-.png")
                      .string()
                      .c_str());
    SetWindowIcon(icon);
    UnloadImage(icon);

    CH_CORE_INFO("Editor PostInitialize - Pushing EditorLayer");
    PushLayer(new EditorLayer());
}

} // namespace CHEngine
