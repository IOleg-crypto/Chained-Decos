#include "core/Log.h"
#include "editor/EditorLayer.h"
#include "editor/panels/HierarchyPanel.h"
#include "editor/panels/InspectorPanel.h"
#include "editor/panels/ViewportPanel.h"
#include "raylib.h"


EditorLayer::EditorLayer(entt::registry &registry) : m_context(registry)
{
}

EditorLayer::~EditorLayer() = default;

void EditorLayer::OnAttach()
{
    CD_INFO("EditorLayer: OnAttach");

    // Initialize managers
    m_sceneManager = std::make_unique<SceneManager>(m_context);
    m_toolManager = std::make_unique<ToolManager>(m_context);

    // Initialize panels
    InitializePanels();

    // Load last scene or new scene
    // m_sceneManager->NewScene();
}

void EditorLayer::OnDetach()
{
    CD_INFO("EditorLayer: OnDetach");
    m_panels.clear();
    m_sceneManager.reset();
    m_toolManager.reset();
}

void EditorLayer::OnEvent(CHEngine::Event &event)
{
    // Pass event to tool manager (shortcuts, gizmos)
    if (m_toolManager)
    {
        m_toolManager->OnEvent(event);
    }

    if (event.Handled)
        return;

    // Propagate to panels
    PropagateEventToPanels(event);
}

void EditorLayer::OnUpdate(float deltaTime)
{
    // Update tools
    if (m_toolManager)
    {
        m_toolManager->OnUpdate();
    }

    // Update panels
    for (auto &panel : m_panels)
    {
        if (panel->IsVisible())
        {
            panel->OnUpdate(deltaTime);
        }
    }
}

void EditorLayer::OnRender()
{
    // Render panels
    for (auto &panel : m_panels)
    {
        if (panel->IsVisible())
        {
            panel->Render();
        }
    }
}

EditorContext &EditorLayer::GetContext()
{
    return m_context;
}

SceneManager &EditorLayer::GetSceneManager()
{
    return *m_sceneManager;
}

void EditorLayer::InitializePanels()
{
    // Initialize panels
    m_panels.push_back(std::make_unique<InspectorPanel>(m_context));
    m_panels.push_back(std::make_unique<HierarchyPanel>(m_context));
    m_panels.push_back(std::make_unique<ViewportPanel>(m_context));
}

void EditorLayer::PropagateEventToPanels(CHEngine::Event &e)
{
    for (auto &panel : m_panels)
    {
        if (e.Handled)
            break;

        if (panel->IsVisible())
        {
            panel->OnEvent(e);
        }
    }
}

