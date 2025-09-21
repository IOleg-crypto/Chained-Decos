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
    m_addedObject = new MapObject(obj);
}

AddObjectOperation::~AddObjectOperation()
{
    delete m_addedObject;
}

void AddObjectOperation::Undo()
{
    if (m_objectIndex >= 0 && m_objectIndex < m_objects.size())
    {
        m_objects.erase(m_objects.begin() + m_objectIndex);
    }
}

void AddObjectOperation::Redo()
{
    if (m_objectIndex <= m_objects.size())
    {
        m_objects.insert(m_objects.begin() + m_objectIndex, *m_addedObject);
    }
}

std::string AddObjectOperation::GetDescription() const
{
    return "Add " + m_addedObject->GetName();
}

// RemoveObjectOperation implementation
RemoveObjectOperation::RemoveObjectOperation(std::vector<MapObject>& objects, int index)
    : m_objects(objects), m_objectIndex(index)
{
    if (index >= 0 && index < objects.size())
    {
        m_removedObject = new MapObject(objects[index]);
    }
    else
    {
        m_removedObject = nullptr;
    }
}

RemoveObjectOperation::~RemoveObjectOperation()
{
    delete m_removedObject;
}

void RemoveObjectOperation::Undo()
{
    if (m_objectIndex <= m_objects.size() && m_removedObject != nullptr)
    {
        m_objects.insert(m_objects.begin() + m_objectIndex, *m_removedObject);
    }
}

void RemoveObjectOperation::Redo()
{
    if (m_objectIndex >= 0 && m_objectIndex < m_objects.size())
    {
        m_objects.erase(m_objects.begin() + m_objectIndex);
    }
}

std::string RemoveObjectOperation::GetDescription() const
{
    if (m_removedObject != nullptr)
        return "Remove " + m_removedObject->GetName();
    return "Remove object";
}

// ModifyObjectOperation implementation
ModifyObjectOperation::ModifyObjectOperation(std::vector<MapObject>& objects, int index, 
                                           const MapObject& oldState, const MapObject& newState, 
                                           const std::string& propertyName)
    : m_objects(objects), m_objectIndex(index), m_propertyName(propertyName)
{
    m_oldState = new MapObject(oldState);
    m_newState = new MapObject(newState);
}

ModifyObjectOperation::~ModifyObjectOperation()
{
    delete m_oldState;
    delete m_newState;
}

void ModifyObjectOperation::Undo()
{
    if (m_objectIndex >= 0 && m_objectIndex < m_objects.size() && m_oldState != nullptr)
    {
        m_objects[m_objectIndex] = *m_oldState;
    }
}

void ModifyObjectOperation::Redo()
{
    if (m_objectIndex >= 0 && m_objectIndex < m_objects.size() && m_newState != nullptr)
    {
        m_objects[m_objectIndex] = *m_newState;
    }
}

std::string ModifyObjectOperation::GetDescription() const
{
    if (m_oldState != nullptr)
        return "Modify " + m_oldState->GetName() + " (" + m_propertyName + ")";
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
