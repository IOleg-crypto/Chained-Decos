#include "CameraController.h"
#include "PlayerController.h"
#include "UIExample.h"
#include "engine/scene/script_registry.h"

namespace CHEngine
{
void RegisterGameScripts()
{
    ScriptRegistry::Register<PlayerController>("PlayerController");
    ScriptRegistry::Register<CameraController>("CameraController");
    ScriptRegistry::Register<CHEmple::GameHUD>("GameHUD");
}
} // namespace CHEngine
