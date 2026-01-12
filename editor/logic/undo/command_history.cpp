#include "command_history.h"
#include "engine/core/log.h"

namespace CHEngine
{
CommandHistory::CommandHistory(size_t maxHistory) : m_MaxHistory(maxHistory)
{
}

void CommandHistory::PushCommand(std::unique_ptr<IEditorCommand> command)
{
    if (!command)
        return;

    // Execute the command first
    command->Execute();

    // Clear redo stack on new command
    m_RedoStack.clear();

    m_UndoStack.push_back(std::move(command));

    if (m_UndoStack.size() > m_MaxHistory)
    {
        m_UndoStack.pop_front();
    }

    CH_CORE_INFO("Command pushed: %s (Undo stack size: %zu)", m_UndoStack.back()->GetName().c_str(),
                 m_UndoStack.size());
}

void CommandHistory::Undo()
{
    if (m_UndoStack.empty())
        return;

    std::unique_ptr<IEditorCommand> command = std::move(m_UndoStack.back());
    m_UndoStack.pop_back();

    CH_CORE_INFO("Undoing command: %s", command->GetName().c_str());
    command->Undo();

    m_RedoStack.push_back(std::move(command));
}

void CommandHistory::Redo()
{
    if (m_RedoStack.empty())
        return;

    std::unique_ptr<IEditorCommand> command = std::move(m_RedoStack.back());
    m_RedoStack.pop_back();

    CH_CORE_INFO("Redoing command: %s", command->GetName().c_str());
    command->Execute();

    m_UndoStack.push_back(std::move(command));
}

void CommandHistory::Clear()
{
    m_UndoStack.clear();
    m_RedoStack.clear();
}

std::string CommandHistory::GetUndoName() const
{
    return m_UndoStack.empty() ? "" : m_UndoStack.back()->GetName();
}

std::string CommandHistory::GetRedoName() const
{
    return m_RedoStack.empty() ? "" : m_RedoStack.back()->GetName();
}
} // namespace CHEngine
