#include "Engine.h"
#include "core/object/module/Core/ModuleManager.h"
#include "servers/input/Core/InputManager.h"
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
    m_moduleManager = std::make_unique<ModuleManager>(nullptr);
}

Engine::~Engine()
{
    Shutdown();
    s_instance = nullptr;
}

bool Engine::Initialize()
{
    // 1. Create and Initialize InputManager
    auto inputManager = std::make_unique<InputManager>();
    if (!inputManager->Initialize())
    {
        TraceLog(LOG_ERROR, "[Engine] Failed to initialize InputManager");
        return false;
    }
    m_inputManager = std::move(inputManager);

    // 2. Create and Initialize RenderManager
    auto renderManager = std::make_unique<RenderManager>();
    if (!renderManager->Initialize())
    {
        TraceLog(LOG_ERROR, "[Engine] Failed to initialize RenderManager");
        return false;
    }
    m_renderManager = std::move(renderManager);

    TraceLog(LOG_INFO, "[Engine] Engine initialized successfully");
    return true;
}

void Engine::Update(float deltaTime)
{
    if (m_inputManager)
        m_inputManager->Update(deltaTime);

    if (m_moduleManager)
    {
        m_moduleManager->UpdateAllModules(deltaTime);
    }
}

void Engine::Shutdown()
{
    if (m_moduleManager)
        m_moduleManager->ShutdownAllModules();
    if (m_renderManager)
        m_renderManager->Shutdown();
    if (m_inputManager)
        m_inputManager->Shutdown();
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
    if (m_renderManager)
    {
        return m_renderManager->IsCollisionDebugVisible();
    }
    return false;
}

// Direct access to game objects (replaces service wrappers)
// These use the service locator but return raw pointers for convenience
Player *Engine::GetPlayer() const
{
    auto service = GetService<Player>();
    return service.get();
}

PlayerController *Engine::GetPlayerController() const
{
    auto service = GetService<PlayerController>();
    return service.get();
}

LevelManager *Engine::GetLevelManager() const
{
    auto service = GetService<LevelManager>();
    return service.get();
}

Menu *Engine::GetMenu() const
{
    auto service = GetService<Menu>();
    return service.get();
}
