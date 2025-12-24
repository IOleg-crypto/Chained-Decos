#ifndef IENGINE_H
#define IENGINE_H

// #include "ILevelManager.h"
// #include "IMenu.h"
// #include "IPlayer.h"

#include <memory>

#include <entt/entt.hpp>
#include <memory>

// Manager Interfaces
#include "components/audio/interfaces/IAudioManager.h"
#include "components/input/interfaces/IInputManager.h"
#include "components/physics/collision/interfaces/ICollisionManager.h"
#include "components/rendering/core/RenderManager.h"
#include "core/interfaces/IGuiManager.h"
#include "events/UIEventRegistry.h"
#include "scene/SceneManager.h"
#include "scene/main/interfaces/IWorldManager.h"
#include "scene/resources/font/FontService.h"
#include "scene/resources/model/interfaces/IModelLoader.h"
#include "scene/resources/texture/TextureService.h"

#include "../ServiceRegistry.h"

// Engine interface for dependency injection
class IEngine
{
public:
    virtual ~IEngine() = default;

    // Manager Service Access
    virtual RenderManager &GetRenderManager() const = 0;
    virtual IInputManager &GetInputManager() const = 0;
    virtual IAudioManager &GetAudioManager() const = 0;
    virtual IModelLoader &GetModelLoader() const = 0;
    virtual ICollisionManager &GetCollisionManager() const = 0;
    virtual IWorldManager &GetWorldManager() const = 0;
    virtual IGuiManager &GetGuiManager() const = 0;
    virtual CHEngine::SceneManager &GetSceneManager() const = 0;
    virtual CHEngine::FontService &GetFontService() const = 0;
    virtual CHEngine::TextureService &GetTextureService() const = 0;
    virtual CHEngine::UIEventRegistry &GetUIEventRegistry() const = 0;
    virtual entt::registry &GetECSRegistry() = 0;

    // Generic Service Locator
    template <typename T> std::shared_ptr<T> GetService() const
    {
        return CHEngine::ServiceRegistry::Get<T>();
    }

    template <typename T> void RegisterService(std::shared_ptr<T> service)
    {
        CHEngine::ServiceRegistry::Register<T>(service);
    }

    virtual void RequestExit() = 0;
    virtual bool ShouldExit() const = 0;
};

#endif // IENGINE_H
