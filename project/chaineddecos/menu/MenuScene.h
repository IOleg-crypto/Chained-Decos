#ifndef MENU_SCENE_H
#define MENU_SCENE_H

#include "scene/core/Scene.h"

namespace CHEngine
{

// Main menu scene with ImGui UI
class MenuScene : public Scene
{
public:
    MenuScene();
    ~MenuScene() = default;

    void OnUpdateRuntime(float deltaTime) override;
    void OnRenderRuntime() override;

    // Menu actions
    enum class MenuAction
    {
        None,
        Play,
        Settings,
        Quit
    };

    MenuAction GetPendingAction() const
    {
        return m_PendingAction;
    }
    void ClearPendingAction()
    {
        m_PendingAction = MenuAction::None;
    }

private:
    void RenderMenuUI();

    MenuAction m_PendingAction;
};

} // namespace CHEngine

#endif // MENU_SCENE_H
