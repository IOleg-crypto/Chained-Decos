//
// Created by AI Assistant
//

#include "ClipboardManager.h"
#include "MapObject.h"
#include <algorithm>

// Initialize static instance
std::unique_ptr<ClipboardManager> ClipboardManager::s_instance = nullptr;

ClipboardManager& ClipboardManager::GetInstance()
{
    if (!s_instance)
    {
        s_instance = std::make_unique<ClipboardManager>();
    }
    return *s_instance;
}

void ClipboardManager::Copy(const std::vector<MapObject>& objects, const std::vector<int>& indices)
{
    m_clipboard.clear();
    
    for (int index : indices)
    {
        if (index >= 0 && index < objects.size())
        {
            m_clipboard.push_back(const_cast<MapObject*>(&objects[index]));
        }
    }
    
    m_hasData = !m_clipboard.empty();
}

void ClipboardManager::Copy(const MapObject& object)
{
    m_clipboard.clear();
    m_clipboard.push_back(const_cast<MapObject*>(&object));
    m_hasData = true;
}

std::vector<MapObject> ClipboardManager::Paste()
{
    if (!m_hasData)
        return {};
    
    std::vector<MapObject> pastedObjects;
    
    for (const auto& objPtr : m_clipboard)
    {
        MapObject newObj = *objPtr;
        // Modify names to avoid duplicates
        newObj.SetObjectName(objPtr->GetObjectName() + " (Copy)");
        // Offset position slightly
        Vector3 pos = objPtr->GetPosition();
        newObj.SetPosition({pos.x + 1.0f, pos.y, pos.z + 1.0f});
        pastedObjects.push_back(newObj);
    }
    
    return pastedObjects;
}

bool ClipboardManager::HasData() const
{
    return m_hasData;
}

void ClipboardManager::Clear()
{
    m_clipboard.clear();
    m_hasData = false;
}

std::string ClipboardManager::GetClipboardInfo() const
{
    if (!m_hasData)
        return "Clipboard is empty";
    
    if (m_clipboard.size() == 1)
        return "1 object: " + m_clipboard[0]->GetObjectName();
    
    return std::to_string(m_clipboard.size()) + " objects copied";
}

size_t ClipboardManager::GetClipboardSize() const
{
    return m_clipboard.size();
}

void ClipboardManager::Duplicate(const std::vector<MapObject>& objects, const std::vector<int>& indices, 
                                std::vector<MapObject>& target, float offset)
{
    for (int index : indices)
    {
        if (index >= 0 && index < objects.size())
        {
            MapObject duplicate = objects[index];
            
            // Offset position
            Vector3 pos = duplicate.GetPosition();
            duplicate.SetPosition({pos.x + offset, pos.y + offset, pos.z + offset});
            
            // Modify name
            duplicate.SetObjectName(objects[index].GetObjectName() + " (Duplicate)");
            
            target.push_back(duplicate);
        }
    }
}
