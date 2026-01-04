#ifndef CH_COMMAND_HISTORY_H
#define CH_COMMAND_HISTORY_H

#include "editor_command.h"
#include <deque>
#include <memory>
#include <string>

namespace CH
{
/**
 * @brief Manages undo and redo stacks for the editor
 */
class CommandHistory
{
public:
    CommandHistory(size_t maxHistory = 50);
    ~CommandHistory() = default;

    /**
     * @brief Push a new command and execute it
     */
    void PushCommand(std::unique_ptr<IEditorCommand> command);

    /**
     * @brief Undo the last command
     */
    void Undo();

    /**
     * @brief Redo the last undone command
     */
    void Redo();

    /**
     * @brief Clear all history
     */
    void Clear();

    /**
     * @brief Check if undo is possible
     */
    bool CanUndo() const
    {
        return !m_UndoStack.empty();
    }

    /**
     * @brief Check if redo is possible
     */
    bool CanRedo() const
    {
        return !m_RedoStack.empty();
    }

    /**
     * @brief Get the name of the last command (for UI)
     */
    std::string GetUndoName() const;
    std::string GetRedoName() const;

private:
    size_t m_MaxHistory;
    std::deque<std::unique_ptr<IEditorCommand>> m_UndoStack;
    std::deque<std::unique_ptr<IEditorCommand>> m_RedoStack;
};
} // namespace CH

#endif // CH_COMMAND_HISTORY_H
