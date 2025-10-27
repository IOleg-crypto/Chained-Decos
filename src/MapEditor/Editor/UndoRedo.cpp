//
// Created by AI Assistant
//

#include "UndoRedo.h"
#include "MapObject.h"
#include <algorithm>

// AddObjectOperation implementation
AddObjectOperation::AddObjectOperation(std::vector<MapObject>& objects, const MapObject& obj, int index)
    : m_objects(objects), m_objectIndex(index)
{
    m_addedObject = std::make_unique<MapObject>(obj);
}

void AddObjectOperation::Undo()
{
    if (m_objectIndex >= 0 && static_cast<size_t>(m_objectIndex) < m_objects.size())
    {
        m_objects.erase(m_objects.begin() + m_objectIndex);
    }
}

void AddObjectOperation::Redo()
{
    if (static_cast<size_t>(m_objectIndex) <= m_objects.size() && m_addedObject)
    {
        m_objects.insert(m_objects.begin() + m_objectIndex, *m_addedObject);
    }
}

std::string AddObjectOperation::GetDescription() const
{
    return "Add " + m_addedObject->GetObjectName();
}

// RemoveObjectOperation implementation
RemoveObjectOperation::RemoveObjectOperation(std::vector<MapObject>& objects, int index)
    : m_objects(objects), m_objectIndex(index)
{
    if (index >= 0 && static_cast<size_t>(index) < objects.size())
    {
        m_removedObject = std::make_unique<MapObject>(objects[static_cast<size_t>(index)]);
    }
}

void RemoveObjectOperation::Undo()
{
    if (static_cast<size_t>(m_objectIndex) <= m_objects.size() && m_removedObject)
    {
        m_objects.insert(m_objects.begin() + m_objectIndex, *m_removedObject);
    }
}

void RemoveObjectOperation::Redo()
{
    if (m_objectIndex >= 0 && static_cast<size_t>(m_objectIndex) < m_objects.size())
    {
        m_objects.erase(m_objects.begin() + m_objectIndex);
    }
}

std::string RemoveObjectOperation::GetDescription() const
{
    if (m_removedObject)
        return "Remove " + m_removedObject->GetObjectName();
    return "Remove object";
}

// ModifyObjectOperation implementation
ModifyObjectOperation::ModifyObjectOperation(std::vector<MapObject>& objects, int index,
                                           const MapObject& oldState, const MapObject& newState,
                                           const std::string& propertyName)
    : m_objects(objects), m_objectIndex(index), m_propertyName(propertyName)
{
    m_oldState = std::make_unique<MapObject>(oldState);
    m_newState = std::make_unique<MapObject>(newState);
}

void ModifyObjectOperation::Undo()
{
    if (m_objectIndex >= 0 && static_cast<size_t>(m_objectIndex) < m_objects.size() && m_oldState)
    {
        m_objects[static_cast<size_t>(m_objectIndex)] = *m_oldState;
    }
}

void ModifyObjectOperation::Redo()
{
    if (m_objectIndex >= 0 && static_cast<size_t>(m_objectIndex) < m_objects.size() && m_newState)
    {
        m_objects[static_cast<size_t>(m_objectIndex)] = *m_newState;
    }
}

std::string ModifyObjectOperation::GetDescription() const
{
    if (m_oldState)
        return "Modify " + m_oldState->GetObjectName() + " (" + m_propertyName + ")";
    return "Modify object";
}

// UndoRedoManager implementation
UndoRedoManager::UndoRedoManager(size_t maxStackSize)
    : m_maxStackSize(maxStackSize), m_isRecording(true)
{
}

void UndoRedoManager::PushOperation(std::unique_ptr<UndoRedoOperation> operation)
{
    if (!m_isRecording)
        return;

    // Clear redo stack when new operation is added
    m_redoStack.clear();

    // Add to undo stack
    m_undoStack.push_back(std::move(operation));

    // Limit stack size
    if (m_undoStack.size() > m_maxStackSize)
    {
        m_undoStack.erase(m_undoStack.begin());
    }
}

bool UndoRedoManager::CanUndo() const
{
    return !m_undoStack.empty();
}

bool UndoRedoManager::CanRedo() const
{
    return !m_redoStack.empty();
}

void UndoRedoManager::Undo()
{
    if (!CanUndo())
        return;

    m_isRecording = false; // Prevent recording during undo
    auto operation = std::move(m_undoStack.back());
    m_undoStack.pop_back();
    
    operation->Undo();
    
    m_redoStack.push_back(std::move(operation));
    m_isRecording = true;
}

void UndoRedoManager::Redo()
{
    if (!CanRedo())
        return;

    m_isRecording = false; // Prevent recording during redo
    auto operation = std::move(m_redoStack.back());
    m_redoStack.pop_back();
    
    operation->Redo();
    
    m_undoStack.push_back(std::move(operation));
    m_isRecording = true;
}

void UndoRedoManager::Clear()
{
    m_undoStack.clear();
    m_redoStack.clear();
}

void UndoRedoManager::StartRecording()
{
    m_isRecording = true;
}

void UndoRedoManager::StopRecording()
{
    m_isRecording = false;
}

bool UndoRedoManager::IsRecording() const
{
    return m_isRecording;
}

std::string UndoRedoManager::GetUndoDescription() const
{
    if (m_undoStack.empty())
        return "";
    return m_undoStack.back()->GetDescription();
}

std::string UndoRedoManager::GetRedoDescription() const
{
    if (m_redoStack.empty())
        return "";
    return m_redoStack.back()->GetDescription();
}

size_t UndoRedoManager::GetUndoStackSize() const
{
    return m_undoStack.size();
}

size_t UndoRedoManager::GetRedoStackSize() const
{
    return m_redoStack.size();
}

void UndoRedoManager::RecordAddObject(std::vector<MapObject>& objects, const MapObject& obj, int index)
{
    PushOperation(std::make_unique<AddObjectOperation>(objects, obj, index));
}

void UndoRedoManager::RecordRemoveObject(std::vector<MapObject>& objects, int index)
{
    PushOperation(std::make_unique<RemoveObjectOperation>(objects, index));
}

void UndoRedoManager::RecordModifyObject(std::vector<MapObject>& objects, int index, 
                                        const MapObject& oldState, const MapObject& newState, 
                                        const std::string& propertyName)
{
    PushOperation(std::make_unique<ModifyObjectOperation>(objects, index, oldState, newState, propertyName));
}
