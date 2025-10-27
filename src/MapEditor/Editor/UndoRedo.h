//
// Created by AI Assistant
//

#ifndef UNDOREDO_H
#define UNDOREDO_H

#include <vector>
#include <string>
#include <memory>
#include <functional>
#include "MapObject.h"

// Base class for undo/redo operations
class UndoRedoOperation
{
public:
    virtual ~UndoRedoOperation() = default;
    virtual void Undo() = 0;
    virtual void Redo() = 0;
    virtual std::string GetDescription() const = 0;
};

// Operation for adding an object
class AddObjectOperation : public UndoRedoOperation
{
private:
    std::vector<MapObject>& m_objects;
    std::unique_ptr<MapObject> m_addedObject;
    int m_objectIndex;

public:
    AddObjectOperation(std::vector<MapObject>& objects, const MapObject& obj, int index);
    void Undo() override;
    void Redo() override;
    std::string GetDescription() const override;
};

// Operation for removing an object
class RemoveObjectOperation : public UndoRedoOperation
{
private:
    std::vector<MapObject>& m_objects;
    std::unique_ptr<MapObject> m_removedObject;
    int m_objectIndex;

public:
    RemoveObjectOperation(std::vector<MapObject>& objects, int index);
    void Undo() override;
    void Redo() override;
    std::string GetDescription() const override;
};

// Operation for modifying object properties
class ModifyObjectOperation : public UndoRedoOperation
{
private:
    std::vector<MapObject>& m_objects;
    int m_objectIndex;
    std::unique_ptr<MapObject> m_oldState;
    std::unique_ptr<MapObject> m_newState;
    std::string m_propertyName;

public:
    ModifyObjectOperation(std::vector<MapObject>& objects, int index,
                         const MapObject& oldState, const MapObject& newState,
                         const std::string& propertyName);
    void Undo() override;
    void Redo() override;
    std::string GetDescription() const override;
};

// Main Undo/Redo manager
class UndoRedoManager
{
private:
    std::vector<std::unique_ptr<UndoRedoOperation>> m_undoStack;
    std::vector<std::unique_ptr<UndoRedoOperation>> m_redoStack;
    size_t m_maxStackSize;
    bool m_isRecording;

public:
    UndoRedoManager(size_t maxStackSize = 50);
    ~UndoRedoManager() = default;

    // Core functionality
    void PushOperation(std::unique_ptr<UndoRedoOperation> operation);
    bool CanUndo() const;
    bool CanRedo() const;
    void Undo();
    void Redo();
    void Clear();

    // Recording control
    void StartRecording();
    void StopRecording();
    bool IsRecording() const;

    // Information
    std::string GetUndoDescription() const;
    std::string GetRedoDescription() const;
    size_t GetUndoStackSize() const;
    size_t GetRedoStackSize() const;

    // Factory methods for common operations
    void RecordAddObject(std::vector<MapObject>& objects, const MapObject& obj, int index);
    void RecordRemoveObject(std::vector<MapObject>& objects, int index);
    void RecordModifyObject(std::vector<MapObject>& objects, int index, 
                           const MapObject& oldState, const MapObject& newState, 
                           const std::string& propertyName);
};

#endif // UNDOREDO_H
