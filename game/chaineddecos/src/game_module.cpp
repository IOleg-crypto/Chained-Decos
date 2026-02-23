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
#include "spawnzone.h"
#include "settings_script.h"

extern "C" {

// Standard C++ function for static linking (used by Editor and Standalone)
void RegisterGameScripts()
{
    CH_CORE_INFO("RegisterGameScripts: Registering game scripts to Global Registry");

    auto& registry = CHEngine::ScriptRegistry::GetGlobalRegistry();

    registry.Register<
        CHEngine::PlayerController,
        CHEngine::CameraController,
        CHEngine::GameHUD,
        CHEngine::SceneScript,
        CHEngine::ExitScript,
        CHEngine::SpawnZoneRespawn,
        CHEngine::OrbitCameraController,
        CHEngine::PlayerFall,
        CHEngine::SettingsScript
    >();

    CH_CORE_INFO("Game Scripts Registered successfully!");
}
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
