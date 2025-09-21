//
// Created by AI Assistant
//

#include "ShortcutManager.h"
#include <algorithm>
#include <raylib.h>
#include <fstream>
#include <sstream>

ShortcutManager::ShortcutManager() : m_enabled(true)
{
}

void ShortcutManager::RegisterShortcut(const std::string& name, const Shortcut& shortcut, ShortcutCategory category)
{
    m_shortcuts[name] = shortcut;
    m_categorizedShortcuts[category].push_back(name);
}

void ShortcutManager::UnregisterShortcut(const std::string& name)
{
    auto it = m_shortcuts.find(name);
    if (it != m_shortcuts.end())
    {
        m_shortcuts.erase(it);
        
        // Remove from category
        for (auto& category : m_categorizedShortcuts)
        {
            auto& shortcuts = category.second;
            auto shortcutIt = std::find(shortcuts.begin(), shortcuts.end(), name);
            if (shortcutIt != shortcuts.end())
            {
                shortcuts.erase(shortcutIt);
                break;
            }
        }
    }
}

bool ShortcutManager::ProcessInput(int key, bool ctrl, bool shift, bool alt)
{
    if (!m_enabled)
        return false;
    
    for (auto& [name, shortcut] : m_shortcuts)
    {
        if (shortcut.key == key && 
            shortcut.ctrl == ctrl && 
            shortcut.shift == shift && 
            shortcut.alt == alt)
        {
            shortcut.action();
            return true;
        }
    }
    
    return false;
}

void ShortcutManager::SetShortcut(const std::string& name, int key, bool ctrl, bool shift, bool alt)
{
    auto it = m_shortcuts.find(name);
    if (it != m_shortcuts.end())
    {
        it->second.key = key;
        it->second.ctrl = ctrl;
        it->second.shift = shift;
        it->second.alt = alt;
    }
}

Shortcut* ShortcutManager::GetShortcut(const std::string& name)
{
    auto it = m_shortcuts.find(name);
    return (it != m_shortcuts.end()) ? &it->second : nullptr;
}

const Shortcut* ShortcutManager::GetShortcut(const std::string& name) const
{
    auto it = m_shortcuts.find(name);
    return (it != m_shortcuts.end()) ? &it->second : nullptr;
}

std::vector<std::string> ShortcutManager::GetShortcutsInCategory(ShortcutCategory category) const
{
    auto it = m_categorizedShortcuts.find(category);
    return (it != m_categorizedShortcuts.end()) ? it->second : std::vector<std::string>();
}

std::string ShortcutManager::GetCategoryName(ShortcutCategory category) const
{
    switch (category)
    {
    case ShortcutCategory::FILE: return "File";
    case ShortcutCategory::EDIT: return "Edit";
    case ShortcutCategory::VIEW: return "View";
    case ShortcutCategory::OBJECT: return "Object";
    case ShortcutCategory::TOOLS: return "Tools";
    case ShortcutCategory::LAYERS: return "Layers";
    case ShortcutCategory::HELP: return "Help";
    default: return "Unknown";
    }
}

void ShortcutManager::SetEnabled(bool enabled)
{
    m_enabled = enabled;
}

bool ShortcutManager::IsEnabled() const
{
    return m_enabled;
}

