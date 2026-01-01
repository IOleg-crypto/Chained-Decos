#include "Application.h"
#include "components/physics/collision/core/CollisionManager.h"
#include "components/rendering/core/RenderManager.h"
#include "core/Log.h"
#include "core/audio/Audio.h"
#include "core/imgui/core/GuiManager.h"
#include "core/input/Input.h"
#include "core/module/ModuleManager.h"
#include "core/renderer/Renderer.h"
#include "core/scripting/ScriptManager.h"
#include "rlImGui.h"
#include "scene/MapManager.h"
#include "scene/core/SceneManager.h"
#include "scene/main/LevelManager.h"
#include "scene/main/World.h"
#include "scene/resources/font/FontService.h"
#include "scene/resources/model/Model.h"
#include "scene/resources/texture/TextureService.h"

#include <raylib.h>

namespace CHEngine
{
Application *Application::s_Instance = nullptr;

Application::Application(const std::string &name)
{
    CD_CORE_ASSERT(!s_Instance, "Application already exists!");
    s_Instance = this;

    WindowProps props;
    props.Title = name;
    m_Window = std::make_unique<Window>(props);
    m_Window->SetEventCallback(CD_BIND_EVENT_FN(Application::OnEvent));

    Renderer::Init();
    RenderManager::Init(m_Window->GetWidth(), m_Window->GetHeight(), props.Title.c_str());
    ModelLoader::Init();
    Audio::Init();
    ScriptManager::Init();
    ModuleManager::Init();
    FontService::Init();
    TextureService::Init();
    SceneManager::Init();
    MapManager::Init();
    LevelManager::Init({});
    GuiManager::Init();
    WorldManager::Init();
    CollisionManager::Init();
}

Application::~Application()
{
    CollisionManager::Shutdown();
    WorldManager::Shutdown();
    GuiManager::Shutdown();
    LevelManager::Shutdown();
    MapManager::Shutdown();
    SceneManager::Shutdown();
    TextureService::Shutdown();
    FontService::Shutdown();
    ModuleManager::Shutdown();
    ScriptManager::Shutdown();
    Audio::Shutdown();
    ModelLoader::Shutdown();
    RenderManager::Shutdown();
    Renderer::Shutdown();
}

void Application::PushLayer(Layer *layer)
{
    m_LayerStack.PushLayer(layer);
    layer->OnAttach();
}

void Application::PushOverlay(Layer *overlay)
{
    m_LayerStack.PushOverlay(overlay);
    overlay->OnAttach();
}

void Application::PopLayer(Layer *layer)
{
    m_LayerStack.PopLayer(layer);
    m_LayerDeletionQueue.push_back(layer);
}

void Application::PopOverlay(Layer *overlay)
{
    m_LayerStack.PopOverlay(overlay);
    m_LayerDeletionQueue.push_back(overlay);
}

void Application::OnEvent(Event &e)
{
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<WindowCloseEvent>(CD_BIND_EVENT_FN(Application::OnWindowClose));
    dispatcher.Dispatch<WindowResizeEvent>(CD_BIND_EVENT_FN(Application::OnWindowResize));

    // Snapshot iteration to prevent crashes if layers are pushed/popped during events
    std::vector<Layer *> layersSnapshot;
    for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
        layersSnapshot.push_back(*it);

    for (Layer *layer : layersSnapshot)
    {
        if (e.Handled)
            break;
        layer->OnEvent(e);
    }
}

void Application::Run()
{
    while (m_Running)
    {
        float time = (float)GetTime(); // Raylib GetTime()
        float deltaTime = time - m_LastFrameTime;
        m_LastFrameTime = time;

        Input::Update();
        Audio::Update(deltaTime);

        if (!m_Minimized)
        {
            // Snapshot iteration for safety
            std::vector<Layer *> layersSnapshot;
            for (Layer *layer : m_LayerStack)
                layersSnapshot.push_back(layer);

            for (Layer *layer : layersSnapshot)
                layer->OnUpdate(deltaTime);

            RenderManager::BeginFrame();

            for (Layer *layer : layersSnapshot)
                layer->OnRender();

            rlImGuiBegin();
            for (Layer *layer : layersSnapshot)
                layer->OnImGuiRender();
            rlImGuiEnd();

            RenderManager::EndFrame();
        }

        // Handle deferred deletion
        for (Layer *layer : m_LayerDeletionQueue)
            delete layer;
        m_LayerDeletionQueue.clear();

        m_Window->OnUpdate();
    }
}

bool Application::OnWindowClose(WindowCloseEvent &e)
{
    m_Running = false;
    return true;
}

bool Application::OnWindowResize(WindowResizeEvent &e)
{
    if (e.GetWidth() == 0 || e.GetHeight() == 0)
    {
        m_Minimized = true;
        return false;
    }

    m_Minimized = false;
    Renderer::OnWindowResize(e.GetWidth(), e.GetHeight());

    return false;
}

} // namespace CHEngine
