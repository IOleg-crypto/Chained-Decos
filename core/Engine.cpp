#include "Engine.h"
#include "components/audio/core/AudioManager.h"
#include "components/input/core/InputManager.h"
#include "components/physics/collision/core/CollisionManager.h"
#include "components/rendering/core/RenderManager.h"
#include "core/Log.h"
#include "core/gui/core/GuiManager.h"
#include "core/module/ModuleManager.h"
#include "core/window/Window.h"
#include "scene/main/core/World.h"
#include "scene/resources/model/core/Model.h"
#include <memory>
#include <raylib.h>
#include <stdexcept>


namespace ChainedEngine
{

using namespace ChainedDecos;

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

bool Engine::Initialize(const ChainedEngine::WindowProps &props)
{
    CD_CORE_INFO("Initializing Core Services...");

    // 0. Window Creation
    // WindowProps local copy if needed, or just use props to create window
    m_Window = std::make_unique<ChainedEngine::Window>(props);

    // 1. Rendering
    m_RenderManager = std::shared_ptr<RenderManager>(&RenderManager::Get(), [](RenderManager *) {});
    if (m_RenderManager)
    {
        m_RenderManager->Initialize(props.Width, props.Height, props.Title.c_str());
    }
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

    CD_CORE_INFO("Engine initialized successfully");
    // Initialize GUI
    m_GuiManager = std::make_shared<ChainedDecos::GuiManager>();
    RegisterService<IGuiManager>(m_GuiManager);

    if (m_GuiManager)
        m_GuiManager->Initialize();

    return true;
}

void Engine::Update(float deltaTime)
{
    if (m_ModuleManager)
    {
        m_ModuleManager->UpdateAllModules(deltaTime);
    }

    if (m_GuiManager)
    {
        m_GuiManager->Update(deltaTime);
    }
}

void Engine::Shutdown()
{
    CD_CORE_INFO("Shutting down Core Services...");

    if (m_ModuleManager)
        m_ModuleManager->ShutdownAllModules();

    m_Services.clear();

    m_WorldManager.reset();
    m_ModelLoader.reset();
    m_CollisionManager.reset();
    m_GuiManager.reset();

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

// std::shared_ptr<IInputManager> Engine::GetInputManager() const
// {
//    return m_InputManager;
// }

std::shared_ptr<AudioManager> Engine::GetAudioManager() const
{
    return m_AudioManager;
}
std::shared_ptr<IModelLoader> Engine::GetModelLoader() const
{
    return m_ModelLoader;
}
std::shared_ptr<IGuiManager> Engine::GetGuiManager() const
{
    return m_GuiManager;
}

std::shared_ptr<ICollisionManager> Engine::GetCollisionManager() const
{
    return m_CollisionManager;
}
std::shared_ptr<IWorldManager> Engine::GetWorldManager() const
{
    return m_WorldManager;
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

} // namespace ChainedEngine
