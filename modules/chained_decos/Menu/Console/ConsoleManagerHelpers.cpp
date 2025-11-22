#include "ConsoleManagerHelpers.h"
#include "core/object/kernel/Core/Kernel.h"
#include "Menu.h"

// Helper function to update ConsoleManager providers via Dependency Injection
// Used in EngineApplication, MapSystem, PlayerSystem after service registration
// NOTE: ConsoleManager now accesses services directly through Kernel::Instance(),
// so this function is now a no-op but kept for backward compatibility
void UpdateConsoleManagerProviders(Kernel *kernel)
{
    // ConsoleManager now accesses all services directly through Kernel::Instance()
    // No need to inject providers anymore
    (void)kernel; // Suppress unused parameter warning

    TraceLog(LOG_INFO,
             "UpdateConsoleManagerProviders() - Using Kernel::Instance() for service access");
}
