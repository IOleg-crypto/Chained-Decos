#ifndef EDITOR_LAYER_H
#define EDITOR_LAYER_H

#include "core/layer/Layer.h"
#include "editor/core/EditorContext.h"
#include "editor/managers/SceneManager.h"
#include "editor/managers/ToolManager.h"
#include "editor/panels/IEditorPanel.h"
#include <memory>
#include <vector>

// EditorLayer - Main editor layer integrating all editor subsystems
// Manages panels, tools, and scene operations
class EditorLayer : public ChainedDecos::Layer
{
public:
    explicit EditorLayer(entt::registry &registry);
    ~EditorLayer() override;

    // Layer interface
    void OnAttach() override;
    void OnDetach() override;
    void OnEvent(ChainedDecos::Event &event) override;
    void OnUpdate(float deltaTime) override;
    void OnRender() override;

    // Accessors
    EditorContext &GetContext();
    SceneManager &GetSceneManager();

private:
    EditorContext m_context;
    std::unique_ptr<SceneManager> m_sceneManager;
    std::unique_ptr<ToolManager> m_toolManager;
    std::vector<std::unique_ptr<IEditorPanel>> m_panels;

    // Internal methods
    void InitializePanels();
    void PropagateEventToPanels(ChainedDecos::Event &e);
};

#endif // EDITOR_LAYER_H
