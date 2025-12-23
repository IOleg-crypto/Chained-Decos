#include "Engine.h"
#include "components/audio/core/AudioManager.h"
#include "components/input/core/InputManager.h"
#include "components/physics/collision/core/CollisionManager.h"
#include "components/rendering/core/RenderManager.h"
#include "core/Log.h"
#include "core/imgui/core/GuiManager.h"
#include "core/module/ModuleManager.h"
#include "core/scripting/ScriptManager.h"
#include "core/window/Window.h"
#include "events/UIEventRegistry.h"
#include "scene/SceneManager.h"
#include "scene/main/core/World.h"
#include "scene/resources/font/FontService.h"
#include "scene/resources/model/core/Model.h"
#include <memory>
#include <raylib.h>
#include <stdexcept>

namespace CHEngine
{

using namespace CHEngine;

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

bool Engine::Initialize(const CHEngine::WindowProps &props)
{
    CD_CORE_INFO("Initializing Core Services...");

    // 0. Window Creation
    m_Window = std::make_unique<CHEngine::Window>(props);

    // 1. Rendering
    m_RenderManager = std::unique_ptr<RenderManager>(new RenderManager());
    if (m_RenderManager)
    {
        m_RenderManager->Initialize(props.Width, props.Height, props.Title.c_str());
    }

    // 2. Input
    m_InputManager = std::make_unique<CHEngine::InputManager>();
    if (m_InputManager)
    {
        m_InputManager->Initialize();
    }

    // 3. Audio
    m_AudioManager = std::unique_ptr<IAudioManager>(new AudioManager());
    if (m_AudioManager)
    {
        m_AudioManager->Initialize();
    }

    // 4. Physics
    m_CollisionManager = std::unique_ptr<ICollisionManager>(new CollisionManager());
    if (m_CollisionManager)
    {
        m_CollisionManager->Initialize();
    }

    // 5. Resources
    m_ModelLoader = std::unique_ptr<IModelLoader>(new ModelLoader());

    // 6. World
    m_WorldManager = std::unique_ptr<IWorldManager>(new WorldManager());

    // 7. Scripting
    m_ScriptManager = std::make_unique<ScriptManager>();
    if (m_ScriptManager)
    {
        m_ScriptManager->Initialize();
    }

    // 8. GUI
    m_GuiManager = std::make_unique<CHEngine::GuiManager>();
    if (m_GuiManager)
    {
        m_GuiManager->Initialize();
    }

    // 9. Scenes
    m_SceneManager = std::make_unique<SceneManager>();
    m_FontService = std::make_unique<FontService>();
    m_UIEventRegistry = std::make_unique<UIEventRegistry>();

    CD_CORE_INFO("Engine initialized successfully");
    return true;
}

void Engine::Shutdown()
{
    CD_CORE_INFO("Shutting down Core Services...");

    if (m_ModuleManager)
        m_ModuleManager->ShutdownAllModules();

    if (m_AudioManager)
        m_AudioManager->Shutdown();

    if (m_InputManager)
        m_InputManager->Shutdown();

    if (m_RenderManager)
        m_RenderManager->Shutdown();

    if (m_ScriptManager)
        m_ScriptManager->Shutdown();

    if (m_GuiManager)
        m_GuiManager->Shutdown();

    if (m_FontService)
        m_FontService->Shutdown();

    m_UIEventRegistry.reset();
    m_FontService.reset();
    m_SceneManager.reset();
    m_GuiManager.reset();
    m_ScriptManager.reset();
    m_WorldManager.reset();
    m_ModelLoader.reset();
    m_CollisionManager.reset();
    m_AudioManager.reset();
    m_InputManager.reset();
    m_RenderManager.reset();

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
    return m_RenderManager ? m_RenderManager->IsCollisionDebugVisible() : false;
}

bool Engine::ShouldExit() const
{
    return m_shouldExit || WindowShouldClose();
}

RenderManager &Engine::GetRenderManager() const
{
    return *m_RenderManager;
}

CHEngine::InputManager &Engine::GetInputManager() const
{
    return *m_InputManager;
}

IAudioManager &Engine::GetAudioManager() const
{
    return *m_AudioManager;
}

IModelLoader &Engine::GetModelLoader() const
{
    return *m_ModelLoader;
}

CHEngine::GuiManager &Engine::GetGuiManager() const
{
    return *m_GuiManager;
}

ICollisionManager &Engine::GetCollisionManager() const
{
    return *m_CollisionManager;
}

IWorldManager &Engine::GetWorldManager() const
{
    return *m_WorldManager;
}

ScriptManager &Engine::GetScriptManager() const
{
    return *m_ScriptManager;
}
CHEngine::SceneManager &Engine::GetSceneManager() const
{
    return *m_SceneManager;
}

CHEngine::FontService &Engine::GetFontService() const
{
    return *m_FontService;
}

CHEngine::UIEventRegistry &Engine::GetUIEventRegistry() const
{
    return *m_UIEventRegistry;
}

entt::registry &Engine::GetECSRegistry()
{
    return m_ECSRegistry;
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

Window *Engine::GetWindow() const
{
    return m_Window.get();
}
} // namespace CHEngine
