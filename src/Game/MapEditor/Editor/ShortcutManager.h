//
// Created by AI Assistant
//

#ifndef SHORTCUTMANAGER_H
#define SHORTCUTMANAGER_H

#include <functional>
#include <map>
#include <string>
#include <vector>

// Keyboard shortcut structure
struct Shortcut
{
    int key;                    // Main key
    bool ctrl;                  // Ctrl modifier
    bool shift;                 // Shift modifier
    bool alt;                   // Alt modifier
    std::string description;    // Human-readable description
    std::function<void()> action; // Action to execute
    
    Shortcut() : key(0), ctrl(false), shift(false), alt(false) {}
    Shortcut(int k, bool c, bool s, bool a, const std::string& desc, std::function<void()> act)
        : key(k), ctrl(c), shift(s), alt(a), description(desc), action(act) {}
};

// Shortcut categories for organization
enum class ShortcutCategory
{
    FILE,           // File operations (New, Open, Save, etc.)
    EDIT,           // Edit operations (Undo, Redo, Copy, Paste, etc.)
    VIEW,           // View operations (Zoom, Pan, etc.)
    OBJECT,         // Object operations (Add, Delete, Transform, etc.)
    TOOLS,          // Tool selection (Select, Move, Rotate, etc.)
    LAYERS,         // Layer operations (Create, Delete, Toggle, etc.)
    HELP            // Help and info operations
};

// Shortcut manager for handling keyboard shortcuts
class ShortcutManager
{
private:
    std::map<std::string, Shortcut> m_shortcuts;
    std::map<ShortcutCategory, std::vector<std::string>> m_categorizedShortcuts;
    bool m_enabled;
    
public:
    ShortcutManager();
    ~ShortcutManager() = default;
    
    // Core functionality
    void RegisterShortcut(const std::string& name, const Shortcut& shortcut, ShortcutCategory category);
    void UnregisterShortcut(const std::string& name);
    bool ProcessInput(int key, bool ctrl, bool shift, bool alt);
    
    // Shortcut management
    void SetShortcut(const std::string& name, int key, bool ctrl = false, bool shift = false, bool alt = false);
    Shortcut* GetShortcut(const std::string& name);
    const Shortcut* GetShortcut(const std::string& name) const;
    
    // Category operations
    std::vector<std::string> GetShortcutsInCategory(ShortcutCategory category) const;
    std::string GetCategoryName(ShortcutCategory category) const;
    
    // Enable/disable
    void SetEnabled(bool enabled);
    bool IsEnabled() const;
    
    // Default shortcuts setup
    void SetupDefaultShortcuts(std::function<void()> newMapAction,
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
                              std::function<void()> togglePropertiesPanelAction);
    
    // Utility functions
    std::string KeyToString(int key, bool ctrl = false, bool shift = false, bool alt = false) const;
    std::string GetShortcutDescription(const std::string& name) const;
    bool IsShortcutRegistered(const std::string& name) const;
    
    // Conflict detection
    bool HasConflict(const std::string& name, int key, bool ctrl, bool shift, bool alt) const;
    std::vector<std::string> FindConflicts(int key, bool ctrl, bool shift, bool alt) const;
    
    // Serialization
    std::string SerializeToJson() const;
    bool DeserializeFromJson(const std::string& json);
    bool SaveToFile(const std::string& filename) const;
    bool LoadFromFile(const std::string& filename);
    
    // All shortcuts access
    std::vector<std::string> GetAllShortcutNames() const;
    size_t GetShortcutCount() const;
};

#endif // SHORTCUTMANAGER_H
