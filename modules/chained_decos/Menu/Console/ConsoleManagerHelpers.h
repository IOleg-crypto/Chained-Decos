#ifndef CONSOLE_MANAGER_HELPERS_H
#define CONSOLE_MANAGER_HELPERS_H

#include "Engine/Kernel/Core/Kernel.h"

// Helper function to update ConsoleManager providers via Dependency Injection
// Used in EngineApplication, MapSystem, PlayerSystem after service registration
void UpdateConsoleManagerProviders(Kernel* kernel);

#endif // CONSOLE_MANAGER_HELPERS_H
