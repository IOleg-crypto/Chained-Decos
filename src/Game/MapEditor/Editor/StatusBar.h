//
// Created by AI Assistant
//

#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <string>
#include <vector>
#include <functional>

// Status bar item types
enum class StatusBarItemType
{
    TEXT,           // Simple text display
    PROGRESS,       // Progress bar
    BUTTON,         // Clickable button
    TOGGLE,         // Toggle button
    DROPDOWN,       // Dropdown menu
    SEPARATOR       // Visual separator
};

// Status bar item structure
struct StatusBarItem
{
    StatusBarItemType type;
    std::string id;
    std::string label;
    std::string value;
    bool enabled;
    bool visible;
    std::function<void()> action;
    float progress;         // For progress bars (0.0 - 1.0)
    std::vector<std::string> options; // For dropdown menus
    int selectedOption;     // For dropdown menus
    
    StatusBarItem() : type(StatusBarItemType::TEXT), enabled(true), visible(true), 
                     progress(0.0f), selectedOption(0) {}
};

// Status bar manager for displaying application status and controls
class StatusBar
{
private:
    std::vector<StatusBarItem> m_items;
    float m_height;
    bool m_visible;
    std::string m_style;
    
public:
    StatusBar();
    ~StatusBar() = default;
    
    // Item management
    void AddItem(const StatusBarItem& item);
    void RemoveItem(const std::string& id);
    StatusBarItem* GetItem(const std::string& id);
    const StatusBarItem* GetItem(const std::string& id) const;
    void UpdateItem(const std::string& id, const std::string& value);
    void SetItemEnabled(const std::string& id, bool enabled);
    void SetItemVisible(const std::string& id, bool visible);
    
    // Quick item creation
    void AddText(const std::string& id, const std::string& label, const std::string& value = "");
    void AddProgress(const std::string& id, const std::string& label, float progress = 0.0f);
    void AddButton(const std::string& id, const std::string& label, std::function<void()> action);
    void AddToggle(const std::string& id, const std::string& label, bool initialState, std::function<void(bool)> action);
    void AddDropdown(const std::string& id, const std::string& label, 
                    const std::vector<std::string>& options, std::function<void(int)> action);
    void AddSeparator();
    
    // Rendering
    void Render(float screenWidth, float screenHeight);
    
    // Properties
    void SetHeight(float height);
    float GetHeight() const;
    void SetVisible(bool visible);
    bool IsVisible() const;
    void SetStyle(const std::string& style);
    
    // Status updates
    void UpdateStatus(const std::string& message);
    void UpdateProgress(const std::string& id, float progress);
    void UpdateValue(const std::string& id, const std::string& value);
    
    // Clear operations
    void ClearItems();
    void ClearStatus();
    
    // Utility
    size_t GetItemCount() const;
    std::vector<std::string> GetItemIds() const;
};

#endif // STATUSBAR_H
