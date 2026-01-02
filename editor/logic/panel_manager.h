#pragma once
#include "editor/panels/editor_panel.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace CHEngine
{
/**
 * @brief Manages editor panels and their lifecycle.
 */
class PanelManager
{
public:
    PanelManager() = default;
    ~PanelManager() = default;

    template <typename T, typename... Args> void AddPanel(const std::string &name, Args &&...args)
    {
        static_assert(std::is_base_of<EditorPanel, T>::value, "T must inherit from EditorPanel");
        m_Panels[name] = std::make_shared<T>(std::forward<Args>(args)...);
    }

    void OnImGuiRender()
    {
        for (auto &[name, panel] : m_Panels)
        {
            if (name == "MenuBar")
                continue; // Handled separately by EditorLayer for docking
            if (panel->IsVisible())
                panel->OnImGuiRender();
        }
    }

    std::shared_ptr<EditorPanel> GetPanel(const std::string &name)
    {
        if (m_Panels.find(name) != m_Panels.end())
            return m_Panels[name];
        return nullptr;
    }

    template <typename T> std::shared_ptr<T> GetPanelTyped(const std::string &name)
    {
        return std::static_pointer_cast<T>(GetPanel(name));
    }

private:
    std::unordered_map<std::string, std::shared_ptr<EditorPanel>> m_Panels;
};

} // namespace CHEngine
