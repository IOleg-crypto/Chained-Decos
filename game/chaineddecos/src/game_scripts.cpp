#include "engine/scene/script_registry.h"
#include "runtime/runtime_application.h"

#include "cameracontroller.h"
#include "orbitcameracontroller.h"
#include "exitscript.h"
#include "playercontroller.h"
#include "playergui.h"
#include "scenescript.h"
#include "spawnzone.h"

namespace CHEngine
{
    void RegisterGameScripts()
    {
        CH_CORE_INFO("ChainedDecos: Registering Project Scripts...");

        ScriptRegistry::Register<PlayerController>("PlayerController");
        ScriptRegistry::Register<CameraController>("CameraController");
        ScriptRegistry::Register<OrbitCameraController>("OrbitCameraController");
        ScriptRegistry::Register<GameHUD>("GameHUD");
        ScriptRegistry::Register<SpawnZoneRespawn>("SpawnZoneRespawn");
        ScriptRegistry::Register<ExitScript>("ExitScript");
        ScriptRegistry::Register<SceneScript>("SceneScript");
    }
} // namespace CHEngine
