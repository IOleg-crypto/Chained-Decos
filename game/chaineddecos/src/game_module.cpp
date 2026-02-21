#include "engine/core/game_entry_point.h"
#include "engine/core/log.h"
#include "engine/scene/components/scripting_components.h"
#include "engine/scene/scene.h"
#include "engine/scene/script_registry.h"

// Include all game scripts here
#include "cameracontroller.h"
#include "exitscript.h"
#include "orbitcameracontroller.h"
#include "player_fall.h"
#include "playercontroller.h"
#include "playergui.h"
#include "scenescript.h"
#include "screen_fall_effect.h"
#include "settings_script.h"
#include "spawnzone.h"

// Standard C++ function for static linking (used by Editor)
void RegisterGameScripts(CHEngine::Scene* scene)
{
    if (!scene)
    {
        CH_CORE_ERROR("RegisterGameScripts: SCENE IS NULL!");
        return;
    }
    CH_CORE_INFO("RegisterGameScripts: Called for scene '{}' (ptr: {})", scene->GetSettings().Name, (void*)scene);

    auto& registry = scene->GetScriptRegistry();

    registry.Register<CHEngine::PlayerController>("PlayerController");
    registry.Register<CHEngine::CameraController>("CameraController");
    registry.Register<CHEngine::GameHUD>("GameHUD");
    registry.Register<CHEngine::SceneScript>("SceneScript");
    registry.Register<CHEngine::ExitScript>("ExitScript");
    registry.Register<CHEngine::SpawnZoneRespawn>("SpawnZoneRespawn");
    registry.Register<CHEngine::OrbitCameraController>("OrbitCameraController");
    registry.Register<CHEngine::PlayerFall>("PlayerFall");
    //registry.Register<CHEngine::ScreenFallEffect>("ScreenFallEffect");
    registry.Register<CHEngine::SettingsScript>("SettingsScript");

    CH_CORE_INFO("Game Scripts Registered successfully!");
}

extern "C" {
#ifdef GAME_BUILD_DLL
// ABI-safe: DLL calls the Engine's callback to register scripts.
// No C++ STL types cross the DLL boundary.
CH_GAME_API void LoadGame(CH_RegisterScriptCallback registerCallback, void* userData)
{
    CH_CORE_INFO("Game Logic Registering (dynamic)...");

    CH_REGISTER_SCRIPT(registerCallback, userData, CHEngine::PlayerController, "PlayerController");
    CH_REGISTER_SCRIPT(registerCallback, userData, CHEngine::CameraController, "CameraController");
    CH_REGISTER_SCRIPT(registerCallback, userData, CHEngine::GameHUD, "GameHUD");
    CH_REGISTER_SCRIPT(registerCallback, userData, CHEngine::SceneScript, "SceneScript");
    CH_REGISTER_SCRIPT(registerCallback, userData, CHEngine::ExitScript, "ExitScript");
    CH_REGISTER_SCRIPT(registerCallback, userData, CHEngine::SpawnZoneRespawn, "SpawnZoneRespawn");
    CH_REGISTER_SCRIPT(registerCallback, userData, CHEngine::OrbitCameraController, "OrbitCameraController");
    CH_REGISTER_SCRIPT(registerCallback, userData, CHEngine::PlayerFall, "PlayerFall");
    //CH_REGISTER_SCRIPT(registerCallback, userData, CHEngine::ScreenFallEffect, "ScreenFallEffect");
    CH_REGISTER_SCRIPT(registerCallback, userData, CHEngine::SettingsScript, "SettingsScript");

    CH_CORE_INFO("Game Scripts Registered successfully!");
}

CH_GAME_API void UnloadGame()
{
    CH_CORE_INFO("Game Logic Unloading...");
}
#else
// Static linking: use the standard C++ function
void LoadGame(CH_RegisterScriptCallback, void*)
{
    // Not used in static linking - RegisterGameScripts is called directly
}

void UnloadGame()
{
    CH_CORE_INFO("Game Logic Unloading...");
}
#endif
}
