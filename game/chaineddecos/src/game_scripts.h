#pragma once

namespace CHEngine { class Scene; }

// Forward declaration of the game-specific script registration function.
// This function is defined in game_module.cpp and used by the Editor and Standalone Runtime.
void RegisterGameScripts(CHEngine::Scene* scene);
