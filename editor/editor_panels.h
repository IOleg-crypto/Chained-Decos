#ifndef CH_EDITOR_PANELS_H
#define CH_EDITOR_PANELS_H

#include "engine/core/timestep.h"
#include "panels/panel.h"
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <string>
#include <vector>

namespace CHEngine
{

class EditorPanels
{
public:
    EditorPanels() = default;
    ~EditorPanels() = default;

public:
    void Init();

public:
    template <typename T, typename... Args> std::shared_ptr<T> Register(Args&&... args)
    {
        auto panel = std::make_shared<T>(std::forward<Args>(args)...);
        m_PanelRegistry[std::type_index(typeid(T))] = panel;
        m_PanelNameRegistry[panel->GetName()] = panel;
        m_RootPanels.push_back(panel);
        return panel;
    }

    template <typename T> std::shared_ptr<T> Get()
    {
        auto it = m_PanelRegistry.find(std::type_index(typeid(T)));
        if (it != m_PanelRegistry.end())
        {
            return std::static_pointer_cast<T>(it->second);
        }
        return nullptr;
    }

    std::shared_ptr<Panel> Get(const std::string& name)
    {
        auto it = m_PanelNameRegistry.find(name);
        if (it != m_PanelNameRegistry.end())
        {
            return it->second;
        }
        return nullptr;
    }

    template <typename F> void ForEach(F&& func)
    {
        for (auto& panel : m_RootPanels)
        {
            func(panel);
        }
    }

public:
    void OnUpdate(Timestep ts);
    void OnImGuiRender(bool readOnly);
    void OnEvent(Event& e);
    void SetContext(const std::shared_ptr<Scene>& context);

    std::vector<std::shared_ptr<Panel>>& GetPanels()
    {
        return m_RootPanels;
    }

private:
    std::unordered_map<std::type_index, std::shared_ptr<Panel>> m_PanelRegistry;
    std::unordered_map<std::string, std::shared_ptr<Panel>> m_PanelNameRegistry;
    std::vector<std::shared_ptr<Panel>> m_RootPanels;
};

} // namespace CHEngine

#endif // CH_EDITOR_PANELS_H
