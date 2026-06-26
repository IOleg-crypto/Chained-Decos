#ifndef CH_PANEL_H
#define CH_PANEL_H

class EditorLayer;
#include "engine/foundation/base.h"
#include "engine/core/events/events.h"
#include "engine/foundation/timestep.h"
#include "engine/scene/scene.h"
#include <algorithm>
#include <memory>
#include <vector>


namespace Chained
{
// Base class for dockable editor panels with optional scene context.
class Panel
{
public:
    Panel() {
    }
    virtual ~Panel() = default;

    // Draws the panel UI. readOnly is used when the panel should avoid editing.
    virtual void OnImGuiRender(bool readOnly = false)
    {
        for (auto& child : m_Children)
        {
            if (child->m_IsVisible && !child->m_PendingKill)
            {
                child->OnImGuiRender(readOnly);
            }
        }
    }
    // Optional per-frame update hook.
    virtual void OnUpdate(Timestep ts)
    {
        for (auto& child : m_Children)
        {
            if (child->m_IsVisible && !child->m_PendingKill)
            {
                child->OnUpdate(ts);
            }
        }
        CleanupChildren();
    }
    // Optional event hook.
    virtual void OnEvent(Event& e)
    {
        for (auto& child : m_Children)
        {
            if (child->m_IsVisible && !child->m_PendingKill)
            {
                child->OnEvent(e);
            }
        }
    }
    // Optional configuration hook used by settings panels.
    virtual void OnConfiguration()
    {
        for (auto& child : m_Children)
        {
            child->OnConfiguration();
        }
    }
    // Updates the scene context used by the panel.
    virtual void SetContext(const std::shared_ptr<Scene>& context)
    {
        m_Context = context;
        for (auto& child : m_Children)
        {
            child->SetContext(context);
        }
    }

    void AddChild(const std::shared_ptr<Panel>& child)
    {
        if (child)
        {
            m_Children.push_back(child);
        }
    }

    void RemoveChild(const std::shared_ptr<Panel>& child)
    {
        auto it = std::find(m_Children.begin(), m_Children.end(), child);
        if (it != m_Children.end())
        {
            (*it)->m_PendingKill = true; // Deferred removal
        }
    }

    void CleanupChildren()
    {
        m_Children.erase(std::remove_if(m_Children.begin(), m_Children.end(),
                                        [](const std::shared_ptr<Panel>& panel) { return panel->m_PendingKill; }),
                         m_Children.end());
    }

    bool& IsOpen()
    {
        return m_IsOpen;
    }
    bool& ShowSettings()
    {
        return m_ShowSettings;
    }
    const std::string& GetName() const
    {
        return m_Name;
    }

    bool IsVisible() const
    {
        return m_IsVisible;
    }
    void SetVisible(bool visible)
    {
        m_IsVisible = visible;
    }

    bool IsPendingKill() const
    {
        return m_PendingKill;
    }
    void MarkForDelete()
    {
        m_PendingKill = true;
    }

    const std::vector<std::shared_ptr<Panel>>& GetChildren() const
    {
        return m_Children;
    }

protected:
    
    std::string m_Name;
    std::shared_ptr<Scene> m_Context;
    bool m_IsOpen = true;
    bool m_ShowSettings = false;

    bool m_IsVisible = true;
    bool m_PendingKill = false;
    std::vector<std::shared_ptr<Panel>> m_Children;
};
} // namespace Chained

#endif // CH_PANEL_H
