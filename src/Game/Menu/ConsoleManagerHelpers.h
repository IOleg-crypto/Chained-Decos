#ifndef CONSOLE_MANAGER_HELPERS_H
#define CONSOLE_MANAGER_HELPERS_H

#include "Engine/Kernel/Kernel.h"
#include "Engine/Kernel/KernelServices.h"
#include "Menu.h"

// Helper function to update ConsoleManager providers via Dependency Injection
// Used in EngineApplication, MapSystem, PlayerSystem after service registration
inline void UpdateConsoleManagerProviders(Kernel* kernel)
{
    if (!kernel) return;
    
    auto menuService = kernel->GetService<MenuService>(Kernel::ServiceType::Menu);
    if (!menuService || !menuService->menu) return;
    
    auto consoleManager = menuService->menu->GetConsoleManager();
    if (!consoleManager) return;
    
    // Get current providers
    auto playerService = kernel->GetService<PlayerService>(Kernel::ServiceType::Player);
    auto mapService = kernel->GetService<MapManagerService>(Kernel::ServiceType::MapManager);
    auto engineService = kernel->GetService<EngineService>(Kernel::ServiceType::Engine);
    
    // Update providers (may be nullptr if services not yet registered)
    consoleManager->SetProviders(
        playerService ? playerService.get() : nullptr,
        mapService ? mapService.get() : nullptr,
        engineService ? engineService.get() : nullptr
    );
}

#endif // CONSOLE_MANAGER_HELPERS_H