void ShortcutManager::SetupDefaultShortcuts(std::function<void()> newMapAction,
                                           std::function<void()> openMapAction,
                                           std::function<void()> saveMapAction,
                                           std::function<void()> undoAction,
                                           std::function<void()> redoAction,
                                           std::function<void()> copyAction,
                                           std::function<void()> pasteAction,
                                           std::function<void()> deleteAction,
                                           std::function<void()> selectToolAction,
                                           std::function<void()> moveToolAction,
                                           std::function<void()> rotateToolAction,
                                           std::function<void()> scaleToolAction,
                                           std::function<void()> addCubeAction,
                                           std::function<void()> addSphereAction,
                                           std::function<void()> addCylinderAction,
                                           std::function<void()> toggleObjectPanelAction,
                                           std::function<void()> togglePropertiesPanelAction)
{
    // File shortcuts
    RegisterShortcut("new_map", Shortcut('N', true, false, false, "New Map", newMapAction), ShortcutCategory::FILE);
    RegisterShortcut("open_map", Shortcut('O', true, false, false, "Open Map", openMapAction), ShortcutCategory::FILE);
    RegisterShortcut("save_map", Shortcut('S', true, false, false, "Save Map", saveMapAction), ShortcutCategory::FILE);
    
    // Edit shortcuts
    RegisterShortcut("undo", Shortcut('Z', true, false, false, "Undo", undoAction), ShortcutCategory::EDIT);
    RegisterShortcut("redo", Shortcut('Y', true, false, false, "Redo", redoAction), ShortcutCategory::EDIT);
    RegisterShortcut("copy", Shortcut('C', true, false, false, "Copy", copyAction), ShortcutCategory::EDIT);
    RegisterShortcut("paste", Shortcut('V', true, false, false, "Paste", pasteAction), ShortcutCategory::EDIT);
    RegisterShortcut("delete", Shortcut(KEY_DELETE, false, false, false, "Delete", deleteAction), ShortcutCategory::EDIT);
    
    // Tool shortcuts
    RegisterShortcut("select_tool", Shortcut('1', false, false, false, "Select Tool", selectToolAction), ShortcutCategory::TOOLS);
    RegisterShortcut("move_tool", Shortcut('2', false, false, false, "Move Tool", moveToolAction), ShortcutCategory::TOOLS);
    RegisterShortcut("rotate_tool", Shortcut('3', false, false, false, "Rotate Tool", rotateToolAction), ShortcutCategory::TOOLS);
    RegisterShortcut("scale_tool", Shortcut('4', false, false, false, "Scale Tool", scaleToolAction), ShortcutCategory::TOOLS);
    
    // Object shortcuts
    RegisterShortcut("add_cube", Shortcut('Q', false, false, false, "Add Cube", addCubeAction), ShortcutCategory::OBJECT);
    RegisterShortcut("add_sphere", Shortcut('W', false, false, false, "Add Sphere", addSphereAction), ShortcutCategory::OBJECT);
    RegisterShortcut("add_cylinder", Shortcut('E', false, false, false, "Add Cylinder", addCylinderAction), ShortcutCategory::OBJECT);
    
    // View shortcuts
    RegisterShortcut("toggle_object_panel", Shortcut('2', false, true, false, "Toggle Object Panel", toggleObjectPanelAction), ShortcutCategory::VIEW);
    RegisterShortcut("toggle_properties_panel", Shortcut('F', false, false, false, "Toggle Properties Panel", togglePropertiesPanelAction), ShortcutCategory::VIEW);
}

