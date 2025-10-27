//
// Created by AI Assistant
//

#include "StatusBar.h"
#include "imgui.h"
#include <algorithm>

StatusBar::StatusBar() : m_height(25.0f), m_visible(true), m_style("dark")
{
    // Add default items
    AddText("status", "Status", "Ready");
    AddSeparator();
    AddText("objects", "Objects", "0");
    AddSeparator();
    AddText("selected", "Selected", "None");
    AddSeparator();
    AddText("position", "Position", "0, 0, 0");
    AddSeparator();
    AddText("tool", "Tool", "Select");
}

void StatusBar::AddItem(const StatusBarItem& item)
{
    m_items.push_back(item);
}

void StatusBar::RemoveItem(const std::string& id)
{
    auto it = std::find_if(m_items.begin(), m_items.end(),
        [&id](const StatusBarItem& item) { return item.id == id; });
    
    if (it != m_items.end())
    {
        m_items.erase(it);
    }
}

StatusBarItem* StatusBar::GetItem(const std::string& id)
{
    auto it = std::find_if(m_items.begin(), m_items.end(),
        [&id](const StatusBarItem& item) { return item.id == id; });
    
    return (it != m_items.end()) ? &(*it) : nullptr;
}

const StatusBarItem* StatusBar::GetItem(const std::string& id) const
{
    auto it = std::find_if(m_items.begin(), m_items.end(),
        [&id](const StatusBarItem& item) { return item.id == id; });
    
    return (it != m_items.end()) ? &(*it) : nullptr;
}

void StatusBar::UpdateItem(const std::string& id, const std::string& value)
{
    StatusBarItem* item = GetItem(id);
    if (item)
    {
        item->value = value;
    }
}

void StatusBar::SetItemEnabled(const std::string& id, bool enabled)
{
    StatusBarItem* item = GetItem(id);
    if (item)
    {
        item->enabled = enabled;
    }
}

void StatusBar::SetItemVisible(const std::string& id, bool visible)
{
    StatusBarItem* item = GetItem(id);
    if (item)
    {
        item->visible = visible;
    }
}

void StatusBar::AddText(const std::string& id, const std::string& label, const std::string& value)
{
    StatusBarItem item;
    item.type = StatusBarItemType::TEXT;
    item.id = id;
    item.label = label;
    item.value = value;
    m_items.push_back(item);
}

void StatusBar::AddProgress(const std::string& id, const std::string& label, float progress)
{
    StatusBarItem item;
    item.type = StatusBarItemType::PROGRESS;
    item.id = id;
    item.label = label;
    item.progress = progress;
    m_items.push_back(item);
}

void StatusBar::AddButton(const std::string& id, const std::string& label, std::function<void()> action)
{
    StatusBarItem item;
    item.type = StatusBarItemType::BUTTON;
    item.id = id;
    item.label = label;
    item.action = action;
    m_items.push_back(item);
}

void StatusBar::AddToggle(const std::string& id, const std::string& label, bool initialState, std::function<void(bool)> action)
{
    StatusBarItem item;
    item.type = StatusBarItemType::TOGGLE;
    item.id = id;
    item.label = label;
    item.value = initialState ? "On" : "Off";
    item.action = [action, initialState]() { action(!initialState); };
    m_items.push_back(item);
}

void StatusBar::AddDropdown(const std::string& id, const std::string& label, 
                           const std::vector<std::string>& options, std::function<void(int)> action)
{
    StatusBarItem item;
    item.type = StatusBarItemType::DROPDOWN;
    item.id = id;
    item.label = label;
    item.options = options;
    item.selectedOption = 0;
    item.value = options.empty() ? "" : options[0];
    item.action = [action]() { action(0); };
    m_items.push_back(item);
}

void StatusBar::AddSeparator()
{
    StatusBarItem item;
    item.type = StatusBarItemType::SEPARATOR;
    item.id = "separator_" + std::to_string(m_items.size());
    m_items.push_back(item);
}

