#ifndef EDITOR_LAYER_H
#define EDITOR_LAYER_H

#include "core/layer/Layer.h"
#include "editor/EditorTypes.h"
#include "editor/panels/AssetBrowserPanel.h"
#include "editor/panels/ConsolePanel.h"
#include "editor/panels/HierarchyPanel.h"
#include "editor/panels/InspectorPanel.h"
#include "editor/panels/ToolbarPanel.h"
#include "editor/panels/ViewportPanel.h"
#include "events/Event.h"
#include "events/KeyEvent.h"
#include "events/MouseEvent.h"
#include "scene/camera/CameraController.h"
#include "scene/resources/map/SceneLoader.h"

#include <imgui.h>
#include <memory>

namespace CHD
{
class RuntimeLayer;
}

namespace CHEngine
{
class EditorLayer : public Layer
{
public:
    EditorLayer();
    virtual ~EditorLayer() = default;

    virtual void OnAttach() override;
    virtual void OnDetach() override;
    virtual void OnUpdate(float deltaTime) override;
    virtual void OnRender() override;
    virtual void OnImGuiRender();
    virtual void OnEvent(Event &event) override;

    void OnScenePlay();
    void OnSceneStop();

    void NewScene();
    void OpenScene();
    void SaveScene();
    void SaveSceneAs();
    void AddModel();
    void AddUIElement(const std::string &type);
    void LoadSkybox(const std::string &path = "");
    void ApplySkybox(const std::string &path);

    SceneState GetSceneState() const
    {
        return m_SceneState;
    }

    int GetSelectedObjectIndex() const
    {
        return m_SelectedObjectIndex;
    }
    void SetSelectedObjectIndex(int index, SelectionType type = SelectionType::WORLD_OBJECT)
    {
        m_SelectedObjectIndex = index;
        m_SelectionType = type;
    }
    MapObjectData *GetSelectedObject();
    UIElementData *GetSelectedUIElement();

    Tool GetActiveTool() const
    {
        return m_ActiveTool;
    }
    void SetActiveTool(Tool tool)
    {
        m_ActiveTool = tool;
    }

    bool OnKeyPressed(KeyPressedEvent &e);
    bool OnMouseButtonPressed(MouseButtonPressedEvent &e);

    void UI_DrawDockspace();
    void UI_DrawToolbar();
    void UI_DrawMenuBar();

private:
    std::shared_ptr<CameraController> m_CameraController;

    // Panels
    std::unique_ptr<HierarchyPanel> m_HierarchyPanel;
    std::unique_ptr<InspectorPanel> m_InspectorPanel;
    std::unique_ptr<ViewportPanel> m_ViewportPanel;
    std::unique_ptr<AssetBrowserPanel> m_AssetBrowserPanel;
    std::unique_ptr<ConsolePanel> m_ConsolePanel;
    std::unique_ptr<ToolbarPanel> m_ToolbarPanel;

    // Scene context
    std::shared_ptr<GameScene> m_ActiveScene;
    std::shared_ptr<GameScene> m_EditorScene;
    std::string m_ScenePath;
    std::string m_SceneFilePath;

    // Viewport
    ImVec2 m_ViewportSize = {0.0f, 0.0f};
    bool m_ViewportFocused = false;
    bool m_ViewportHovered = false;

    // State
    SceneState m_SceneState = SceneState::Edit;
    RuntimeMode m_RuntimeMode =
        RuntimeMode::Standalone; // Default to standalone as requested before
    Tool m_ActiveTool = Tool::MOVE;
    int m_SelectedObjectIndex = -1;
    SelectionType m_SelectionType = SelectionType::NONE;

    CHD::RuntimeLayer *m_RuntimeLayer = nullptr;
    bool m_ShowProjectSettings = false;
};
} // namespace CHEngine

#endif // EDITOR_LAYER_H
