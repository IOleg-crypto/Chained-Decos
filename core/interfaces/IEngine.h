#ifndef IENGINE_H
#define IENGINE_H

// #include "ILevelManager.h"
// #include "IMenu.h"
// #include "IPlayer.h"

#include <memory>

#include <memory>

// Manager Interfaces
#include "components/audio/interfaces/IAudioManager.h"
#include "components/input/interfaces/IInputManager.h"
#include "components/physics/collision/interfaces/ICollisionManager.h"
#include "components/rendering/core/RenderManager.h"
#include "core/interfaces/IGuiManager.h"
#include "scene/main/interfaces/IWorldManager.h"
#include "scene/resources/model/interfaces/IModelLoader.h"

#include "../ServiceRegistry.h"

// Engine interface for dependency injection
class IEngine
{
public:
    virtual ~IEngine() = default;

    // Manager Service Access
    virtual std::shared_ptr<RenderManager> GetRenderManager() const = 0;
    virtual std::shared_ptr<IInputManager> GetInputManager() const = 0;
    virtual std::shared_ptr<IAudioManager> GetAudioManager() const = 0;
    virtual std::shared_ptr<IModelLoader> GetModelLoader() const = 0;
    virtual std::shared_ptr<ICollisionManager> GetCollisionManager() const = 0;
    virtual std::shared_ptr<IWorldManager> GetWorldManager() const = 0;
    virtual std::shared_ptr<IGuiManager> GetGuiManager() const = 0;

    // Generic Service Locator
    template <typename T> std::shared_ptr<T> GetService() const
    {
        return ChainedEngine::ServiceRegistry::Get<T>();
    }

    template <typename T> void RegisterService(std::shared_ptr<T> service)
    {
        ChainedEngine::ServiceRegistry::Register<T>(service);
    }

    virtual void RequestExit() = 0;
    virtual bool ShouldExit() const = 0;
};

#endif // IENGINE_H
