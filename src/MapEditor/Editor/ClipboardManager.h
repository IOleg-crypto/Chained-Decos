//
// Created by AI Assistant
//

#ifndef CLIPBOARDMANAGER_H
#define CLIPBOARDMANAGER_H

#include <vector>
#include <string>
#include <memory>

// Include MapObject header
#include "MapObject.h"

// Clipboard manager for copy/paste operations
class ClipboardManager
{
private:
    std::vector<MapObject*> m_clipboard;
    bool m_hasData;
    
    // Singleton pattern
    static std::unique_ptr<ClipboardManager> s_instance;

public:
    // Singleton access
    static ClipboardManager& GetInstance();
    ~ClipboardManager() = default;
    
    // Disable copy constructor and assignment operator
    ClipboardManager(const ClipboardManager&) = delete;
    ClipboardManager& operator=(const ClipboardManager&) = delete;
    
    // Make constructor public for make_unique
    ClipboardManager() : m_hasData(false) {}

    // Core clipboard operations
    void Copy(const std::vector<MapObject>& objects, const std::vector<int>& indices);
    void Copy(const MapObject& object);
    std::vector<MapObject> Paste();
    bool HasData() const;
    void Clear();
    
    // Utility functions
    std::string GetClipboardInfo() const;
    size_t GetClipboardSize() const;
    
    // Advanced operations
    void Duplicate(const std::vector<MapObject>& objects, const std::vector<int>& indices, 
                   std::vector<MapObject>& target, float offset = 1.0f);
};

#endif // CLIPBOARDMANAGER_H
