#ifndef ISERVICE_PROVIDER_H
#define ISERVICE_PROVIDER_H

// IServiceProvider - Interface for accessing engine services
// Modules depend on THIS interface, NOT on Engine directly

#include "components/audio/Core/AudioManager.h"
#include "components/input/Core/InputManager.h"
#include "components/rendering/Core/RenderManager.h"

// ✅ Use INTERFACES instead of concrete classes - NO forward declarations!
#include "components/physics/collision/Interfaces/ICollisionManager.h"
#include "scene/main/Interfaces/IWorldManager.h"
#include "scene/resources/model/Interfaces/IModelLoader.h"

/**
 * @brief Service Provider Interface
 *
 * Provides access to core engine services via lightweight interfaces.
 * Modules depend on this interface instead of concrete Engine class.
 * This improves testability and reduces coupling.
 */
class IServiceProvider
{
public:
    virtual ~IServiceProvider() = default;

    // Core Managers
    virtual InputManager *GetInputManager() = 0;
    virtual RenderManager *GetRenderManager() = 0;

    // Services (via interfaces - ZERO forward declarations!)
    virtual AudioManager *GetAudioManager() = 0;
    virtual IModelLoader *GetModelLoader() = 0;
    virtual ICollisionManager *GetCollisionManager() = 0;
    virtual IWorldManager *GetWorldManager() = 0;
};

#endif // ISERVICE_PROVIDER_H