void StatusBar::Render(float screenWidth, float screenHeight)
{
    if (!m_visible)
        return;
    
    // Set status bar position at bottom of screen
    ImGui::SetNextWindowPos(ImVec2(0, screenHeight - m_height));
    ImGui::SetNextWindowSize(ImVec2(screenWidth, m_height));
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | 
                           ImGuiWindowFlags_NoResize | 
                           ImGuiWindowFlags_NoMove | 
                           ImGuiWindowFlags_NoScrollbar |
                           ImGuiWindowFlags_NoSavedSettings;
    
    if (ImGui::Begin("StatusBar", nullptr, flags))
    {
        // Render items horizontally
        for (size_t i = 0; i < m_items.size(); ++i)
        {
            auto& item = m_items[i];
            
            if (!item.visible)
                continue;
            
            switch (item.type)
            {
            case StatusBarItemType::TEXT:
                ImGui::Text("%s: %s", item.label.c_str(), item.value.c_str());
                break;
                
            case StatusBarItemType::PROGRESS:
                ImGui::Text("%s: %.1f%%", item.label.c_str(), item.progress * 100.0f);
                ImGui::SameLine();
                ImGui::ProgressBar(item.progress, ImVec2(100, 16));
                break;
                
            case StatusBarItemType::BUTTON:
                if (ImGui::Button(item.label.c_str()) && item.enabled && item.action)
                {
                    item.action();
                }
                break;
                
            case StatusBarItemType::TOGGLE:
                if (ImGui::Button(item.value.c_str()) && item.enabled && item.action)
                {
                    item.action();
                    item.value = (item.value == "On") ? "Off" : "On";
                }
                break;
                
            case StatusBarItemType::DROPDOWN:
                if (ImGui::BeginCombo(item.label.c_str(), item.value.c_str()))
                {
                    for (size_t j = 0; j < item.options.size(); ++j)
                    {
                        bool isSelected = (j == item.selectedOption);
                        if (ImGui::Selectable(item.options[j].c_str(), isSelected))
                        {
                            item.selectedOption = static_cast<int>(j);
                            item.value = item.options[j];
                            if (item.action)
                            {
                                item.action();
                            }
                        }
                        if (isSelected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
                break;
                
            case StatusBarItemType::SEPARATOR:
                ImGui::Separator();
                break;
            }
            
            // Add spacing between items (except for the last item)
            if (i < m_items.size() - 1)
            {
                ImGui::SameLine();
                ImGui::Spacing();
                ImGui::SameLine();
            }
        }
    }
    ImGui::End();
}

void StatusBar::SetHeight(float height)
{
    m_height = height;
}

float StatusBar::GetHeight() const
{
    return m_height;
}

void StatusBar::SetVisible(bool visible)
{
    m_visible = visible;
}

bool StatusBar::IsVisible() const
{
    return m_visible;
}

void StatusBar::SetStyle(const std::string& style)
{
    m_style = style;
}

void StatusBar::UpdateStatus(const std::string& message)
{
    UpdateItem("status", message);
}

void StatusBar::UpdateProgress(const std::string& id, float progress)
{
    StatusBarItem* item = GetItem(id);
    if (item && item->type == StatusBarItemType::PROGRESS)
    {
        item->progress = std::clamp(progress, 0.0f, 1.0f);
    }
}

void StatusBar::UpdateValue(const std::string& id, const std::string& value)
{
    UpdateItem(id, value);
}

void StatusBar::ClearItems()
{
    m_items.clear();
}

void StatusBar::ClearStatus()
{
    UpdateItem("status", "Ready");
}

size_t StatusBar::GetItemCount() const
{
    return m_items.size();
}

std::vector<std::string> StatusBar::GetItemIds() const
{
    std::vector<std::string> ids;
    for (const auto& item : m_items)
    {
        ids.push_back(item.id);
    }
    return ids;
}
