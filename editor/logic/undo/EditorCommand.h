#pragma once

#include <string>

namespace CHEngine
{

/**
 * @brief Base interface for all undoable commands in the editor
 */
class IEditorCommand
{
public:
    virtual ~IEditorCommand() = default;

    /**
     * @brief Execute the command
     */
    virtual void Execute() = 0;

    /**
     * @brief Undo the command
     */
    virtual void Undo() = 0;

    /**
     * @brief Get the user-friendly name of the command
     */
    virtual std::string GetName() const = 0;
};

} // namespace CHEngine
