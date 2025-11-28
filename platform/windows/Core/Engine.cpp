#include "Engine.h"
#include "core/object/kernel/Core/Kernel.h"
#include "core/object/module/Core/ModuleManager.h"
#include "servers/input/Core/InputManager.h"
#include "servers/rendering/Core/RenderManager.h"
#include <memory>

Engine::Engine(Kernel *kernel) : m_kernel(kernel)
{
    m_moduleManager = std::make_unique<ModuleManager>(kernel);
}

Engine::~Engine()
{
    Shutdown();
}

bool Engine::Initialize()
{
    // Create and initialize RenderManager
    m_renderManager = std::make_unique<RenderManager>();
    if (!m_renderManager->Initialize())
    {
        TraceLog(LOG_ERROR, "[Engine] Failed to initialize RenderManager");
        return false;
    }

    // Create and initialize InputManager
    m_inputManager = std::make_unique<InputManager>();
    if (!m_inputManager->Initialize())
    {
        TraceLog(LOG_ERROR, "[Engine] Failed to initialize InputManager");
        return false;
    }

    TraceLog(LOG_INFO, "[Engine] Engine initialized successfully");
    return true;
}

void Engine::Update(float deltaTime)
{
    if (m_moduleManager)
    {
        m_moduleManager->UpdateAllModules(deltaTime);
    }
}

void Engine::Shutdown()
{
    if (m_moduleManager)
    {
        m_moduleManager->ShutdownAllModules();
    }
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
