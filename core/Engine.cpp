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

    // 2. Input
    m_InputManager = std::shared_ptr<InputManager>(&InputManager::Get(), [](InputManager *) {});

    // 3. Audio
    m_AudioManager = std::shared_ptr<AudioManager>(&AudioManager::Get(), [](AudioManager *) {});

    // 4. Physics
    m_CollisionManager = std::shared_ptr<ICollisionManager>(new CollisionManager());

    // 5. Resources
    m_ModelLoader = std::shared_ptr<IModelLoader>(new ModelLoader());

    // 6. World
    m_WorldManager = std::shared_ptr<IWorldManager>(new WorldManager());

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




