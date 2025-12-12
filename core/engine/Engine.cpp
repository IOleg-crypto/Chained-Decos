#include "Engine.h"
#include <raylib.h>

Engine::Engine(RenderManager &renderManager, InputManager &inputManager, AudioManager &audioManager)
    : m_renderManager(renderManager), m_inputManager(inputManager), m_audioManager(audioManager),
      m_debugInfoVisible(false), m_shouldExit(false)
{
    m_moduleManager = std::make_unique<ModuleManager>();
    TraceLog(LOG_INFO, "[Engine] Engine created with DI");
}

Engine::~Engine()
{
    Shutdown();
    TraceLog(LOG_INFO, "[Engine] Engine destroyed");
}

bool Engine::Initialize()
{
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
    TraceLog(LOG_INFO, "[Engine] Engine shutdown complete");
}

RenderManager &Engine::GetRenderManager()
{
    return m_renderManager;
}

InputManager &Engine::GetInputManager()
{
    return m_inputManager;
}

AudioManager &Engine::GetAudioManager()
{
    return m_audioManager;
}

ModuleManager &Engine::GetModuleManager()
{
    return *m_moduleManager;
}

void Engine::RegisterModule(std::unique_ptr<IEngineModule> module)
{
    if (m_moduleManager)
    {
        m_moduleManager->RegisterModule(std::move(module));
    }
}

void Engine::RequestExit()
{
    m_shouldExit = true;
}

bool Engine::ShouldExit() const
{
    return m_shouldExit || WindowShouldClose();
}

bool Engine::IsDebugInfoVisible() const
{
    return m_debugInfoVisible;
}

void Engine::SetDebugInfoVisible(bool visible)
{
    m_debugInfoVisible = visible;
}