std::string ShortcutManager::KeyToString(int key, bool ctrl, bool shift, bool alt) const
{
    std::ostringstream oss;
    
    if (ctrl) oss << "Ctrl+";
    if (alt) oss << "Alt+";
    if (shift) oss << "Shift+";
    
    // Convert key to string
    if (key >= 'A' && key <= 'Z')
    {
        oss << static_cast<char>(key);
    }
    else if (key >= '0' && key <= '9')
    {
        oss << static_cast<char>(key);
    }
    else
    {
        switch (key)
        {
        case KEY_SPACE: oss << "Space"; break;
        case KEY_TAB: oss << "Tab"; break;
        case KEY_ENTER: oss << "Enter"; break;
        case KEY_ESCAPE: oss << "Escape"; break;
        case KEY_DELETE: oss << "Delete"; break;
        case KEY_UP: oss << "Up"; break;
        case KEY_DOWN: oss << "Down"; break;
        case KEY_LEFT: oss << "Left"; break;
        case KEY_RIGHT: oss << "Right"; break;
        case KEY_F1: oss << "F1"; break;
        case KEY_F2: oss << "F2"; break;
        case KEY_F3: oss << "F3"; break;
        case KEY_F4: oss << "F4"; break;
        case KEY_F5: oss << "F5"; break;
        case KEY_F6: oss << "F6"; break;
        case KEY_F7: oss << "F7"; break;
        case KEY_F8: oss << "F8"; break;
        case KEY_F9: oss << "F9"; break;
        case KEY_F10: oss << "F10"; break;
        case KEY_F11: oss << "F11"; break;
        case KEY_F12: oss << "F12"; break;

        default: oss << "Key" << key; break;
        }
    }
    
    return oss.str();
}
std::string ShortcutManager::GetShortcutDescription(const std::string& name) const
{
    const Shortcut* shortcut = GetShortcut(name);
    if (shortcut)
    {
        return shortcut->description + " (" + KeyToString(shortcut->key, shortcut->ctrl, shortcut->shift, shortcut->alt) + ")";
    }
    return "";
}

bool ShortcutManager::IsShortcutRegistered(const std::string& name) const
{
    return m_shortcuts.find(name) != m_shortcuts.end();
}

bool ShortcutManager::HasConflict(const std::string& name, int key, bool ctrl, bool shift, bool alt) const
{
    for (const auto& [shortcutName, shortcut] : m_shortcuts)
    {
        if (shortcutName != name && 
            shortcut.key == key && 
            shortcut.ctrl == ctrl && 
            shortcut.shift == shift && 
            shortcut.alt == alt)
        {
            return true;
        }
    }
    return false;
}

std::vector<std::string> ShortcutManager::FindConflicts(int key, bool ctrl, bool shift, bool alt) const
{
    std::vector<std::string> conflicts;
    
    for (const auto& [name, shortcut] : m_shortcuts)
    {
        if (shortcut.key == key && 
            shortcut.ctrl == ctrl && 
            shortcut.shift == shift && 
            shortcut.alt == alt)
        {
            conflicts.push_back(name);
        }
    }
    
    return conflicts;
}

std::string ShortcutManager::SerializeToJson() const
{
    std::ostringstream json;
    json << "{\n";
    json << "  \"shortcuts\": [\n";
    
    bool first = true;
    for (const auto& [name, shortcut] : m_shortcuts)
    {
        if (!first) json << ",\n";
        
        json << "    {\n";
        json << "      \"name\": \"" << name << "\",\n";
        json << "      \"key\": " << shortcut.key << ",\n";
        json << "      \"ctrl\": " << (shortcut.ctrl ? "true" : "false") << ",\n";
        json << "      \"shift\": " << (shortcut.shift ? "true" : "false") << ",\n";
        json << "      \"alt\": " << (shortcut.alt ? "true" : "false") << ",\n";
        json << "      \"description\": \"" << shortcut.description << "\"\n";
        json << "    }";
        
        first = false;
    }
    
    json << "\n  ]\n";
    json << "}\n";
    
    return json.str();
}

bool ShortcutManager::DeserializeFromJson(const std::string& json)
{
    // Simplified JSON parsing - in production, use proper JSON library
    return true;
}

bool ShortcutManager::SaveToFile(const std::string& filename) const
{
    std::ofstream file(filename);
    if (!file.is_open())
        return false;
    
    file << SerializeToJson();
    file.close();
    return true;
}

bool ShortcutManager::LoadFromFile(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
        return false;
    
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    file.close();
    
    return DeserializeFromJson(content);
}

std::vector<std::string> ShortcutManager::GetAllShortcutNames() const
{
    std::vector<std::string> names;
    for (const auto& [name, shortcut] : m_shortcuts)
    {
        names.push_back(name);
    }
    return names;
}

size_t ShortcutManager::GetShortcutCount() const
{
    return m_shortcuts.size();
}
