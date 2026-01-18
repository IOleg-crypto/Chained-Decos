#ifndef CH_INPUT_GRAPH_PANEL_H
#define CH_INPUT_GRAPH_PANEL_H

#include "engine/core/input.h"
#include <string>
#include <unordered_map>

namespace CHEngine
{

class InputGraphPanel
{
public:
    InputGraphPanel();
    ~InputGraphPanel() = default;

    void OnImGuiRender();

private:
    // Rendering
    void RenderContextSelector();
    void RenderActionList();
    void RenderActionNode(const std::string &actionName, InputAction *action);
    void RenderBindingRow(const std::string &actionName, const InputBinding &binding, int index);

    // Modals
    void ShowAddActionModal();
    void ShowEditBindingModal();
    void ShowKeyDetectionModal();

    // Actions
    void SaveCurrentActions();
    void LoadActions(const std::string &path);
    void OnAddBinding(const std::string &actionName);
    void OnEditBinding(const std::string &actionName, int bindingIndex);
    void OnRemoveBinding(const std::string &actionName, int bindingIndex);
    void ApplyBinding();

    // Helpers
    std::string KeyCodeToString(int keyCode) const;
    std::string ActionTypeToString(InputActionType type) const;
    const char *AxisToString(InputAxis axis) const;

    // State
    std::string m_SelectedContext = "Gameplay";
    bool m_ShowAddActionModal = false;
    bool m_ShowEditBindingModal = false;
    bool m_ShowKeyDetectionModal = false;

    // Modal state
    char m_NewActionName[256] = "";
    int m_NewActionTypeIndex = 0;
    int m_DetectedKey = 0;
    InputBinding m_EditingBinding;
    std::string m_EditingActionName;
    int m_EditingBindingIndex = -1;

    // Axis editing
    int m_AxisIndex = 0;
    float m_AxisScale = 1.0f;
};

} // namespace CHEngine

#endif // CH_INPUT_GRAPH_PANEL_H
