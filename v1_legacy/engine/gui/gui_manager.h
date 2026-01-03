#ifndef CD_ENGINE_GUI_GUI_MANAGER_H
#define CD_ENGINE_GUI_GUI_MANAGER_H

#include "gui_element.h"
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
    GuiManager();

    void InternalUpdate(float deltaTime);
    void InternalRender();
    void InternalShutdown();

private:
    std::vector<std::shared_ptr<GuiElement>> m_elements;
    bool m_visible = true;
};
} // namespace CHEngine

#endif
