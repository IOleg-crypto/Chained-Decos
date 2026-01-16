#include "CameraController.h"
#include "PlayerController.h"
#include "PlayerControllerNew.h"
#include "engine/scene/script_registry.h"

namespace CHEngine
{
void RegisterGameScripts()
{
    ScriptRegistry::Register<PlayerController>("PlayerController");
    ScriptRegistry::Register<PlayerControllerNew>("PlayerControllerNew");
    ScriptRegistry::Register<CameraController>("CameraController");
}
} // namespace CHEngine
