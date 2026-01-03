#ifndef CH_EDITOR_LAYER_H
#define CH_EDITOR_LAYER_H

#include "engine/base.h"
#include "engine/layer.h"
#include "engine/scene.h"
#include "inspector_panel.h"
#include "scene_hierarchy_panel.h"
#include <filesystem>
#include <raylib.h>


namespace CH
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
    virtual void OnImGuiRender() override;
    virtual void OnEvent(Event &e) override;

private:
    void UI_DrawProjectSelector();
    void NewProject();
    void OpenProject();
    void OpenProject(const std::filesystem::path &path);
    void SaveProject();

    void NewScene();
    void OpenScene();
    void OpenScene(const std::filesystem::path &path);
    void SaveScene();
    void SaveSceneAs();

private:
    Camera3D m_EditorCamera;
    Ref<Scene> m_ActiveScene;

private:
    SceneHierarchyPanel m_SceneHierarchyPanel;
    InspectorPanel m_InspectorPanel;
};
} // namespace CH

#endif // CH_EDITOR_LAYER_H
