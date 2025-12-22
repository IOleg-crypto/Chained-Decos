#ifndef IENGINE_H
#define IENGINE_H

// #include "ILevelManager.h"
// #include "IMenu.h"
// #include "IPlayer.h"

#include <memory>

class IModuleManager;

// Engine interface for dependency injection
class IEngine
{
public:
    virtual ~IEngine() = default;

    // Generic Service Access - implementation defined by Engine
    // virtual std::shared_ptr<IPlayer> GetPlayer() const = 0; // Removed
    // virtual std::shared_ptr<ILevelManager> GetLevelManager() const = 0; // Removed
    // virtual std::shared_ptr<IMenu> GetMenu() const = 0; // Removed

    virtual void RequestExit() = 0;
    virtual bool ShouldExit() const = 0;
};

#endif // IENGINE_H
