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
    // Get managers from Kernel (they are registered by EngineApplication)
    auto renderMgr = m_kernel->GetService<RenderManager>();
    m_renderManager = std::make_unique<RenderManager>();

    auto inputMgr = m_kernel->GetService<InputManager>();
    m_inputManager = std::make_unique<InputManager>();

    // Initialize ModuleManager
    if (m_moduleManager)
    {
        // ModuleManager doesn't have Initialize, it has InitializeAllModules
    }
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
