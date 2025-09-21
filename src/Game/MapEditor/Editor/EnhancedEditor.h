//
// Created by AI Assistant
//

#ifndef ENHANCEDEDITOR_H
#define ENHANCEDEDITOR_H

#include "Editor.h"
#include "UndoRedo.h"
#include "ClipboardManager.h"
#include "LayerManager.h"
#include "ShortcutManager.h"
#include "ObjectFilter.h"
#include "StatusBar.h"
#include <memory>

// Enhanced editor class that integrates all new features
class EnhancedEditor : public Editor
{
private:
    // Enhanced systems
    std::unique_ptr<UndoRedoManager> m_undoRedoManager;
    std::unique_ptr<LayerManager> m_layerManager;
    std::unique_ptr<ShortcutManager> m_shortcutManager;
    std::unique_ptr<ObjectFilter> m_objectFilter;
    std::unique_ptr<StatusBar> m_statusBar;
    
    // Enhanced UI state
    bool m_showLayerPanel;
    bool m_showFilterPanel;
    bool m_showShortcutPanel;
    bool m_showInfoPanel;
    std::string m_searchQuery;
    std::string m_filterQuery;
    
    // Enhanced object management
    std::vector<int> m_selectedObjects; // Multiple selection support
    bool m_multiSelectMode;
    
public:
    EnhancedEditor();
    ~EnhancedEditor() = default;
    
    // Override base class methods
    void Update() override;
    void RenderImGui() override;
    void HandleInput() override;
    
    // Enhanced object management
    void AddObject(const MapObject& obj) override;
    void RemoveObject(int index) override;
    void SelectObject(int index) override;
    void ClearSelection() override;
    
    // Multiple selection support
    void AddToSelection(int index);
    void RemoveFromSelection(int index);
    void SelectMultiple(const std::vector<int>& indices);
    std::vector<int> GetSelectedObjects() const;
    bool IsMultiSelectMode() const;
    void SetMultiSelectMode(bool enabled);
    
    // Copy/Paste operations
    void CopySelected();
    void Paste();
    void DuplicateSelected();
    bool CanPaste() const;
    
    // Layer operations
    void MoveSelectedToLayer(const std::string& layerName);
    void CreateNewLayer(const std::string& name);
    void DeleteLayer(const std::string& name);
    std::vector<std::string> GetLayerNames() const;
    
    // Filter and search
    void ApplyFilter(const std::string& query);
    void ApplySearch(const std::string& query);
    void ClearFilter();
    void ClearSearch();
    std::vector<int> GetFilteredObjects() const;
    
    // Status updates
    void UpdateStatusBar();
    void ShowMessage(const std::string& message);
    
    // Enhanced file operations
    void SaveMap(const std::string& filename) override;
    void LoadMap(const std::string& filename) override;
    
private:
    // Enhanced UI rendering
    void RenderEnhancedToolbar();
    void RenderLayerPanel();
    void RenderFilterPanel();
    void RenderShortcutPanel();
    void RenderInfoPanel();
    void RenderEnhancedObjectPanel();
    void RenderEnhancedPropertiesPanel();
    
    // Enhanced input handling
    void HandleEnhancedKeyboardInput();
    void HandleEnhancedMouseInput();
    
    // Utility functions
    void SetupDefaultShortcuts();
    void UpdateObjectIndices();
    void RefreshFilteredObjects();
    std::string GetObjectInfo(const MapObject& obj) const;
};

#endif // ENHANCEDEDITOR_H
