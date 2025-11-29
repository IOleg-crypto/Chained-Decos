#ifndef CONSOLE_MANAGER_HELPERS_H
#define CONSOLE_MANAGER_HELPERS_H

#include "core/engine/Engine.h"

// Helper function to update ConsoleManager providers via Dependency Injection
// Used in EngineApplication, MapSystem, PlayerSystem after service registration
void UpdateConsoleManagerProviders(Engine *engine);

#endif // CONSOLE_MANAGER_HELPERS_H
