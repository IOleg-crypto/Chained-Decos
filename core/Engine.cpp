#include "Engine.h"
#include "components/audio/Core/AudioManager.h"
#include "components/input/Core/InputManager.h"
#include "components/physics/collision/Core/CollisionManager.h"
#include "components/rendering/Core/RenderManager.h"
#include "core/module/ModuleManager.h"
#include "project/chaineddecos/gamegui/Menu.h"
#include "project/chaineddecos/player/Core/Player.h"
#include "scene/main/Core/World.h"
#include "scene/resources/model/Core/Model.h"
#include <memory>
#include <raylib.h>
#include <stdexcept>

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
    m_ModuleManager = std::make_unique<ModuleManager>();
}

Engine::~Engine()
{
    Shutdown();
    s_instance = nullptr;
}

bool Engine::Initialize()
{
    TraceLog(LOG_INFO, "[Engine] Initializing Core Services...");

    // 1. Rendering
    m_RenderManager = std::shared_ptr<RenderManager>(&RenderManager::Get(), [](RenderManager *) {});
    RegisterService<RenderManager>(m_RenderManager);

    // 2. Input
    m_InputManager = std::shared_ptr<ChainedDecos::InputManager>(
        &ChainedDecos::InputManager::Get(), [](ChainedDecos::InputManager *) {});
    RegisterService<ChainedDecos::InputManager>(m_InputManager);

    // 3. Audio
    m_AudioManager = std::shared_ptr<AudioManager>(&AudioManager::Get(), [](AudioManager *) {});
    RegisterService<AudioManager>(m_AudioManager);

    // 4. Physics
    m_CollisionManager = std::shared_ptr<ICollisionManager>(new CollisionManager());
    RegisterService<ICollisionManager>(m_CollisionManager);

    // 5. Resources
    m_ModelLoader = std::shared_ptr<IModelLoader>(new ModelLoader());
    RegisterService<IModelLoader>(m_ModelLoader);

    // 6. World
    m_WorldManager = std::shared_ptr<IWorldManager>(new WorldManager());
    RegisterService<IWorldManager>(m_WorldManager);

    TraceLog(LOG_INFO, "[Engine] Engine initialized successfully");
    return true;
}

void Engine::Update(float deltaTime)
{
    if (m_ModuleManager)
    {
        m_ModuleManager->UpdateAllModules(deltaTime);
    }
}

void Engine::Shutdown()
{
    TraceLog(LOG_INFO, "[Engine] Shutting down Core Services...");

    if (m_ModuleManager)
        m_ModuleManager->ShutdownAllModules();

    m_WorldManager.reset();
    m_ModelLoader.reset();
    m_CollisionManager.reset();

    if (m_AudioManager)
        m_AudioManager->Shutdown();
    m_AudioManager.reset();

    if (m_InputManager)
        m_InputManager->Shutdown();
    m_InputManager.reset();

    if (m_RenderManager)
        m_RenderManager->Shutdown();
    m_RenderManager.reset();
}

void Engine::RegisterModule(std::unique_ptr<IEngineModule> module)
{
    if (m_ModuleManager)
    {
        m_ModuleManager->RegisterModule(std::move(module));
    }
}

bool Engine::IsCollisionDebugVisible() const
{
    return RenderManager::Get().IsCollisionDebugVisible();
}

bool Engine::ShouldExit() const
{
    return m_shouldExit || WindowShouldClose();
}
std::shared_ptr<RenderManager> Engine::GetRenderManager() const
{
    return m_RenderManager;
}
std::shared_ptr<AudioManager> Engine::GetAudioManager() const
{
    return m_AudioManager;
}
std::shared_ptr<IModelLoader> Engine::GetModelLoader() const
{
    return m_ModelLoader;
}
std::shared_ptr<ICollisionManager> Engine::GetCollisionManager() const
{
    return m_CollisionManager;
}
std::shared_ptr<IWorldManager> Engine::GetWorldManager() const
{
    return m_WorldManager;
}
std::shared_ptr<ILevelManager> Engine::GetLevelManager() const
{
    return m_LevelManager;
}
std::shared_ptr<IPlayer> Engine::GetPlayer() const
{
    return m_Player;
}
std::shared_ptr<IMenu> Engine::GetMenu() const
{
    return m_Menu;
}
ModuleManager *Engine::GetModuleManager() const
{
    return m_ModuleManager.get();
}
bool Engine::IsDebugInfoVisible() const
{
    return m_debugInfoVisible;
}
void Engine::SetDebugInfoVisible(bool visible)
{
    m_debugInfoVisible = visible;
}
void Engine::RequestExit()
{
    m_shouldExit = true;
}
