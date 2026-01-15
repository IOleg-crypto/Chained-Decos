#include "CameraController.h"
#include "PlayerController.h"
#include "engine/scene/script_registry.h"


namespace CHEngine
{
void RegisterGameScripts()
{
    ScriptRegistry::Register<PlayerController>("PlayerController");
    ScriptRegistry::Register<CameraController>("CameraController");
}
} // namespace CHEngine
