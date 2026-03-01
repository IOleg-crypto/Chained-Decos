#include "command_history.h"
#include "engine/core/log.h"

namespace CHEngine
{
CommandHistory::CommandHistory(size_t maxHistory)
    : m_MaxHistory(maxHistory)
{
}

void CommandHistory::PushCommand(std::unique_ptr<IEditorCommand> command)
{
    if (!command)
    {
        return;
    }

    command->Execute();
    m_RedoStack.clear();
    m_UndoStack.push_back(std::move(command));

    if (m_UndoStack.size() > m_MaxHistory)
    {
        m_UndoStack.pop_front();
    }

    CH_CORE_INFO("Command pushed: {} (Undo stack size: {})", m_UndoStack.back()->GetName(), m_UndoStack.size());

    Notify();
}

void CommandHistory::Undo()
{
    if (m_UndoStack.empty())
    {
        return;
    }

    std::unique_ptr<IEditorCommand> command = std::move(m_UndoStack.back());
    m_UndoStack.pop_back();

    CH_CORE_INFO("Undoing command: {}", command->GetName());
    command->Undo();

    m_RedoStack.push_back(std::move(command));

    Notify();
}

void CommandHistory::Redo()
{
    if (m_RedoStack.empty())
    {
        return;
    }

    std::unique_ptr<IEditorCommand> command = std::move(m_RedoStack.back());
    m_RedoStack.pop_back();

    CH_CORE_INFO("Redoing command: {}", command->GetName());
    command->Execute();

    m_UndoStack.push_back(std::move(command));

    Notify();
}

void CommandHistory::Clear()
{
    m_UndoStack.clear();
    m_RedoStack.clear();
    Notify();
}

std::string CommandHistory::GetUndoName() const
{
    return m_UndoStack.empty() ? "" : m_UndoStack.back()->GetName();
}

std::string CommandHistory::GetRedoName() const
{
    return m_RedoStack.empty() ? "" : m_RedoStack.back()->GetName();
}

void CommandHistory::Notify()
{
    if (m_NotifyCallback)
    {
        m_NotifyCallback();
    }
}
} // namespace CHEngine
