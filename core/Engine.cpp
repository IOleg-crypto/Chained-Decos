#include "Engine.h"
#include "components/audio/core/AudioManager.h"
#include "components/input/core/InputManager.h"
#include "components/physics/collision/core/CollisionManager.h"
#include "components/rendering/core/RenderManager.h"
#include "core/Log.h"
#include "core/imgui/core/GuiManager.h"
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
    m_Window = std::make_unique<ChainedEngine::Window>(props);

    // 1. Rendering
    auto renderManager =
        std::shared_ptr<RenderManager>(&RenderManager::Get(), [](RenderManager *) {});
    if (renderManager)
    {
        renderManager->Initialize(props.Width, props.Height, props.Title.c_str());
    }
    RegisterService<RenderManager>(renderManager);

    // 2. Input
    auto inputManager = std::shared_ptr<ChainedDecos::InputManager>(
        &ChainedDecos::InputManager::Get(), [](ChainedDecos::InputManager *) {});
    RegisterService<ChainedDecos::InputManager>(inputManager);

    // 3. Audio
    auto audioManager = std::shared_ptr<AudioManager>(&AudioManager::Get(), [](AudioManager *) {});
    RegisterService<IAudioManager>(audioManager);

    // 4. Physics
    auto collisionManager = std::shared_ptr<ICollisionManager>(new CollisionManager());
    RegisterService<ICollisionManager>(collisionManager);

    // 5. Resources
    auto modelLoader = std::shared_ptr<IModelLoader>(new ModelLoader());
    RegisterService<IModelLoader>(modelLoader);

    // 6. World
    auto worldManager = std::shared_ptr<IWorldManager>(new WorldManager());
    RegisterService<IWorldManager>(worldManager);

    CD_CORE_INFO("Engine initialized successfully");

    // Initialize GUI
    auto guiManager = std::make_shared<ChainedDecos::GuiManager>();
    RegisterService<IGuiManager>(guiManager);

    if (guiManager)
        guiManager->Initialize();

    return true;
}

void Engine::Update(float deltaTime)
{
    if (m_ModuleManager)
    {
        m_ModuleManager->UpdateAllModules(deltaTime);
    }

    auto guiManager = GetService<IGuiManager>();
    if (guiManager)
    {
        guiManager->Update(deltaTime);
    }
}

void Engine::Shutdown()
{
    CD_CORE_INFO("Shutting down Core Services...");

    if (m_ModuleManager)
        m_ModuleManager->ShutdownAllModules();

    auto audioManager = GetService<AudioManager>();
    if (audioManager)
        audioManager->Shutdown();

    auto inputManager = GetService<ChainedDecos::InputManager>();
    if (inputManager)
        inputManager->Shutdown();

    auto renderManager = GetService<RenderManager>();
    if (renderManager)
        renderManager->Shutdown();

    ServiceRegistry::Clear();
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
    return GetService<RenderManager>();
}

std::shared_ptr<IInputManager> Engine::GetInputManager() const
{
    return GetService<ChainedDecos::InputManager>();
}

std::shared_ptr<IAudioManager> Engine::GetAudioManager() const
{
    return GetService<IAudioManager>();
}

std::shared_ptr<IModelLoader> Engine::GetModelLoader() const
{
    return GetService<IModelLoader>();
}

std::shared_ptr<IGuiManager> Engine::GetGuiManager() const
{
    return GetService<IGuiManager>();
}

std::shared_ptr<ICollisionManager> Engine::GetCollisionManager() const
{
    return GetService<ICollisionManager>();
}

std::shared_ptr<IWorldManager> Engine::GetWorldManager() const
{
    return GetService<IWorldManager>();
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
