#include "engine/core/game_entry_point.h"
#include "engine/scene/scene.h"
#include "engine/scene/script_registry.h"

// Include all game scripts here
#include "playercontroller.h"
#include "cameracontroller.h"
#include "playergui.h"
#include "scenescript.h"
#include "exitscript.h"
#include "spawnzone.h"
#include "orbitcameracontroller.h"

// Standard C++ function for static linking
void RegisterGameScripts(CHEngine::Scene* scene) {
    if (!scene) return;

    CH_CORE_INFO("Game Logic Registering...");

    // Register Scripts
    auto& registry = scene->GetScriptRegistry();
    
    registry.Register<CHEngine::PlayerController>("PlayerController");
    registry.Register<CHEngine::CameraController>("CameraController");
    registry.Register<CHEngine::GameHUD>("GameHUD");
    registry.Register<CHEngine::SceneScript>("SceneScript");
    registry.Register<CHEngine::ExitScript>("ExitScript");
    registry.Register<CHEngine::SpawnZoneRespawn>("SpawnZoneRespawn");
    registry.Register<CHEngine::OrbitCameraController>("OrbitCameraController");

    CH_CORE_INFO("Game Scripts Registered successfully!");
}
