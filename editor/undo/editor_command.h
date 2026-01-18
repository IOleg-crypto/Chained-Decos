#ifndef CH_EDITOR_COMMAND_H
#define CH_EDITOR_COMMAND_H

#include <string>

namespace CHEngine
{
class IEditorCommand
{
public:
    virtual ~IEditorCommand() = default;

    virtual void Execute() = 0;

    virtual void Undo() = 0;

    virtual std::string GetName() const = 0;
};
} // namespace CHEngine

#endif // CH_EDITOR_COMMAND_H
