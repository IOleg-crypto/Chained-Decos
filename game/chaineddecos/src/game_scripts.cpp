#include "CameraController.h"
#include "ExitScript.h"
#include "PlayerController.h"
#include "SpawnZone.h"
#include "UIExample.h"
#include "engine/scene/script_registry.h"

namespace CHEngine
{
void RegisterGameScripts()
{
    // Example: Launching a scene without editor via code
    // Application::SetStartupScene("assets/scenes/start menu.chscene");

    ScriptRegistry::Register<PlayerController>("PlayerController");
    ScriptRegistry::Register<CameraController>("CameraController");
    ScriptRegistry::Register<GameHUD>("GameHUD");
    ScriptRegistry::Register<SpawnZoneRespawn>("SpawnZoneRespawn");
    ScriptRegistry::Register<ExitScript>("ExitScript");
}
} // namespace CHEngine
