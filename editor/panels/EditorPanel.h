#pragma once

namespace CHEngine
{
/**
 * @brief Base class for all editor panels.
 *
 * Follows the Hazel Engine pattern where panels are discrete components
 * with their own rendering and event handling logic.
 */
class EditorPanel
{
public:
    virtual ~EditorPanel() = default;

    virtual void OnImGuiRender() = 0;
    // virtual void OnEvent(Event& e) {} // Add when needed
    // virtual void OnUpdate(float ts) {} // Add when needed

    bool IsVisible() const
    {
        return m_IsVisible;
    }
    void SetVisible(bool visible)
    {
        m_IsVisible = visible;
    }

protected:
    bool m_IsVisible = true;
};

} // namespace CHEngine
