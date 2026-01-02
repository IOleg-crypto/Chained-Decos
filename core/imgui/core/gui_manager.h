#ifndef CD_CORE_IMGUI_CORE_GUI_MANAGER_H
#define CD_CORE_IMGUI_CORE_GUI_MANAGER_H

#include "gui_element.h"
#include <algorithm>
#include <memory>
#include <vector>

namespace CHEngine
{
class GuiManager
{
public:
    static void Init();
    static void Shutdown();
    static bool IsInitialized();
    static void Update(float deltaTime);
    static void Render();

    static void AddElement(std::shared_ptr<GuiElement> element);
    static void RemoveElement(std::shared_ptr<GuiElement> element);
    static void Clear();

    static bool IsVisible();
    static void SetVisible(bool visible);

    ~GuiManager();

public:
    GuiManager();

    void InternalUpdate(float deltaTime);
    void InternalRender();
    void InternalShutdown();

    void InternalAddElement(std::shared_ptr<GuiElement> element)
    {
        m_elements.push_back(element);
    }
    void InternalRemoveElement(std::shared_ptr<GuiElement> element)
    {
        m_elements.erase(std::remove(m_elements.begin(), m_elements.end(), element),
                         m_elements.end());
    }
    void InternalClear()
    {
        m_elements.clear();
    }
    bool InternalIsVisible() const
    {
        return m_visible;
    }
    void InternalSetVisible(bool visible)
    {
        m_visible = visible;
    }

private:
    std::vector<std::shared_ptr<GuiElement>> m_elements;
    bool m_visible;
};
} // namespace CHEngine

#endif // CD_CORE_IMGUI_CORE_GUI_MANAGER_H
