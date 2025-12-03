#include "Engine.h"
#include "core/object/module/Core/ModuleManager.h"
#include "project/chaineddecos/Menu/Menu.h"
#include "project/chaineddecos/Player/Core/Player.h"
#include "project/chaineddecos/Systems/MapSystem/LevelManager.h"
#include "scene/main/Core/World.h"
#include "scene/resources/model/Core/Model.h"
#include "servers/audio/Core/AudioManager.h"
#include "servers/input/Core/InputManager.h"
#include "servers/physics/collision/Core/CollisionManager.h"
#include "servers/rendering/Core/RenderManager.h"
#include <memory>
#include <raylib.h> // For TraceLog


Engine *Engine::s_instance = nullptr;

Engine &Engine::Instance()
{
    if (!s_instance)
    {
        throw std::runtime_error("Engine not initialized!");
    }
    return *s_instance;
}

Engine::Engine()
{
    s_instance = this;
    m_moduleManager = std::make_unique<ModuleManager>();
}

Engine::~Engine()
{
    Shutdown();
    s_instance = nullptr;
}

bool Engine::Initialize()
{
    // Managers are now Static Singletons initialized by GameApplication
    // or lazily initialized on first access (though explicit init is preferred)

    TraceLog(LOG_INFO, "[Engine] Engine initialized successfully");
    return true;
}

void Engine::Update(float deltaTime)
{
    // InputManager is updated by GameApplication

    if (m_moduleManager)
    {
        m_moduleManager->UpdateAllModules(deltaTime);
    }
}

void Engine::Shutdown()
{
    if (m_moduleManager)
        m_moduleManager->ShutdownAllModules();

    // RenderManager and InputManager are shutdown by GameApplication
}

RenderManager *Engine::GetRenderManager()
{
    return &RenderManager::Get();
}

InputManager *Engine::GetInputManager()
{
    return &InputManager::Get();
}

AudioManager *Engine::GetAudioManager()
{
    return GetService<AudioManager>().get();
}

IModelLoader *Engine::GetModelLoader()
{
    return GetService<ModelLoader>().get();
}

ICollisionManager *Engine::GetCollisionManager()
{
    return GetService<CollisionManager>().get();
}

IWorldManager *Engine::GetWorldManager()
{
    return GetService<WorldManager>().get();
}

void Engine::RegisterModule(std::unique_ptr<IEngineModule> module)
{
    if (m_moduleManager)
    {
        m_moduleManager->RegisterModule(std::move(module));
    }
}

bool Engine::IsCollisionDebugVisible() const
{
    return RenderManager::Get().IsCollisionDebugVisible();
}

// Direct access to game objects (replaces service wrappers)
// These use the service locator but return raw pointers for convenience
IPlayer *Engine::GetPlayer() const
{
    auto service = GetService<Player>();
    return service.get();
}

ILevelManager *Engine::GetLevelManager() const
{
    auto service = GetService<LevelManager>();
    return service.get();
}

IMenu *Engine::GetMenu() const
{
    auto service = GetService<Menu>();
    return service.get();
}
