#ifndef GUI_MANAGER_H
#define GUI_MANAGER_H

#include "GuiElement.h"
#include "core/interfaces/IGuiManager.h"
#include <algorithm>
#include <vector>


namespace CHEngine
{
class GuiManager : public IGuiManager
{
public:
    GuiManager() : m_visible(true)
    {
    }
    ~GuiManager() override
    {
        Shutdown();
    }

    void Initialize() override
    {
    }

    void Update(float deltaTime) override
    {
        if (!m_visible)
            return;

        for (auto &element : m_elements)
        {
            if (element->IsEnabled())
            {
                element->Update(deltaTime);
                element->HandleInput();
            }
        }
    }

    void Render() override
    {
        if (!m_visible)
            return;

        for (auto &element : m_elements)
        {
            if (element->IsVisible())
            {
                element->Render();
            }
        }
    }

    void Shutdown() override
    {
        Clear();
    }

    void AddElement(std::shared_ptr<GuiElement> element) override
    {
        m_elements.push_back(element);
    }

    void RemoveElement(std::shared_ptr<GuiElement> element) override
    {
        m_elements.erase(std::remove(m_elements.begin(), m_elements.end(), element),
                         m_elements.end());
    }

    void Clear() override
    {
        m_elements.clear();
    }

    bool IsVisible() const override
    {
        return m_visible;
    }
    void SetVisible(bool visible) override
    {
        m_visible = visible;
    }

private:
    std::vector<std::shared_ptr<GuiElement>> m_elements;
    bool m_visible;
};
} // namespace CHEngine

#endif // GUI_MANAGER_H


