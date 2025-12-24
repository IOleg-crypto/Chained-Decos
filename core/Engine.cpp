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
#include "scene/resources/texture/TextureService.h"
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
    CD_CORE_INFO("Initializing Core Services via ServiceRegistry...");

    // 0. Window Creation
    m_Window = std::make_unique<CHEngine::Window>(props);

    // 1. Rendering
    auto renderManager = std::make_shared<RenderManager>();
    renderManager->Initialize(props.Width, props.Height, props.Title.c_str());
    RegisterService<RenderManager>(renderManager);

    // 2. Input
    auto inputManager = std::make_shared<CHEngine::InputManager>();
    inputManager->Initialize();
    RegisterService<CHEngine::InputManager>(inputManager);

    // 3. Audio
    auto audioManager = std::make_shared<AudioManager>();
    audioManager->Initialize();
    RegisterService<IAudioManager>(audioManager);

    // 4. Physics
    auto collisionManager = std::make_shared<CollisionManager>();
    RegisterService<ICollisionManager>(collisionManager);

    // 5. Resources
    auto modelLoader = std::make_shared<ModelLoader>();
    RegisterService<IModelLoader>(modelLoader);

    // 6. World
    auto worldManager = std::make_shared<WorldManager>();
    RegisterService<IWorldManager>(worldManager);

    // 7. Scripting
    auto scriptManager = std::make_shared<ScriptManager>();
    scriptManager->Initialize();
    RegisterService<ScriptManager>(scriptManager);

    // 8. GUI
    auto guiManager = std::make_shared<CHEngine::GuiManager>();
    guiManager->Initialize();
    RegisterService<CHEngine::GuiManager>(guiManager);

    // 9. Scenes
    RegisterService<SceneManager>(std::make_shared<SceneManager>());
    RegisterService<FontService>(std::make_shared<FontService>());
    RegisterService<TextureService>(std::make_shared<TextureService>());
    RegisterService<UIEventRegistry>(std::make_shared<UIEventRegistry>());

    CD_CORE_INFO("Engine initialized successfully");
    return true;
}

void Engine::Update(float deltaTime)
{
    if (m_ModuleManager)
        m_ModuleManager->UpdateAllModules(deltaTime);

    if (auto input = GetService<InputManager>())
        input->Update(deltaTime);

    if (auto audio = GetService<IAudioManager>())
        audio->Update(deltaTime);

    if (auto script = GetService<ScriptManager>())
        script->Update(deltaTime);

    if (auto gui = GetService<GuiManager>())
        gui->Update(deltaTime);
}

void Engine::Shutdown()
{
    CD_CORE_INFO("Shutting down Engine and clearing ServiceRegistry...");

    if (m_ModuleManager)
        m_ModuleManager->ShutdownAllModules();

    if (auto audio = GetService<IAudioManager>())
        audio->Shutdown();

    if (auto input = GetService<InputManager>())
        input->Shutdown();

    if (auto render = GetService<RenderManager>())
        render->Shutdown();

    if (auto script = GetService<ScriptManager>())
        script->Shutdown();

    if (auto gui = GetService<GuiManager>())
        gui->Shutdown();

    if (auto font = GetService<FontService>())
        font->Shutdown();

    if (auto texture = GetService<TextureService>())
        texture->Shutdown();

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
    auto render = GetService<RenderManager>();
    return render ? render->IsCollisionDebugVisible() : false;
}

bool Engine::ShouldExit() const
{
    return m_shouldExit || WindowShouldClose();
}

RenderManager &Engine::GetRenderManager() const
{
    return *GetService<RenderManager>();
}

IInputManager &Engine::GetInputManager() const
{
    return *GetService<CHEngine::InputManager>();
}

IAudioManager &Engine::GetAudioManager() const
{
    return *GetService<IAudioManager>();
}

IModelLoader &Engine::GetModelLoader() const
{
    return *GetService<IModelLoader>();
}

IGuiManager &Engine::GetGuiManager() const
{
    return *GetService<CHEngine::GuiManager>();
}

ICollisionManager &Engine::GetCollisionManager() const
{
    return *GetService<ICollisionManager>();
}

IWorldManager &Engine::GetWorldManager() const
{
    return *GetService<IWorldManager>();
}

ScriptManager &Engine::GetScriptManager() const
{
    return *GetService<ScriptManager>();
}

CHEngine::SceneManager &Engine::GetSceneManager() const
{
    return *GetService<CHEngine::SceneManager>();
}

CHEngine::FontService &Engine::GetFontService() const
{
    return *GetService<CHEngine::FontService>();
}

CHEngine::TextureService &Engine::GetTextureService() const
{
    return *GetService<CHEngine::TextureService>();
}

CHEngine::UIEventRegistry &Engine::GetUIEventRegistry() const
{
    return *GetService<CHEngine::UIEventRegistry>();
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
