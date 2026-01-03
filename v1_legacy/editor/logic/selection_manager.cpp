#include "selection_manager.h"

namespace CHEngine
{
SelectionManager *s_SelectionInstance = nullptr;

SelectionManager &SelectionManager::Get()
{
    return *s_SelectionInstance;
}

void SelectionManager::Init()
{
    if (!s_SelectionInstance)
        s_SelectionInstance = new SelectionManager();
}

void SelectionManager::Shutdown()
{
    delete s_SelectionInstance;
    s_SelectionInstance = nullptr;
}
} // namespace CHEngine
