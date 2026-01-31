#ifndef CH_COMMAND_HISTORY_H
#define CH_COMMAND_HISTORY_H

#include "editor_command.h"
#include "deque"
#include <functional>
#include <memory>
#include <string>

namespace CHEngine
{
class CommandHistory
{
public:
    using CommandEventCallback = std::function<void()>;

    CommandHistory(size_t maxHistory = 50);
    ~CommandHistory() = default;

    void PushCommand(std::unique_ptr<IEditorCommand> command);
    void Undo();
    void Redo();
    void Clear();

    bool CanUndo() const
    {
        return !m_UndoStack.empty();
    }
    bool CanRedo() const
    {
        return !m_RedoStack.empty();
    }

    std::string GetUndoName() const;
    std::string GetRedoName() const;

    void SetNotifyCallback(CommandEventCallback callback)
    {
        m_NotifyCallback = callback;
    }

private:
    void Notify();

private:
    size_t m_MaxHistory;
    std::deque<std::unique_ptr<IEditorCommand>> m_UndoStack;
    std::deque<std::unique_ptr<IEditorCommand>> m_RedoStack;
    CommandEventCallback m_NotifyCallback;
};
} // namespace CHEngine

#endif // CH_COMMAND_HISTORY_H
