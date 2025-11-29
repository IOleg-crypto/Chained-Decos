#include "ConsoleManagerHelpers.h"
#include "Menu.h"
#include "core/engine/Engine.h"


// Helper function to update ConsoleManager providers via Dependency Injection
// Used in EngineApplication, MapSystem, PlayerSystem after service registration
// NOTE: ConsoleManager now accesses services directly through Engine::Instance(),
// so this function is now a no-op but kept for backward compatibility
void UpdateConsoleManagerProviders(Engine *engine)
{
    // ConsoleManager now accesses all services directly through Engine::Instance()
    // No need to inject providers anymore
    (void)engine; // Suppress unused parameter warning

    TraceLog(LOG_INFO,
             "UpdateConsoleManagerProviders() - Using Engine::Instance() for service access");
}
